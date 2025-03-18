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
#include "DisplayInterface.h"

#define SS_PIN  5
#define RST_PIN 22
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

#define TFT_CS 15

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

void resetSPIForDisplay() {
    // End current SPI configuration
    SPI.end();
    delay(20);
    
    pinMode(MOSI_PIN, INPUT);
    pinMode(MISO_PIN, INPUT);
    pinMode(SCK_PIN, INPUT);
    pinMode(SS_PIN, INPUT);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH); // Deselect display
    delay(10);

    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
    pinMode(SCK_PIN, OUTPUT);

    // Restart SPI with display settings
    SPI.begin();
    delay(20);
    
    // Reinitialize display
    display.begin();
    delay(50);
}

void updateDisplayAsync() {
    static unsigned long lastDisplayUpdate = 0;
    static UIState lastRenderedState = display.getState();
    static int lastSelectedOption = display.getSelectedOption();
    static bool stateJustChanged = false;

    if (display.getState() == USER_LIST_SENDER || 
        display.getState() == USER_LIST_RECIPIENT || 
        display.getState() == ENCRYPTED_MESSAGES) {
        // Just keep track of state but DON'T update screen
        lastRenderedState = display.getState();
        lastSelectedOption = display.getSelectedOption();
        return; // Skip updates entirely while selecting users
    }

    bool forceUpdate = false;
    
        // Check if important state has changed, requiring immediate update
    if (lastRenderedState != display.getState() || 
        lastSelectedOption != display.getSelectedOption()) {
        forceUpdate = true;
        lastRenderedState = display.getState();
        lastSelectedOption = display.getSelectedOption();
        stateJustChanged = true;
    }
    
    // Only update if forced or enough time has passed (100ms = 10fps)
    if (forceUpdate || millis() - lastDisplayUpdate > 500) { // Increased from 300 to 500ms
        display.update(stateJustChanged);
        stateJustChanged = false;
        lastDisplayUpdate = millis();
    }
}

int AguardaLeituraRFID() {
    Serial.println(F("Aguardando cartão RFID..."));
    display.showRFIDReadingScreen();
    delay(300);

    unsigned long tempoInicio = millis();
    const unsigned int TIMEOUT = 20000; // 20 segundos de timeout
    
    digitalWrite(LED_BUILTIN, HIGH);

    SPI.end(); // Completely end SPI
    delay(100);
    
    // Explicitly set up all pins for RFID
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
    pinMode(SCK_PIN, OUTPUT);
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, HIGH); // Deselect RFID
    delay(10);

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    mfrc522.PCD_Init(SS_PIN, RST_PIN);
    delay(100);
    mfrc522.PCD_Reset();
    delay(50);
    mfrc522.PCD_Init(SS_PIN, RST_PIN);
    delay(50);
    //mfrc522.PCD_AntennaOn();
    delay(50);

    while (millis() - tempoInicio < TIMEOUT) {
        // Check for card presence without any display updates
        if (mfrc522.PICC_IsNewCardPresent()) {
            Serial.println(F("Cartão detectado!"));
            
            if (mfrc522.PICC_ReadCardSerial()) {
                Serial.print("UID do cartão: ");
                for (byte i = 0; i < mfrc522.uid.size; i++) {
                    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
                    Serial.print(mfrc522.uid.uidByte[i], HEX);
                }
                Serial.println();
                digitalWrite(LED_BUILTIN, LOW);
                return 1;
            } else {
                Serial.println(F("Falha na leitura do cartão"));
            }
        }
        
        // Blink LED as visual indicator instead of display animation
        if (millis() % 500 < 250) {
            digitalWrite(LED_BUILTIN, HIGH);
        } else {
            digitalWrite(LED_BUILTIN, LOW);
        }
        
        delay(10); // Short delay is still needed
    }
  
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(F("Tempo esgotado! Nenhum cartão detectado."));
    display.showRFIDTimeout();
    delay(2000);

    return 0;
}

void CadastraUsuario(Usuario *usuario) {
    PrivateKeys *chavePrivada;
    
    Serial.println(F("Digite o nome do usuário: "));
    LimpaBufferSerial();
    String nome = LerString();

    if (nome.length() == 0) {
        Serial.println(F("Nome não pode ser vazio!"));
        display.showMessage("Nome nao pode ser vazio!", ERROR_DISPLAY_TIME);
        return;
    }

    strncpy(usuario->username, nome.c_str(), MAX_USERNAME_LENGTH - 1);
    usuario->username[MAX_USERNAME_LENGTH - 1] = '\0';

    strncpy(display.usernameField.text, nome.c_str(), sizeof(display.usernameField.text) - 1);
    display.update(true);
    
    InitKeys(&chavePrivada, &usuario->chavePublica);
    GeraChaves(chavePrivada, usuario->chavePublica);
    
    Serial.print("\nChaves geradas para " + String(usuario->username) + ":");
    MostraChaves(chavePrivada, usuario->chavePublica);
    Serial.println(F("\nIMPORTANTE: Aproxime a TAG RFID para salvar as chaves privadas!"));

    display.showMessage("Chaves geradas!\nAproxime o cartao RFID", 200);

    if (AguardaLeituraRFID()) {
        if (SalvarChaves(chavePrivada, NULL, &mfrc522)) {
            Serial.println(F("Chaves salvas com sucesso!"));
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            resetSPIForDisplay();
            display.showRFIDSuccess();
            display.showMessage("Chaves salvas com sucesso!", SUCCESS_DISPLAY_TIME);
        } else {
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            resetSPIForDisplay();
            Serial.println(F("ERRO: Falha ao salvar chaves no cartão!"));
            Serial.println(F("O usuário foi cadastrado mas as chaves privadas não foram salvas."));
            Serial.println(F("Por favor, tente novamente com outro cartão RFID."));
            display.showMessage("Falha ao salvar chaves!\nUsuario cadastrado sem chaves", ERROR_DISPLAY_TIME);
        }
    } else {
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        resetSPIForDisplay();
        Serial.println(F("Nenhum cartão RFID detectado no tempo limite."));
        Serial.println(F("O usuário foi cadastrado mas as chaves privadas não foram salvas."));
        display.showMessage("Timeout! Usuario cadastrado\nmas sem chaves salvas", ERROR_DISPLAY_TIME);
    }
    DeleteKeys(chavePrivada, NULL);
    display.setState(MENU_PRINCIPAL);
    display.update(true);
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
    display.showUserList(usuarios, numUsuarios, "Enviar Mensagem");

    Serial.println(F("Remetente: "));
    LimpaBufferSerial();
    while (!Serial.available()) {
        delay(50);
    }
    String remetente = LerString();
    
    if (remetente.length() == 0) {
        Serial.println(F("Remetente não pode ser vazio!"));
        display.showMessage("Remetente não pode ser vazio!", ERROR_DISPLAY_TIME);
        return;
    }

    display.showUserList(usuarios, numUsuarios, "Selecione Destinatario");

    Serial.println(F("Digite o nome do destinatário: "));
    LimpaBufferSerial();
    while (!Serial.available()) {
        delay(50);
    }
    String destinatario = LerString();
    
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, destinatario.c_str());
    if (idDestinatario == -1) {
        Serial.println(F("Destinatário não encontrado!"));
        display.showMessage("Destinatario nao encontrado!", ERROR_DISPLAY_TIME);
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
    display.showMessage("Mensagem enviada\ncom sucesso!", SUCCESS_DISPLAY_TIME);
    display.setState(MENU_PRINCIPAL);
    display.update(true);
}

void LerMensagens(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int numMensagens) {
    display.resetDisplay();
    display.setState(LER_MENSAGENS);
    display.showReadMessages();
    Serial.println(F("\nDigite seu username: "));
    LimpaBufferSerial();
    while (!Serial.available()) {
        delay(50);
    }
    String username = LerString();
    
    int idUsuario = EncontraUsuario(usuarios, numUsuarios, username.c_str());
    if (idUsuario == -1) {
        Serial.println(F("Usuário não encontrado!"));
        display.showMessage("Usuario nao encontrado!", ERROR_DISPLAY_TIME);
        return;
    }

    display.selectedUser = idUsuario;

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
        display.showMessage("Nenhuma mensagem encontrada", MESSAGE_DISPLAY_TIME);
        return;
    }
    display.showEncryptedMessages(&usuarios[idUsuario], mensagens, numMensagens);
    //display.update(true);
    delay(100);

    Serial.println(F("\nDeseja descriptografar as mensagens? (1-Sim/0-Não)"));
    LimpaBufferSerial();
    while (!Serial.available()) {
        delay(10);
    }
    // Lê o valor como string e converte para int
    String resposta = LerString();
    int opc = resposta.toInt();

    if (opc == 1) {
        display.setDecryptMode(true);
        display.update(true);
        PrivateKeys *ChavePrivada;
        InitKeys(&ChavePrivada, NULL);
        
        Serial.println(F("Aproxime o cartão RFID para ler a chave privada..."));
        if (AguardaLeituraRFID()) {
            if (!LerChavesPrivadas(ChavePrivada, &mfrc522)) {
                Serial.println(F("Erro ao ler chaves do cartão!"));
                mfrc522.PICC_HaltA();
                mfrc522.PCD_StopCrypto1();
                resetSPIForDisplay();
                display.showMessage("Erro ao ler chaves do cartao!", ERROR_DISPLAY_TIME);
                DeleteKeys(ChavePrivada, NULL);
                return;
            }
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            resetSPIForDisplay();
            display.clearScreen();
            display.showRFIDSuccess();
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
            
            String mensagensDecriptadas = "";

            for (int i = 0; i < numMensagens; i++) {
                if (strcmp(mensagens[i].destinatario, username.c_str()) == 0) {
                    unsigned char mensagemDecriptada[MAX_MESSAGE_LENGTH + 1] = {0};
                    
                    if (DecriptaMensagem(mensagens[i].mensagemCriptografada, 
                    mensagens[i].tamanhoMsg, mensagemDecriptada, ChavePrivada)) {
                        mensagemDecriptada[MAX_MESSAGE_LENGTH] = '\0';
                        algumSucesso = true;

                        // Adicionar a mensagem para exibir no display
                        mensagensDecriptadas += "De: ";
                        mensagensDecriptadas += mensagens[i].remetente;
                        mensagensDecriptadas += " [";
                        mensagensDecriptadas += mensagens[i].data;
                        mensagensDecriptadas += "]\n";
                        mensagensDecriptadas += (char*)mensagemDecriptada;
                        mensagensDecriptadas += "\n\n";

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

            if (algumSucesso) {
                if (mensagensDecriptadas.length() > 500) {
                    xTaskCreate(
                        [](void* parameter) {
                            char* msg = (char*)parameter;
                            display.showMessageDetailPaged(msg);
                            free(parameter); // Free the allocated memory
                        },
                        "DisplayTask",
                        4096,
                        strdup(mensagensDecriptadas.c_str()), // Copy the message to heap
                        1,
                        NULL
                    );
                } else {
                    // Use regular display for shorter messages
                    display.showDecryptedMessages(mensagensDecriptadas.c_str());
                    delay(8000); // Give time to read
                }
            } else {
                Serial.println(F("Nenhuma mensagem foi descriptografada com sucesso."));
                Serial.println(F("Verifique se o cartão RFID contém as chaves corretas."));
                display.showMessage("Falha ao descriptografar!\nVerifique o cartao RFID", ERROR_DISPLAY_TIME);
            }
        }
        DeleteKeys(ChavePrivada, NULL);
        display.setDecryptMode(false);
        display.resetDisplay();
        display.setState(MENU_PRINCIPAL);
        display.update(true);
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

    display.setState(STATUS_SISTEMA);
    display.update(true);
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

    // Test if RFID reader is responding
    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (version == 0x00 || version == 0xFF) {
        Serial.println(F("AVISO: Problema na comunicação com RFID"));
        display.showMessage("Problema com RFID\nVerifique conexoes!", ERROR_DISPLAY_TIME);
    } else {
        Serial.print(F("RFID OK, versao: 0x"));
        Serial.println(version, HEX);
    }

    inicializarSPIFFS();
    delay(300);
    verificarArquivosNecessarios();
    delay(100);

    display.begin();
    display.showSplashScreen();
    delay(100);

    for(int i=0; i<6; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }

    //ativarWiFi();
    delay(300);

    Serial.println(F("Inicialização completa!"));
    Serial.println(F("===================================\n"));
    digitalWrite(LED_BUILTIN, LOW);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);

    // Mostrar menu principal no display
    display.setState(MENU_PRINCIPAL);
    display.update(true);
}

void loop() {
    static unsigned long lastWifiCheck = 0;
    static unsigned long lastHeartbeat = 0;
    static unsigned long lastUpdateTime = 0;
    static bool menuExibido = false;

    if(millis() - lastUpdateTime > 500) {
        lastUpdateTime = millis();
    }

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
        Serial.println(F("[1] - Cadastrar usuário"));
        Serial.println(F("[2] - Enviar mensagem"));
        Serial.println(F("[3] - Ler mensagens"));
        Serial.println(F("[8] - Ativar/Desativar WiFi"));
        Serial.println(F("[9] - Status do sistema")); 
        Serial.println(F("[0] - Sair"));
        Serial.println(F("=================="));
        Serial.print("Opção: ");
        
        menuExibido = true;
        // Mostrar menu principal no display quando o menu é reexibido no terminal
        display.setState(MENU_PRINCIPAL);
        display.update(true);
        delay(50);
    }
    
    if (Serial.available()) {
      int opcao = Serial.parseInt();
      LimpaBufferSerial();
      switch(opcao) {
          case 1:
            if (numUsuarios < MAX_USERS) {
                display.setState(CADASTRAR_USUARIO);
                display.update(true);
                CadastraUsuario(&usuarios[numUsuarios++]);
                display.setState(MENU_PRINCIPAL);
                display.update(true);
              } else {
                  Serial.println(F("Limite de usuários atingido!"));
                  display.showMessage("Limite de usuarios atingido!", ERROR_DISPLAY_TIME);
              }
              break;
              
          case 2:
              if (numMensagens < MAX_MESSAGES) {
                //display.showMessage("Digite os dados\nno terminal...", 0);
                EnviarMensagem(usuarios, numUsuarios, mensagens, &numMensagens);
              } else {
                  Serial.println(F("Limite de mensagens atingido!"));
                  display.showMessage("Limite de mensagens\natingido!", ERROR_DISPLAY_TIME);
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
                    display.showMessage("Ativando WiFi...", 0);
                    ativarWiFi();
                    if (WiFi.status() == WL_CONNECTED) {
                        String ipMessage = "WiFi conectado!\nIP: " + WiFi.localIP().toString();
                        display.showMessage(ipMessage.c_str(), SUCCESS_DISPLAY_TIME);
                    } else {
                        wifiEnabled = false;
                        display.showMessage("Falha na conexao WiFi", ERROR_DISPLAY_TIME);
                    }
                } else {
                    display.showMessage("Desativando WiFi...", SUCCESS_DISPLAY_TIME);
                    wifiEnabled = false;
                }
                break;

          case 9:
              mostrarStatusSistema();
              display.setState(STATUS_SISTEMA);
              display.update(true);
              delay(8000); // Mostrar status por 5 segundos
              display.setState(MENU_PRINCIPAL);
              display.update(true);
              break;
              
          case 0:
            {
                Serial.println("Digite DESLIGAR para confirmar.");
                display.showMessage("Digite DESLIGAR para confirmar\ndesligamento", 0);
                String resposta = LerString();
                resposta.trim();

                if (resposta == "DESLIGAR") {
                  Serial.println(F("Limpando recursos..."));
                  display.showMessage("Limpando recursos...\nDesligando...", 0);
                  for (int i = 0; i < numUsuarios; i++) {
                      DeleteKeys(NULL, usuarios[i].chavePublica);
                  }
                  Serial.println(F("Programa finalizado."));
                  delay(1000);
                  ESP.restart();
                } else {
                  display.showMessage("Operacao cancelada", SUCCESS_DISPLAY_TIME);
                }
              }
                break;
              
          default:
              Serial.println(F("Opção inválida!"));
              display.showMessage("Opcao invalida!", ERROR_DISPLAY_TIME);
              break;
      }

      menuExibido = false;
      delay(200);
    } else{
      delay(200);
    }
    updateDisplayAsync();
}