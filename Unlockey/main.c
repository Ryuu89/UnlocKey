#include "rsa.h"
#include <stdio.h>
#include <string.h>

#define MAX_USERS 10
#define MAX_MESSAGES 50
#define MAX_MESSAGE_LENGTH 256
#define MAX_USERNAME_LENGTH 32

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

void CadastraUsuario(Usuario *usuario);
int EncontraUsuario(Usuario *usuarios, int numUsuarios, const char *username);
void EnviarMensagem(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int *numMensagens);
void LerMensagens(Usuario *usuarios, int numUsuarios, Mensagem *mensagens, int numMensagens);

int main() {
    Usuario usuarios[MAX_USERS];
    Mensagem mensagens[MAX_MESSAGES];
    int numUsuarios = 0, numMensagens = 0;
    int opcao;
    
    do {
        printf("\n1 - Cadastrar usuário\n");
        printf("2 - Enviar mensagem\n");
        printf("3 - Ler mensagens\n");
        printf("0 - Sair\n");
        printf("Opção: ");
        scanf("%d", &opcao);
        scanf("%*c");  // Limpa o buffer
        
        switch(opcao) {
            case 1:
                if (numUsuarios < MAX_USERS) {
                    CadastraUsuario(&usuarios[numUsuarios++]);
                } else {
                    printf("Limite de usuários atingido!\n");
                }
                break;
                
            case 2:
                if (numMensagens < MAX_MESSAGES) {
                    EnviarMensagem(usuarios, numUsuarios, mensagens, &numMensagens);
                } else {
                    printf("Limite de mensagens atingido!\n");
                }
                break;
                
            case 3:
                LerMensagens(usuarios, numUsuarios, mensagens, numMensagens);
                break;
        }
    } while (opcao != 0);
    
    // Cleanup
    for (int i = 0; i < numUsuarios; i++) {
        DeleteKeys(usuarios[i].chavePrivada, usuarios[i].chavePublica);
    }
    
    return 0;
}

void CadastraUsuario(Usuario *usuario) {
    char opcao;

    printf("Digite o nome do usuário: ");
    fgets(usuario->username, MAX_USERNAME_LENGTH, stdin);
    usuario->username[strcspn(usuario->username, "\n")] = '\0';
    
    InitKeys(&usuario->chavePrivada, &usuario->chavePublica);
    GeraChaves(usuario->chavePrivada, usuario->chavePublica);
    
    printf("\nChaves geradas para %s:\n", usuario->username);
    MostraChaves(usuario->chavePrivada, usuario->chavePublica);

    printf("\nDeseja salvar chaves privadas em um arquivo? (S/N) : ");
    scanf("%c", &opcao);
    scanf("%*c");  // Limpa o buffer

    if (opcao == 'S' || opcao == 's'){
        // Deve criar um arquivo em /workspaces/UnlocKey/UnlockR v1/Debug/Keys
        FILE *fp;
        char path[128] = "./Debug/Keys/";

        strcat(path, usuario->username);
        strcat(path, "PrivateKey.txt");
        fp = fopen(path, "w");
        if (fp != NULL) {
            SalvarChaves(usuario->chavePrivada, NULL, fp);
        }

        fclose(fp);
    }
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
    
    printf("Remetente: ");
    fgets(remetente, MAX_USERNAME_LENGTH, stdin);
    remetente[strcspn(remetente, "\n")] = '\0';
    
    printf("Destinatário: ");
    fgets(destinatario, MAX_USERNAME_LENGTH, stdin);
    destinatario[strcspn(destinatario, "\n")] = '\0';
    
    int idRemetente = EncontraUsuario(usuarios, numUsuarios, remetente);
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, destinatario);
    
    if (idRemetente == -1 || idDestinatario == -1) {
        printf("Usuário não encontrado!\n");
        return;
    }
    
    printf("Mensagem: ");
    fgets((char*)mensagem, MAX_MESSAGE_LENGTH, stdin);
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

    printf("\nDigite seu username para ler as mensagens: ");
    fgets(username, MAX_USERNAME_LENGTH, stdin);
    username[strcspn(username, "\n")] = '\0';
    
    int idUsuario = EncontraUsuario(usuarios, numUsuarios, username);
    if (idUsuario == -1) {
        printf("Usuário não encontrado!\n");
        return;
    }
    
    printf("\nMensagens para %s:\n", username);
    for (int i = 0; i < numMensagens; i++) {
        if (strcmp(mensagens[i].destinatario, username) == 0) {
            unsigned char mensagemDecriptada[MAX_MESSAGE_LENGTH];
            
            printf("\nDe: %s\n\t", mensagens[i].remetente);
            for (int j = 0; j < mensagens[i].tamanhoMsg; j++){
                printf("%c", (char)(mensagens[i].mensagemCriptografada[j] % 94 + 33)); // Converte para caracteres imprimíveis
            }

            printf("\n\n1 - Digitar chaves manualmente para descriptografar.\n");
            printf("2 - Ler chaves de um arquivo.txt para descriptografar.\n");
            printf("0 - Sair.\n");

            int opcao;
            printf("Opção: ");
            scanf("%d", &opcao);
            scanf("%*c");
            PrivateKeys *ChavePrivada;
            InitKeys(&ChavePrivada, NULL);

            if (opcao == 1){
                LerChavesPrivadas(ChavePrivada, NULL);
            }
            else if (opcao == 2){
                FILE *fp;
                char path[128] = "./Debug/Keys/";
                char nomeArquivo[128];

                printf("\nDigite o nome do arquivo: ");
                fgets(nomeArquivo, sizeof(nomeArquivo), stdin);
                nomeArquivo[strcspn(nomeArquivo, "\n")] = '\0'; // Remove o '\n'
                strcat(path, nomeArquivo);
                strcat(path, ".txt");
                
                fp = fopen(path, "r");
                if (fp == NULL) {
                    printf("Erro ao abrir o arquivo %s.\n", path);
                    DeleteKeys(ChavePrivada, NULL);
                    return;
                }
                
                LerChavesPrivadas(ChavePrivada, fp);
                fclose(fp);
            }
            else{
                printf("Saindo...\n");
                DeleteKeys(ChavePrivada, NULL);
                return;
            }

            DecriptaMensagem(mensagens[i].mensagemCriptografada, 
                           mensagens[i].tamanhoMsg,
                           mensagemDecriptada, 
                           ChavePrivada);
            
            printf("Mensagem decriptada: %s\n", mensagemDecriptada);
            DeleteKeys(ChavePrivada, NULL);
        }
    }
}