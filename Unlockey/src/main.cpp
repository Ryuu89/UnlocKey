#include <Arduino.h>
#include "rsa.h"
#include <SPI.h>
#include <MFRC522.h>

#define MAX_USERS 5
#define MAX_MESSAGES 10
#define MAX_MESSAGE_LENGTH 50
#define MAX_USERNAME_LENGTH 15

#define SS_PIN 10
#define RST_PIN 9

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

void limpaBufferSerial() {
    while(Serial.available() > 0) {
        Serial.read();
    }
}

String lerString() {
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

void setDataAtual(char* data) {
    strcpy(data, "15/03/2024 10:30");
}

void CadastraUsuario(Usuario *usuario) {
    PrivateKeys *chavePrivada;
    Serial.println("Digite o nome do usuário: ");
    limpaBufferSerial();
    String nome = lerString();
    strncpy(usuario->username, nome.c_str(), MAX_USERNAME_LENGTH - 1);
    usuario->username[MAX_USERNAME_LENGTH - 1] = '\0';
    
    InitKeys(&chavePrivada, &usuario->chavePublica);
    GeraChaves(chavePrivada, usuario->chavePublica);
    
    Serial.println("\nChaves geradas para " + String(usuario->username) + ":");
    MostraChaves(chavePrivada, usuario->chavePublica);
    Serial.println("\nIMPORTANTE: Aproxime a TAG RFID para salvar as chaves privadas!");
    SalvarChaves(chavePrivada, NULL, &mfrc522);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
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
    limpaBufferSerial();
    String remetente = lerString();
    
    Serial.println("Destinatário: ");
    limpaBufferSerial();
    String destinatario = lerString();
    
    int idRemetente = EncontraUsuario(usuarios, numUsuarios, remetente.c_str());
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, destinatario.c_str());
    
    if (idRemetente == -1 || idDestinatario == -1) {
        Serial.println("Usuário não encontrado!");
        return;
    }
    
    Serial.println("Mensagem: ");
    limpaBufferSerial();
    String mensagem = lerString();
    
    strncpy(mensagens[*numMensagens].remetente, remetente.c_str(), MAX_USERNAME_LENGTH - 1);
    strncpy(mensagens[*numMensagens].destinatario, destinatario.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[*numMensagens].tamanhoMsg = mensagem.length();
    
    setDataAtual(mensagens[*numMensagens].data);
    
    EncriptaMensagem((unsigned char*)mensagem.c_str(), 
                     mensagens[*numMensagens].mensagemCriptografada, 
                     usuarios[idDestinatario].chavePublica);
    
    (*numMensagens)++;
    Serial.println("Mensagem enviada com sucesso!");
}

void LerMensagens(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int numMensagens) {
    Serial.println("\nDigite seu username: ");
    limpaBufferSerial();
    String username = lerString();
    
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
    limpaBufferSerial();
    if (Serial.parseInt() == 1) {
        
        PrivateKeys *ChavePrivada;
        InitKeys(&ChavePrivada, NULL);
        
        Serial.println("Aproxime o cartão RFID para ler a chave privada...");
        while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
            delay(50);
        }

        LerChavesPrivadas(ChavePrivada, &mfrc522);
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        
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
        DeleteKeys(ChavePrivada, NULL);
    }
}

void setup() {
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
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
    limpaBufferSerial();
    
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