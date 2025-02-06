#include "rsa.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct PublicKeys{
  unsigned long e; // Expoente público
  unsigned long n; // Produto p*q
};

struct PrivateKeys{
  unsigned long d; // Expoente privado
  unsigned long p; // Chave privada primo1
  unsigned long q; // Chave privada primo2
};

// Verifica se um número é primo
int EhPrimo(unsigned long num) {
    if (num < 2) return 0;
    unsigned long limit = sqrt(num);
    for (unsigned long i = 2; i <= limit; i++) {
        if (num % i == 0){
          //printf("%lu divisivel por %lu\n", num, i);
          return 0;
        }
    }
    return 1;
}

// Gera dois números primos aleatórios
void GeraPrimos(unsigned long *p, unsigned long *q) {
    srand(time(NULL));
    do {
        *p = rand() % 100 + 100; // Gera números maiores (intervalo de 100 a 200)
    } while (!EhPrimo(*p));

    do {
        *q = rand() % 100 + 200;
    } while (!EhPrimo(*q) || *q == *p);
}

// Calcula o Máximo Divisor Comum (MDC)
unsigned long MDC(unsigned long a, unsigned long b) {
    while (b != 0) {
        unsigned long temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Calcula o inverso modular
unsigned long InversoModular(unsigned long e, unsigned long totient) {
    long t_anterior = 0;
    long t_atual = 1;
    long resto_anterior = totient;
    long resto_atual = e;

    while (resto_atual != 0) {
        /*   // algoritmo de Euclides estendido
        20 em binário = 10100

        7^20 = 7^(16 + 4) = 7^16 * 7^4

        Iterações:
        1. base = 7, exp = 20
        2. base = 7^2 = 49 ≡ 10 (mod 13), exp = 10
        3. base = 10^2 = 100 ≡ 9 (mod 13), exp = 5
        4. base = 9^2 = 81 ≡ 3 (mod 13), exp = 2
        5. base = 3^2 = 9 (mod 13), exp = 1 */

        long quociente = resto_anterior / resto_atual;

        // Atualiza coeficientes
        long temp = t_anterior;
        t_anterior = t_atual;
        t_atual = temp - quociente * t_atual;

        // Atualiza restos
        temp = resto_anterior;
        resto_anterior = resto_atual;
        resto_atual = temp - (quociente * resto_atual);
    }

    // Verifica se existe inverso modular
    if (resto_anterior > 1) {
        return 0;  // e não é coprimo com totient
    }

    // Garante que o resultado seja positivo
    if (t_anterior < 0) {
        t_anterior += totient;
    }

    return (unsigned long)t_anterior;
}

// Calcula (base^exp) % mod de forma eficiente
unsigned long ExpModular(unsigned long base, unsigned long exp, unsigned long mod){
    unsigned long result = 1;
    base = base % mod; // Normaliza a base
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }
        exp = exp / 2;
        base = (base * base) % mod;
    }
    return result;
}

// Gerenciamento de Memoria
void InitKeys(PrivateKeys** priv, PublicKeys** pub) {
    if (priv){
        *priv = (struct PrivateKeys*) malloc(sizeof(PrivateKeys));
        if (*priv) {
            (*priv)->d = 0;
            (*priv)->p = 0;
            (*priv)->q = 0;
        }
    }
    
    if (pub){
        *pub = (PublicKeys*) malloc(sizeof(PublicKeys));
        if (*pub) {
            (*pub)->e = 0;
            (*pub)->n = 0;
        }
    }    
}

void DeleteKeys(PrivateKeys* priv, PublicKeys* pub) {
    if (priv != NULL){
        free(priv);
    }
    if (pub != NULL){
        free(pub);
    }
}

// Gera as chaves RSA
void GeraChaves(PrivateKeys *pvkeys, PublicKeys *pbkeys) {
    GeraPrimos(&pvkeys->p, &pvkeys->q);

    pbkeys->n = pvkeys->p * pvkeys->q;

    unsigned long totient = (pvkeys->p - 1) * (pvkeys->q - 1);

    // Escolhe 'e' como um número pequeno e coprimo com totient
    pbkeys->e = 21;
    while (MDC(pbkeys->e, totient) != 1) {
        pbkeys->e += 2;
    }

    // Calcula 'd', o inverso modular de e
    pvkeys->d = InversoModular(pbkeys->e, totient);

    if (pvkeys->d == 0) {
        printf("Erro ao calcular o inverso modular. Gere novas chaves.\n");
        exit(1);
    }
}

void MostraChaves(const PrivateKeys* priv, const PublicKeys* pub){
    printf("\nChave Pública:\n");
    printf("\te = %lu | ", pub->e);
    printf("n = %lu\n", pub->n);
    printf("Chave Privada:\n");
    printf("\tp = %lu | ", priv->p);
    printf("q = %lu | ", priv->q);
    printf("d = %lu\n", priv->d);
}

void SalvarChaves(PrivateKeys *priv, PublicKeys *pub, FILE *fp){
    if (fp == NULL){
        printf("Arquivo invalido.\n");
        return;
    }
    if (priv != NULL){
        fprintf(fp, "%lu %lu %lu ", priv->p, priv->q, priv->d);
    }
    if (pub != NULL){
        fprintf(fp, "%lu %lu", pub->e, pub->n);
    }
}

void LerChavesPrivadas(PrivateKeys *priv, FILE *fp){
    if (fp == NULL){
        printf("Digite as chaves p, q, d, respectivamente: ");
        scanf("%lu %lu %lu", &priv->p, &priv->q, &priv->d);
        scanf("%*c");
    }
    else{
        fscanf(fp, "%lu %lu %lu", &priv->p, &priv->q, &priv->d);
    }
}

// Criptografa uma mensagem caractere por caractere
int EncriptaMensagem(const unsigned char *message, unsigned long *encrypted, const PublicKeys *pbkeys) {
    if (!message || !encrypted || !pbkeys){
        return -1;
    }

    unsigned int length = strlen((char*)message);
    for (unsigned int i = 0; i < length; i++) {
        encrypted[i] = ExpModular((unsigned long)message[i], pbkeys->e, pbkeys->n);
    }

    return length;
}

// Descriptografa a mensagem criptografada
int DecriptaMensagem(const unsigned long *encrypted, unsigned int length, unsigned char *message, const PrivateKeys *pvkeys) {
    if (!message || !encrypted || !pvkeys){
        return -1;
    }

    for (unsigned int i = 0; i < length; i++) {
        // Usa as chaves para descriptografar (d e n)
        message[i] = (unsigned char)ExpModular(encrypted[i], pvkeys->d, (pvkeys->p * pvkeys->q));
    }
    message[length] = '\0'; // Adiciona o terminador nulo

    return 0;
}