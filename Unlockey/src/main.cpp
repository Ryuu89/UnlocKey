#include <Arduino.h>
#include "rsa.h"
#include <SPI.h>
#include <MFRC522.h>

#define MAX_USERS 5
#define MAX_MESSAGES 10
#define MAX_MESSAGE_LENGTH 50
#define MAX_USERNAME_LENGTH 15

#define SS_PIN  5
#define RST_PIN 22
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

#define DEBUG true

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
    strcpy(data, "15/03/2024 10:30");
}

int AguardaLeituraRFID() {
    Serial.println("Aguardando cartão RFID...");
    unsigned long tempoInicio = millis();
    const unsigned long TIMEOUT = 10000; // 10 segundos de timeout
    
    while (millis() - tempoInicio < TIMEOUT) {
        if (!mfrc522.PICC_IsNewCardPresent()) {
            delay(50);
            continue;
        }
        Serial.println("Cartão detectado!");

        if (!mfrc522.PICC_ReadCardSerial()) {
            Serial.println("Falha na leitura do cartão");
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
    Serial.println("Tempo esgotado! Nenhum cartão detectado.");
    return 0;
}

void CadastraUsuario(Usuario *usuario) {
    PrivateKeys *chavePrivada;
    Serial.println("Digite o nome do usuário: ");
    LimpaBufferSerial();
    String nome = LerString();

    if (nome.length() == 0) {
        Serial.println("Nome não pode ser vazio!");
        return;
    }

    strncpy(usuario->username, nome.c_str(), MAX_USERNAME_LENGTH - 1);
    usuario->username[MAX_USERNAME_LENGTH - 1] = '\0';
    
    InitKeys(&chavePrivada, &usuario->chavePublica);
    GeraChaves(chavePrivada, usuario->chavePublica);
    
    Serial.println("\nChaves geradas para " + String(usuario->username) + ":");
    MostraChaves(chavePrivada, usuario->chavePublica);
    Serial.println("\nIMPORTANTE: Aproxime a TAG RFID para salvar as chaves privadas!");

    if (AguardaLeituraRFID()) {
        SalvarChaves(chavePrivada, NULL, &mfrc522);
        Serial.println("Chaves salvas com sucesso!");
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
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
    Serial.println("Remetente: ");
    LimpaBufferSerial();
    String remetente = LerString();
    
    if (remetente.length() == 0) {
        Serial.println("Remetente não pode ser vazio!");
        return;
    }
    
    Serial.println("Destinatário: ");
    LimpaBufferSerial();
    String destinatario = LerString();
    
    int idRemetente = EncontraUsuario(usuarios, numUsuarios, remetente.c_str());
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, destinatario.c_str());
    
    if (idRemetente == -1 || idDestinatario == -1) {
        Serial.println("Usuário não encontrado!");
        return;
    }
    
    Serial.println("Mensagem: ");
    LimpaBufferSerial();
    String mensagem = LerString();
    
    strncpy(mensagens[*numMensagens].remetente, remetente.c_str(), MAX_USERNAME_LENGTH - 1);
    strncpy(mensagens[*numMensagens].destinatario, destinatario.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[*numMensagens].tamanhoMsg = mensagem.length();
    
    SetDataAtual(mensagens[*numMensagens].data);
    
    EncriptaMensagem((unsigned char*)mensagem.c_str(), 
                     mensagens[*numMensagens].mensagemCriptografada, 
                     usuarios[idDestinatario].chavePublica);
    
    (*numMensagens)++;
    Serial.println("Mensagem enviada com sucesso!");
}

void LerMensagens(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int numMensagens) {
    Serial.println("\nDigite seu username: ");
    LimpaBufferSerial();
    String username = LerString();
    
    int idUsuario = EncontraUsuario(usuarios, numUsuarios, username.c_str());
    if (idUsuario == -1) {
        Serial.println("Usuário não encontrado!");
        return;
    }
    
    bool temMensagem = false;
    for (int i = 0; i < numMensagens; i++) {
        if (strcmp(mensagens[i].destinatario, username.c_str()) == 0) {
            temMensagem = true;
            Serial.print("\nDe: ");
            Serial.print(mensagens[i].remetente);
            Serial.print("[");
            Serial.print(mensagens[i].data);
            Serial.print("]");
            Serial.println("Mensagem criptografada: ");
            
            for (unsigned int j = 0; j < mensagens[i].tamanhoMsg; j++) {
                Serial.write((char)(mensagens[i].mensagemCriptografada[j] % 94 + 33));
            }
            Serial.println();
        }
    }
    
    if (!temMensagem) {
        Serial.println("Nenhuma mensagem encontrada.");
        return;
    }

    Serial.println("\nDeseja descriptografar as mensagens? (1-Sim/0-Não)");
    LimpaBufferSerial();
    if (Serial.parseInt() == 1) {
        PrivateKeys *ChavePrivada;
        InitKeys(&ChavePrivada, NULL);
        
        Serial.println("Aproxime o cartão RFID para ler a chave privada...");
        if (AguardaLeituraRFID()) {
            LerChavesPrivadas(ChavePrivada, &mfrc522);
            Serial.println("\nMensagens descriptografadas:");
            for (int i = 0; i < numMensagens; i++) {
                if (strcmp(mensagens[i].destinatario, username.c_str()) == 0) {
                    unsigned char mensagemDecriptada[MAX_MESSAGE_LENGTH] = {0};
                    DecriptaMensagem(mensagens[i].mensagemCriptografada, 
                                mensagens[i].tamanhoMsg,
                                mensagemDecriptada, 
                                ChavePrivada);
                    
                    Serial.print("[");
                    Serial.print(mensagens[i].data);
                    Serial.print("] De: ");
                    Serial.print(mensagens[i].remetente);
                    Serial.print(" - ");
                    Serial.println((char*)mensagemDecriptada);
                }
            }
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
        }
        DeleteKeys(ChavePrivada, NULL);
    }
}

void setup() {
    Serial.begin(115200);
    while(!Serial) delay(10);
    delay(1000);

    Serial.println("\n\n=== Iniciando Sistema RFID ===");

    // Pin Configuration
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT_PULLUP); // Enable pull-up on MISO
    pinMode(SCK_PIN, OUTPUT);
    pinMode(SS_PIN, OUTPUT);
    pinMode(RST_PIN, OUTPUT);

    // Hardware Reset Sequence
    Serial.println("Realizando reset do hardware...");
    digitalWrite(RST_PIN, LOW);  // Assert reset
    delay(100);
    digitalWrite(RST_PIN, HIGH); // Release reset
    delay(100);

    // Initialize SPI
    Serial.println("Iniciando SPI...");
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));

    // Initialize MFRC522
    Serial.println("Iniciando MFRC522...");
    mfrc522.PCD_Init();
    delay(50);

    // Read Version Register (Verify MFRC522 is responding)
    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

    if (version == 0x00 || version == 0xFF) {
        Serial.println("\nERRO CRÍTICO: MFRC522 não respondeu!");
        Serial.println("Possíveis causas:");
        Serial.println("1. Conexões incorretas (verifique MOSI, MISO, SCK, SS)");
        Serial.println("2. Módulo MFRC522 com defeito");
        Serial.println("3. Problemas de alimentação (tensão instável)");
        while (1) {
            delay(5000);
        }
    }

    // Configure RFID settings
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    mfrc522.PCD_WriteRegister(mfrc522.TxControlReg, 0x83);

    Serial.println("\nSistema RFID inicializado com sucesso!");
    Serial.printf("Versão do chip: 0x%02X\n", version);
    Serial.println("===================================\n");
}

void loop() {
    Serial.println("\n=== Menu Principal ===");
    Serial.println("1 - Cadastrar usuário");
    Serial.println("2 - Enviar mensagem");
    Serial.println("3 - Ler mensagens");
    Serial.println("0 - Sair");
    Serial.println("==================");
    Serial.print("Opção: ");
    
    while(!Serial.available()) {
        delay(10);
    }
    int opcao = Serial.parseInt();
    LimpaBufferSerial();
    
    switch(opcao) {
        case 1:
            if (numUsuarios < MAX_USERS) {
                CadastraUsuario(&usuarios[numUsuarios++]);
            } else {
                Serial.println("Limite de usuários atingido!");
            }
            break;
            
        case 2:
            if (numMensagens < MAX_MESSAGES) {
                EnviarMensagem(usuarios, numUsuarios, mensagens, &numMensagens);
            } else {
                Serial.println("Limite de mensagens atingido!");
            }
            break;
            
        case 3:
            LerMensagens(usuarios, numUsuarios, mensagens, numMensagens);
            break;
            
        case 0:
            Serial.println("Limpando recursos...");
            for (int i = 0; i < numUsuarios; i++) {
                DeleteKeys(NULL, usuarios[i].chavePublica);
            }
            Serial.println("Programa finalizado.");
            ESP.restart();
            break;
            
        default:
            Serial.println("Opção inválida!");
            break;
    }
}