#include "rsa.h"
#include <stdio.h>

void CadastraUsuario(PrivateKeys *PrivadaTemp, PublicKeys *PublicaTemp);

int main(){
    PrivateKeys PrivadaTemp[10];
    PublicKeys PublicaTemp[10];

    int qnt;
    printf("Digite quantos usuarios deseja cadastrar, limite de 10 usuários: ");
    scanf("%d", &qnt);
    scanf("%*c");

    for (int i = 0; i < qnt; i++){
        printf("\nUsuário número %d.\n", i+1);
        CadastraUsuario(&PrivadaTemp[i], &PublicaTemp[i]);

        printf("\nFinalizado usuario n° %d...\n", i+1);
    }
    printf("\n\n\n\tPrograma finalizado!\n");

    return 0;
}

void CadastraUsuario(PrivateKeys *PrivadaTemp, PublicKeys *PublicaTemp){
    GeraChaves(PrivadaTemp, PublicaTemp);

    printf("Chaves RSA:\n");
    printf("p: %lu, q: %lu\n", PrivadaTemp->p, PrivadaTemp->q);
    printf("e: %lu, d: %lu, n: %lu\n", PublicaTemp->e, PrivadaTemp->d, PublicaTemp->n);

    char mensagem[256];
    printf("\nDigite a mensagem para criptografar: ");
    fgets(mensagem, sizeof(mensagem), stdin);
    mensagem[strcspn(mensagem, "\n")] = '\0'; // Remove o '\n'

    size_t length = strlen(mensagem);
    unsigned long mensagemCriptada[length];
    char mensagemDecriptada[256];

    EncriptaMensagem(mensagem, mensagemCriptada, PublicaTemp);

    printf("\n--------------=--------------\n\n");
    printf("Mensagem criptografada em valores numéricos:\n\n\t");
    for (size_t i = 0; i < length; i++)
    {
        printf("%lu ", mensagemCriptada[i]);
    }

    printf("\n\n--------------=--------------\n\n");
    printf("Mensagem criptografada como caracteres:\n\n\t");
    for (size_t i = 0; i < length; i++)
    {
        printf("%c", (char)(mensagemCriptada[i] % 94 + 33)); // Converte para caracteres imprimíveis
    }
    printf("\n\n--------------=--------------\n\n");

    DecriptaMensagem(mensagemCriptada, length, mensagemDecriptada, PrivadaTemp);
    printf("Mensagem descriptografada:\n\n\t %s\n", mensagemDecriptada);
}