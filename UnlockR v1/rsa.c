#include "rsa.h"

// Verifica se um número é primo
int EhPrimo(unsigned long num) {
    if (num < 2) return 0;
    for (unsigned long i = 2; i <= sqrt(num); i++) {
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
        *q = rand() % 100 + 400;
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
    long t = 0, newt = 1;
    long r = totient, newr = e;

    while (newr != 0) {   // algoritmo de Euclides estendido
        long quotient = r / newr;
        long temp = t;
        t = newt;
        newt = temp - quotient * newt;

        temp = r;
        r = newr;
        newr = temp - quotient * newr;
    }

    if (r > 1) return 0; // Não há inverso modular
    if (t < 0) t += totient;
    return (unsigned long)t;
}

// Calcula (base^exp) % mod de forma eficiente
unsigned long ExpModular(unsigned long base, unsigned long exp, unsigned long mod) {
    unsigned long result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    return result;
}

// Gera as chaves RSA
void GeraChaves(PrivateKeys *pvkeys, PublicKeys *pbkeys) {
    GeraPrimos(&pvkeys->p, &pvkeys->q);

    pvkeys->n = pvkeys->p * pvkeys->q;
    pbkeys->n = pvkeys->n;
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

// Criptografa uma mensagem caractere por caractere
void EncriptaMensagem(const char *message, unsigned long *encrypted, const PublicKeys *pbkeys) {
    size_t length = strlen(message);
    for (size_t i = 0; i < length; i++) {
        encrypted[i] = ExpModular((unsigned long)message[i], pbkeys->e, pbkeys->n);
    }
}

// Descriptografa a mensagem criptografada
void DecriptaMensagem(const unsigned long *encrypted, size_t length, char *message, const PrivateKeys *pvkeys) {
    for (size_t i = 0; i < length; i++) {
        // Usa as chaves para descriptografar (d e n)
        message[i] = (char)ExpModular(encrypted[i], pvkeys->d, pvkeys->n);
    }
    message[length] = '\0'; // Adiciona o terminador nulo
}