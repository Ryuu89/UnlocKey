#include <Arduino.h>
#include "DataTypes.h"
#include "WebInterface.h"
#include "rsa.h"
#include <SPI.h>
#include <MFRC522.h>
#include <time.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WiFi.h>
#include <SPIFFS.h>

#define SS_PIN  5
#define RST_PIN 22
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif
#define DEBUG_ON 1

const char *ssid = "Galaxy A32773F";
const char *password = "gooo0280";
const long gmtOffset_sec = -3 * 3600; // UTC-3;
const int daylightOffset_sec = 0; // config horário de verão
bool wifiEnabled = false;

MFRC522 mfrc522(SS_PIN, RST_PIN);
Usuario usuarios[MAX_USERS];
Mensagem mensagens[MAX_MESSAGES];
int numUsuarios = 0, numMensagens = 0;
unsigned long lastHeapCheck = 0;

void LimpaBufferSerial() {
    while(Serial.available() > 0) {
        Serial.read();
    }
}

String LerString() {
    String result = "";
    while(!Serial.available()) {
        delay(10);
    }
    delay(100);
    while(Serial.available() > 0) {
        char c = Serial.read();
        if(c == '\n' || c == '\r') continue;
        result += c;
    }
    return result;
}

void SetDataAtual(char* data) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        strcpy(data, "00/00/0000 00:00\0");
        return;
    }
    strftime(data, 17, "%d/%m/%Y %H:%M", &timeinfo);
}

int AguardaLeituraRFID() {
    Serial.println(F("Aguardando cartão RFID..."));
    unsigned long tempoInicio = millis();
    const unsigned int TIMEOUT = 20000; // 20 segundos de timeout
    
    digitalWrite(LED_BUILTIN, HIGH);

    while (millis() - tempoInicio < TIMEOUT) {
        if (!mfrc522.PICC_IsNewCardPresent()) {
            delay(50);
            continue;
        }
        Serial.println(F("Cartão detectado!"));

        if (!mfrc522.PICC_ReadCardSerial()) {
            Serial.println(F("Falha na leitura do cartão"));
            continue;
        }

        Serial.print("UID do cartão: ");
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
        }
        Serial.println();
        digitalWrite(LED_BUILTIN, LOW);
        return 1;
    }
  
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(F("Tempo esgotado! Nenhum cartão detectado."));
    return 0;
}

void CadastraUsuario(Usuario *usuario) {
    PrivateKeys *chavePrivada;
    Serial.println(F("Digite o nome do usuário: "));
    LimpaBufferSerial();
    String nome = LerString();

    if (nome.length() == 0) {
        Serial.println(F("Nome não pode ser vazio!"));
        return;
    }

    strncpy(usuario->username, nome.c_str(), MAX_USERNAME_LENGTH - 1);
    usuario->username[MAX_USERNAME_LENGTH - 1] = '\0';
    
    InitKeys(&chavePrivada, &usuario->chavePublica);
    GeraChaves(chavePrivada, usuario->chavePublica);
    
    Serial.print("\nChaves geradas para " + String(usuario->username) + ":");
    MostraChaves(chavePrivada, usuario->chavePublica);
    Serial.println(F("\nIMPORTANTE: Aproxime a TAG RFID para salvar as chaves privadas!"));

    if (AguardaLeituraRFID()) {
        if (SalvarChaves(chavePrivada, NULL, &mfrc522)) {
            Serial.println(F("Chaves salvas com sucesso!"));
        } else {
            Serial.println(F("ERRO: Falha ao salvar chaves no cartão!"));
            Serial.println(F("O usuário foi cadastrado mas as chaves privadas não foram salvas."));
            Serial.println(F("Por favor, tente novamente com outro cartão RFID."));
        }
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    } else {    
        Serial.println(F("Nenhum cartão RFID detectado no tempo limite."));
        Serial.println(F("O usuário foi cadastrado mas as chaves privadas não foram salvas."));
    }
    DeleteKeys(chavePrivada, NULL);
}

int EncontraUsuario(Usuario *usuarios, int numUsuarios, const char *username) {
    for (int i = 0; i < numUsuarios; i++) {
        if (strcmp(usuarios[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

void EnviarMensagem(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int *numMensagens) {
    Serial.println(F("Remetente: "));
    LimpaBufferSerial();
    String remetente = LerString();
    
    if (remetente.length() == 0) {
        Serial.println(F("Remetente não pode ser vazio!"));
        return;
    }
    
    Serial.println(F("Destinatário: "));
    LimpaBufferSerial();
    String destinatario = LerString();
    
    int idRemetente = EncontraUsuario(usuarios, numUsuarios, remetente.c_str());
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, destinatario.c_str());
    
    if (idRemetente == -1 || idDestinatario == -1) {
        Serial.println(F("Usuário não encontrado!"));
        return;
    }
    
    Serial.println(F("Mensagem: "));
    LimpaBufferSerial();
    String mensagem = LerString();

    // Trunca a mensagem se for maior que o limite
    if (mensagem.length() > MAX_MESSAGE_LENGTH) {
        mensagem.remove(MAX_MESSAGE_LENGTH);
    }
    
    // Copia remetente/destinatario com segurança
    strncpy(mensagens[*numMensagens].remetente, remetente.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[*numMensagens].remetente[MAX_USERNAME_LENGTH - 1] = '\0';
    
    strncpy(mensagens[*numMensagens].destinatario, destinatario.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[*numMensagens].destinatario[MAX_USERNAME_LENGTH - 1] = '\0';

    // Guarda tamanho real da mensagem
    mensagens[*numMensagens].tamanhoMsg = mensagem.length();
    SetDataAtual(mensagens[*numMensagens].data);
    
    EncriptaMensagem((unsigned char*)mensagem.c_str(), 
                     mensagens[*numMensagens].mensagemCriptografada, 
                     usuarios[idDestinatario].chavePublica);
    
    (*numMensagens)++;
    Serial.println(F("Mensagem enviada com sucesso!"));
}

void LerMensagens(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int numMensagens) {
    Serial.println(F("\nDigite seu username: "));
    LimpaBufferSerial();
    String username = LerString();
    
    int idUsuario = EncontraUsuario(usuarios, numUsuarios, username.c_str());
    if (idUsuario == -1) {
        Serial.println(F("Usuário não encontrado!"));
        return;
    }
    
    bool temMensagem = false;
    for (int i = 0; i < numMensagens; i++) {
        if (strcmp(mensagens[i].destinatario, username.c_str()) == 0) {
            temMensagem = true;
            Serial.print("\nDe: ");
            Serial.print(mensagens[i].remetente);
            Serial.print(" [");
            Serial.print(mensagens[i].data);
            Serial.print("]\n\t");
            
            for (unsigned int j = 0; j < mensagens[i].tamanhoMsg; j++) {
                Serial.write((char)(mensagens[i].mensagemCriptografada[j] % 94 + 33));
            }
            Serial.println();
        }
    }
    
    if (!temMensagem) {
        Serial.println(F("Nenhuma mensagem encontrada."));
        return;
    }

    Serial.println(F("\nDeseja descriptografar as mensagens? (1-Sim/0-Não)"));
    LimpaBufferSerial();
    while (!Serial.available()) {
        delay(10);
    }
    // Lê o valor como string e converte para int
    String resposta = LerString();
    int opc = resposta.toInt();

    if (opc == 1) {
        PrivateKeys *ChavePrivada;
        InitKeys(&ChavePrivada, NULL);
        
        Serial.println(F("Aproxime o cartão RFID para ler a chave privada..."));
        if (AguardaLeituraRFID()) {
            if (!LerChavesPrivadas(ChavePrivada, &mfrc522)) {
                Serial.println(F("Erro ao ler chaves do cartão!"));
                DeleteKeys(ChavePrivada, NULL);
                return;
            }

            if (DEBUG_ON) {
                // Print encrypted message details without exposing keys
                Serial.println(F("\nDetalhes da mensagem criptografada:"));
                for (int i = 0; i < numMensagens; i++) {
                    if (strcmp(mensagens[i].destinatario, username.c_str()) == 0) {
                        Serial.printf("Mensagem de %s:\n", mensagens[i].remetente);
                        Serial.printf("Tamanho: %d bytes\n", mensagens[i].tamanhoMsg);
                        Serial.printf("Chaves privadas: ");
                        MostraChaves(ChavePrivada, NULL);
                    }
                }
            }

            Serial.println(F("\nMensagens descriptografadas:"));
            bool algumSucesso = false;
            
            for (int i = 0; i < numMensagens; i++) {
                if (strcmp(mensagens[i].destinatario, username.c_str()) == 0) {
                    unsigned char mensagemDecriptada[MAX_MESSAGE_LENGTH + 1] = {0};
                    
                    if (DecriptaMensagem(mensagens[i].mensagemCriptografada, 
                    mensagens[i].tamanhoMsg, mensagemDecriptada, ChavePrivada)) {
                        mensagemDecriptada[MAX_MESSAGE_LENGTH] = '\0';
                        algumSucesso = true;
                        Serial.print("\nDe: ");
                        Serial.print(mensagens[i].remetente);
                        Serial.print(" [");
                        Serial.print(mensagens[i].data);
                        Serial.print("]\n\t");
                        Serial.println((char*)mensagemDecriptada);
                    } else {
                        if (DEBUG_ON) {
                            Serial.println(F("Falha na descriptografia. Verificando valores:"));
                            Serial.print("Remetente: ");
                            Serial.println(mensagens[i].remetente);
                            Serial.print("Tamanho da mensagem: ");
                            Serial.println(mensagens[i].tamanhoMsg);
                        }
                    }
                }
            }

            if (!algumSucesso) {
                Serial.println(F("Nenhuma mensagem foi descriptografada com sucesso."));
                Serial.println(F("Verifique se o cartão RFID contém as chaves corretas."));
            }

            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
        }
        DeleteKeys(ChavePrivada, NULL);
    }
}

void mostrarStatusSistema() {
    Serial.println(F("\n===== STATUS DO SISTEMA ====="));
    Serial.print("Usuários: ");
    Serial.print(numUsuarios);
    Serial.print("/");
    Serial.println(MAX_USERS);
    
    Serial.print("Mensagens: ");
    Serial.print(numMensagens);
    Serial.print("/");
    Serial.println(MAX_MESSAGES);

    Serial.print("Memória livre: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi: Conectado ");
        yield();
    
        // Verificação mais segura para RSSI
        int rssi = WiFi.RSSI();
        if (rssi < 0 && rssi > -120) {  // Valor razoável
            Serial.print(F("("));
            Serial.print(rssi);
            Serial.println(F(" dBm)"));
        } else {
          Serial.println(F("(sinal não mensurável)"));
        }
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Interface web: http://");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(F("WiFi: Desconectado"));
    }
    Serial.println(F("==========================="));
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    delay(100);
    Serial.begin(115200);
    delay(500);
    btStop();

    Serial.println(F("\n"));
    Serial.println(F("██╗░░░██╗███╗░░██╗██╗░░░░░░█████╗░░█████╗░██╗░░██╗███████╗██╗░░░██╗"));
    Serial.println(F("██║░░░██║████╗░██║██║░░░░░██╔══██╗██╔══██╗██║░██╔╝██╔════╝╚██╗░██╔╝"));
    Serial.println(F("██║░░░██║██╔██╗██║██║░░░░░██║░░██║██║░░╚═╝█████═╝░█████╗░░░╚████╔╝░"));
    Serial.println(F("██║░░░██║██║╚████║██║░░░░░██║░░██║██║░░██╗██╔═██╗░██╔══╝░░░░╚██╔╝░░"));
    Serial.println(F("╚██████╔╝██║░╚███║███████╗╚█████╔╝╚█████╔╝██║░╚██╗███████╗░░░██║░░░"));
    Serial.println(F("░╚═════╝░╚═╝░░╚══╝╚══════╝░╚════╝░░╚════╝░╚═╝░░╚═╝╚══════╝░░░╚═╝░░░"));
    Serial.println(F("\n       Sistema de Mensagens Criptografadas em RSA\n"));
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);  // LED indica inicialização
    delay(100);
    SPI.begin();
    delay(100);

    // Initialize MFRC522
    Serial.println(F("Iniciando MFRC522..."));
    mfrc522.PCD_Init(SS_PIN, RST_PIN);
    delay(100);

    inicializarSPIFFS();
    delay(300);
    verificarArquivosNecessarios();
    delay(100);

    for(int i=0; i<6; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }

    ativarWiFi();
    delay(300);
    Serial.println(F("Inicialização completa!"));
    Serial.println(F("===================================\n"));
    digitalWrite(LED_BUILTIN, LOW);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
}

void loop() {
    static unsigned long lastWifiCheck = 0;
    static bool menuExibido = false;
    if (millis() - lastWifiCheck > 30000 || millis() < lastWifiCheck) {
        lastWifiCheck = millis();
        verificarWiFi();
    }

    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    }

    if (novaMensagemWeb) {
      processarMensagemWeb();
    }

    if (!menuExibido) {
        Serial.println(F("\n=== Menu Principal ==="));
        Serial.println(F("1 - Cadastrar usuário"));
        Serial.println(F("2 - Enviar mensagem"));
        Serial.println(F("3 - Ler mensagens"));
        Serial.println(F("8 - Ativar/Desativar WiFi"));
        Serial.println(F("9 - Status do sistema")); 
        Serial.println(F("0 - Sair"));
        Serial.println(F("=================="));
        Serial.print("Opção: ");
        
        menuExibido = true;
    }
    
    if (Serial.available()) {
      int opcao = Serial.parseInt();
      LimpaBufferSerial();
      switch(opcao) {
          case 1:
              if (numUsuarios < MAX_USERS) {
                  CadastraUsuario(&usuarios[numUsuarios++]);
              } else {
                  Serial.println(F("Limite de usuários atingido!"));
              }
              break;
              
          case 2:
              if (numMensagens < MAX_MESSAGES) {
                  EnviarMensagem(usuarios, numUsuarios, mensagens, &numMensagens);
              } else {
                  Serial.println(F("Limite de mensagens atingido!"));
              }
              break;
              
          case 3:
              LerMensagens(usuarios, numUsuarios, mensagens, numMensagens);
              break;

          case 8:
              if (!wifiEnabled){
                Serial.print(F("Memória livre antes de conectar: "));
                Serial.print(ESP.getFreeHeap());
                Serial.println(F(" bytes"));
                ativarWiFi();
              } else {
                wifiEnabled = false;
              }
              
              break;

          case 9:
              mostrarStatusSistema();
              break;
              
          case 0:
            {
              Serial.println("Digite 0 para confirmar.");
              String resposta = LerString();
              int opc = resposta.toInt();

              if (opc == 0) {
                Serial.println(F("Limpando recursos..."));
                for (int i = 0; i < numUsuarios; i++) {
                    DeleteKeys(NULL, usuarios[i].chavePublica);
                }
                Serial.println(F("Programa finalizado."));
                delay(1000);
                ESP.restart();
              }
            }
              break;
              
          default:
              Serial.println(F("Opção inválida!"));
              break;
      }

      menuExibido = false;
      delay(500);
    } else{
      delay(200);
    }
}