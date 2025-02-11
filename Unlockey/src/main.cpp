#include <Arduino.h>
#include "rsa.h"

#define MAX_USERS 5
#define MAX_MESSAGES 10
#define MAX_MESSAGE_LENGTH 100
#define MAX_USERNAME_LENGTH 20

typedef struct {
    char remetente[MAX_USERNAME_LENGTH];
    char destinatario[MAX_USERNAME_LENGTH];
    unsigned long mensagemCriptografada[MAX_MESSAGE_LENGTH];
    unsigned int tamanhoMsg;
} Mensagem;

typedef struct {
    char username[MAX_USERNAME_LENGTH];
    PrivateKeys *chavePrivada;
    PublicKeys *chavePublica;
} Usuario;

Usuario usuarios[MAX_USERS];
Mensagem mensagens[MAX_MESSAGES];
int numUsuarios = 0, numMensagens = 0;

void CadastraUsuario(Usuario *usuario);
int EncontraUsuario(Usuario *usuarios, int numUsuarios, const char *username);
void EnviarMensagem(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int *numMensagens);
void LerMensagens(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int numMensagens);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Aguarda a conexão da porta serial
    }
}

void loop() {
    int opcao;
    Serial.println("\n1 - Cadastrar usuário");
    Serial.println("2 - Enviar mensagem");
    Serial.println("3 - Ler mensagens");
    Serial.println("0 - Sair");
    Serial.print("Opção: ");
    
    while (Serial.available() == 0) {
        // Aguarda a entrada do usuário
    }
    opcao = Serial.parseInt();
    
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
            // Cleanup
            for (int i = 0; i < numUsuarios; i++) {
                DeleteKeys(usuarios[i].chavePrivada, usuarios[i].chavePublica);
            }
            Serial.println("Saindo...");
            while (true) {
                // Para o loop
            }
            break;
            
        default:
            Serial.println("Opção inválida!");
            break;
    }
}

void CadastraUsuario(Usuario *usuario) {
    Serial.print("Digite o nome do usuário: ");
    while (Serial.available() == 0) {
        // Aguarda a entrada do usuário
    }
    Serial.readBytesUntil('\n', usuario->username, MAX_USERNAME_LENGTH);
    usuario->username[strcspn(usuario->username, "\n")] = '\0';
    
    InitKeys(&usuario->chavePrivada, &usuario->chavePublica);
    GeraChaves(usuario->chavePrivada, usuario->chavePublica);
    
    Serial.print("\nChaves geradas para ");
    Serial.print(usuario->username);
    Serial.println(":");
    MostraChaves(usuario->chavePrivada, usuario->chavePublica);
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
    char remetente[MAX_USERNAME_LENGTH];
    char destinatario[MAX_USERNAME_LENGTH];
    unsigned char mensagem[MAX_MESSAGE_LENGTH];
    
    Serial.print("Remetente: ");
    while (Serial.available() == 0) {
        // Aguarda a entrada do usuário
    }
    Serial.readBytesUntil('\n', remetente, MAX_USERNAME_LENGTH);
    remetente[strcspn(remetente, "\n")] = '\0';
    
    Serial.print("Destinatário: ");
    while (Serial.available() == 0) {
        // Aguarda a entrada do usuário
    }
    Serial.readBytesUntil('\n', destinatario, MAX_USERNAME_LENGTH);
    destinatario[strcspn(destinatario, "\n")] = '\0';
    
    int idRemetente = EncontraUsuario(usuarios, numUsuarios, remetente);
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, destinatario);
    
    if (idRemetente == -1 || idDestinatario == -1) {
        Serial.println("Usuário não encontrado!");
        return;
    }
    
    Serial.print("Mensagem: ");
    while (Serial.available() == 0) {
        // Aguarda a entrada do usuário
    }
    Serial.readBytesUntil('\n', (char*)mensagem, MAX_MESSAGE_LENGTH);
    mensagem[strcspn((char*)mensagem, "\n")] = '\0';
    
    strcpy(mensagens[*numMensagens].remetente, remetente);
    strcpy(mensagens[*numMensagens].destinatario, destinatario);
    mensagens[*numMensagens].tamanhoMsg = strlen((char*)mensagem);
    
    EncriptaMensagem(mensagem, mensagens[*numMensagens].mensagemCriptografada, 
                     usuarios[idDestinatario].chavePublica);
    
    (*numMensagens)++;
}

void LerMensagens(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int numMensagens) {
    char username[MAX_USERNAME_LENGTH];

    Serial.print("\nDigite seu username para ler as mensagens: ");
    while (Serial.available() == 0) {
        // Aguarda a entrada do usuário
    }
    Serial.readBytesUntil('\n', username, MAX_USERNAME_LENGTH);
    username[strcspn(username, "\n")] = '\0';
    
    int idUsuario = EncontraUsuario(usuarios, numUsuarios, username);
    if (idUsuario == -1) {
        Serial.println("Usuário não encontrado!");
        return;
    }
    
    Serial.print("\nMensagens para ");
    Serial.print(username);
    Serial.println(":");
    for (int i = 0; i < numMensagens; i++) {
        if (strcmp(mensagens[i].destinatario, username) == 0) {
            unsigned char mensagemDecriptada[MAX_MESSAGE_LENGTH];
            
            Serial.print("\nDe: ");
            Serial.print(mensagens[i].remetente);
            Serial.print("\n\t");
            for (unsigned int j = 0; j < mensagens[i].tamanhoMsg; j++){
                Serial.print((char)(mensagens[i].mensagemCriptografada[j] % 94 + 33)); // Converte para caracteres imprimíveis
            }

            Serial.println("\n\n1 - Digitar chaves manualmente para descriptografar.");
            Serial.println("2 - Ler chaves de um arquivo.txt para descriptografar.");
            Serial.println("0 - Sair.");

            int opcao;
            Serial.print("Opção: ");
            while (Serial.available() == 0) {
                // Aguarda a entrada do usuário
            }
            opcao = Serial.parseInt();
            PrivateKeys *ChavePrivada;
            InitKeys(&ChavePrivada, NULL);

            if (opcao == 1){
                LerChavesPrivadas(ChavePrivada, Serial);
            }
            else if (opcao == 2){
                // No Arduino, leitura de arquivo não é trivial, então omitimos essa parte
                Serial.println("Leitura de arquivo não suportada no Arduino.");
                DeleteKeys(ChavePrivada, NULL);
                return;
            }
            else{
                Serial.println("Saindo...");
                DeleteKeys(ChavePrivada, NULL);
                return;
            }

            DecriptaMensagem(mensagens[i].mensagemCriptografada, 
                           mensagens[i].tamanhoMsg,
                           mensagemDecriptada, 
                           ChavePrivada);
            
            Serial.print("Mensagem decriptada: ");
            Serial.println((char*)mensagemDecriptada);
            DeleteKeys(ChavePrivada, NULL);
        }
    }
}