#include <Arduino.h>
#include "rsa.h"
#include <SPI.h>
#include <MFRC522.h>
#include <time.h>

// Bibliotecas para servidor web
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

#define MAX_USERS 5
#define MAX_MESSAGES 10
#define MAX_MESSAGE_LENGTH 100
#define MAX_USERNAME_LENGTH 15  

#define SS_PIN  21
#define RST_PIN 22
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

#ifdef DEBUG
  #undef DEBUG
#endif
#define DEBUG_ON 1

const char *ssid = "SuaRedeWiFi";
const char *password = "SuaSenha";
const long gmtOffset_sec = -3 * 3600; // UTC-3;
const int daylightOffset_sec = 0; // config horário de verão

WebServer server(80);
MFRC522 mfrc522(SS_PIN, RST_PIN);

typedef struct {
    char remetente[MAX_USERNAME_LENGTH];
    char destinatario[MAX_USERNAME_LENGTH];
    unsigned long mensagemCriptografada[MAX_MESSAGE_LENGTH];
    unsigned int tamanhoMsg;
    char data[17]; // DD/MM/YYYY HH:MM
} Mensagem;

typedef struct {
    char username[MAX_USERNAME_LENGTH];
    PublicKeys *chavePublica;
} Usuario;

Usuario usuarios[MAX_USERS];
Mensagem mensagens[MAX_MESSAGES];
int numUsuarios = 0, numMensagens = 0;
String webRemetente;
String webDestinatario;
String webMensagem; 
bool novaMensagemWeb = false;

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
        strcpy(data, "??/??/???? 00:00");
        return;
    }
    strftime(data, 17, "%d/%m/%Y %H:%M", &timeinfo);
}

int AguardaLeituraRFID() {
    Serial.println(F("Aguardando cartão RFID..."));
    unsigned long tempoInicio = millis();
    const unsigned int TIMEOUT = 20000; // 20 segundos de timeout
    
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
        
        return 1;
    }
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
                        Serial.println(F((char*)mensagemDecriptada));
                    } else {
                        if (DEBUG_ON) {
                            Serial.println(F("Falha na descriptografia. Verificando valores:"));
                            Serial.print("Remetente: ");
                            Serial.println(F(mensagens[i].remetente));
                            Serial.print("Tamanho da mensagem: ");
                            Serial.println(F(mensagens[i].tamanhoMsg));
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

void inicializarSPIFFS() {
    if (!SPIFFS.begin(true)) {
      Serial.println(F("Erro ao montar SPIFFS"));
      return;
    }
    Serial.println(F("SPIFFS montado com sucesso"));
    
    // Listar arquivos no SPIFFS
    Serial.println(F("Arquivos no SPIFFS:"));
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file) {
        Serial.print("  - ");
        Serial.print(file.name());
        Serial.print(" (");
        Serial.print(file.size());
        Serial.println(F(" bytes)"));
        file = root.openNextFile();
    }
}

bool verificarArquivosNecessarios() {
    bool status = true;
    
    if (!SPIFFS.exists("/index.html")) {
        Serial.println(F("Erro: arquivo index.html não encontrado!"));
        status = false;
    }
    
    if (!SPIFFS.exists("/success.html")) {
        Serial.println(F("Erro: arquivo success.html não encontrado!"));
        status = false;
    }
    
    return status;
}

void inicializarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Conectando ao WiFi ");

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F(""));
        Serial.print("Conectado ao WiFi: ");
        Serial.println(F(ssid));
        Serial.print("Endereço IP: ");
        Serial.println(WiFi.localIP());
      
        if (MDNS.begin("unlockey")) {
            Serial.println(F("Serviço mDNS iniciado em: http://unlockey.local"));
        }
    }
    else {
        Serial.println(F("\nFalha ao conectar ao WiFi!"));
    }
}

void verificarWiFi() {
    static unsigned long ultimaVerificacao = 0;
    unsigned long tempoAtual = millis();
    
    // Verifica a cada 30 segundos ou se houve overflow
    if (tempoAtual < ultimaVerificacao || tempoAtual - ultimaVerificacao > 30000) {
        ultimaVerificacao = tempoAtual;
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(F("WiFi desconectado. Tentando reconectar..."));
            WiFi.reconnect();
            int tentativas = 0;

            while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
                delay(500);
                Serial.print(".");
                tentativas++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println(F("\nWiFi reconectado!"));
                Serial.print("IP: ");
                Serial.println(WiFi.localIP());
            } else {
                Serial.println(F("\nFalha na reconexão WiFi!"));
            }
        }
    }
}

void inicializarServidor() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/enviar", HTTP_POST, handleEnviarMensagem);
    server.begin();
    Serial.println(F("Servidor HTTP iniciado"));
}

void handleRoot() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Erro ao abrir arquivo");
        return;
    }
  
    String html = file.readString();
    file.close();
  
    // Encontrar o ponto de inserção no HTML (depois de <option value="">Selecione o destinatário</option>)
    int pos = html.indexOf("<option value=\"\">Selecione o destinatário</option>");
    if (pos > 0) {
        pos = html.indexOf("</select>", pos);
    
        if (pos > 0) {
            // Adicionar opções de usuários dinamicamente
            String userOptions = "";
            for (int i = 0; i < numUsuarios; i++) {
                userOptions += "<option value=\"" + String(usuarios[i].username) + "\">" + 
                                String(usuarios[i].username) + "</option>\n";
            }
      
            // Inserir as opções antes do fechamento do select
            html = html.substring(0, pos) + userOptions + html.substring(pos);
        }
    }  
    server.send(200, "text/html", html);
}

void handleEnviarMensagem() {
    if (server.method() != HTTP_POST) {
        server.sendHeader("Location", "/");
        server.send(303);
        return;
    }

    webRemetente = server.arg("remetente");
    webDestinatario = server.arg("destinatario");
    webMensagem = server.arg("mensagem");

    if (webRemetente.length() == 0 || webDestinatario.length() == 0 || webMensagem.length() == 0) {
        server.send(400, "text/html", "<html><body><h1>Erro!</h1><p>Todos os campos são obrigatórios</p><p><a href='/'>Voltar</a></p></body></html>");
        return;
    }
    
    if (numMensagens >= MAX_MESSAGES) {
        server.send(503, "text/html", "<html><body><h1>Erro!</h1><p>Sistema de mensagens cheio. Tente novamente mais tarde.</p><p><a href='/'>Voltar</a></p></body></html>");
        return;
    }

    novaMensagemWeb = true;
    File file = SPIFFS.open("/success.html", "r");
    if (!file) {
        server.send(200, "text/html", "<html><body><h1>Mensagem enviada!</h1><p>Redirecionando...</p><script>setTimeout(function(){window.location='/';},3000);</script></body></html>");
        return;
    }

    String html = file.readString();
    file.close();
    server.send(200, "text/html", html);
}

void processarMensagemWeb() {
    if (!novaMensagemWeb || numMensagens >= MAX_MESSAGES) {
        return;
    }
    
    Serial.println(F("\nNova mensagem recebida via web:"));
    
    // Validação de entradas
    if (webRemetente.length() == 0) {
        Serial.println(F("Erro: Remetente não informado!"));
        novaMensagemWeb = false;
        return;
    }
    
    if (webDestinatario.length() == 0) {
        Serial.println(F("Erro: Destinatário não informado!"));
        novaMensagemWeb = false;
        return;
    }
    
    if (webMensagem.length() == 0) {
        Serial.println(F("Erro: Mensagem vazia!"));
        novaMensagemWeb = false;
        return;
    }
    
    Serial.print("De: ");
    Serial.println(webRemetente);
    Serial.print("Para: ");
    Serial.println(webDestinatario);
    
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, webDestinatario.c_str());
    if (idDestinatario == -1) {
        Serial.println(F("Destinatário não encontrado!"));
        novaMensagemWeb = false;
        return;
    }
    // Copiar remetente para a mensagem
    strncpy(mensagens[numMensagens].remetente, webRemetente.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[numMensagens].remetente[MAX_USERNAME_LENGTH - 1] = '\0';
    
    // Copiar destinatário para a mensagem
    strncpy(mensagens[numMensagens].destinatario, webDestinatario.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[numMensagens].destinatario[MAX_USERNAME_LENGTH - 1] = '\0';

    if (webMensagem.length() > MAX_MESSAGE_LENGTH) {
        webMensagem = webMensagem.substring(0, MAX_MESSAGE_LENGTH);
    }

    mensagens[numMensagens].tamanhoMsg = webMensagem.length();
    SetDataAtual(mensagens[numMensagens].data);
    EncriptaMensagem((unsigned char*)webMensagem.c_str(), 
                    mensagens[numMensagens].mensagemCriptografada, 
                    usuarios[idDestinatario].chavePublica);
    
    numMensagens++;
    Serial.println(F("Mensagem processada com sucesso!"));
    
    novaMensagemWeb = false;
}

void mostrarStatusSistema() {
    Serial.println(F("\n===== STATUS DO SISTEMA ====="));
    Serial.print("Usuários: ");
    Serial.print(numUsuarios);
    Serial.print("/");
    Serial.println(F(MAX_USERS));
    
    Serial.print("Mensagens: ");
    Serial.print(numMensagens);
    Serial.print("/");
    Serial.println(F(MAX_MESSAGES));

    Serial.print("Memória livre: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi: Conectado (");
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm)"));
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
    Serial.begin(115200);
    delay(1000);

    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    Serial.println(F("\n"));
    Serial.println(F("██╗░░░██╗███╗░░██╗██╗░░░░░░█████╗░░█████╗░██╗░░██╗███████╗██╗░░░██╗"));
    Serial.println(F("██║░░░██║████╗░██║██║░░░░░██╔══██╗██╔══██╗██║░██╔╝██╔════╝╚██╗░██╔╝"));
    Serial.println(F("██║░░░██║██╔██╗██║██║░░░░░██║░░██║██║░░╚═╝█████═╝░█████╗░░░╚████╔╝░"));
    Serial.println(F("██║░░░██║██║╚████║██║░░░░░██║░░██║██║░░██╗██╔═██╗░██╔══╝░░░░╚██╔╝░░"));
    Serial.println(F("╚██████╔╝██║░╚███║███████╗╚█████╔╝╚█████╔╝██║░╚██╗███████╗░░░██║░░░"));
    Serial.println(F("░╚═════╝░╚═╝░░╚══╝╚══════╝░╚════╝░░╚════╝░╚═╝░░╚═╝╚══════╝░░░╚═╝░░░"));
    Serial.println(F("\n       Sistema de Mensagens Criptografadas em RSA\n"));
    inicializarSPIFFS();
    verificarArquivosNecessarios();
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    

    // Initialize MFRC522
    Serial.println(F("Iniciando MFRC522..."));
    mfrc522.PCD_Init(SS_PIN, RST_PIN);

    Serial.println(F("Iniciando servidor web..."));
    inicializarWiFi();

    if (WiFi.status() == WL_CONNECTED) {
        inicializarServidor();
        Serial.println("     🌐 Acesse a interface web: http://" + WiFi.localIP().toString());
    }
    Serial.println(F("===================================\n"));
}

void loop() {
    verificarWiFi();

    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    }

    if (novaMensagemWeb) {
        processarMensagemWeb();
    }

    Serial.println(F("\n=== Menu Principal ==="));
    Serial.println(F("1 - Cadastrar usuário"));
    Serial.println(F("2 - Enviar mensagem"));
    Serial.println(F("3 - Ler mensagens"));
    Serial.println(F("9 - Status do sistema")); 
    Serial.println(F("0 - Sair"));
    Serial.println(F("=================="));
    Serial.print("Opção: ");

    // Aguardar entrada mantendo o servidor web ativo
    unsigned long startTime = millis();
    while(!Serial.available() && millis() - startTime < 20000) { // 20 segundos
        if (WiFi.status() == WL_CONNECTED) {
            server.handleClient();
        }
        if (novaMensagemWeb) {
            processarMensagemWeb();
        } 
        delay(10);
    }

    if (!Serial.available()) {
        return;
    }

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

        case 9:
            mostrarStatusSistema();
            break;
            
        case 0:
            Serial.println(F("Limpando recursos..."));
            for (int i = 0; i < numUsuarios; i++) {
                DeleteKeys(NULL, usuarios[i].chavePublica);
            }
            Serial.println(F("Programa finalizado."));
            ESP.restart();
            break;
            
        default:
            Serial.println(F("Opção inválida!"));
            break;
    }
}