#include <Arduino.h>
#include <MFRC522.h>
#include "rsa.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct PublicKeys {
    unsigned long e; // Expoente público
    unsigned long n; // Produto p*q
};

struct PrivateKeys {
    unsigned long d; // Expoente privado
    unsigned long p; // Chave privada primo1
    unsigned long q; // Chave privada primo2
};

// Verifica se um número é primo
int EhPrimo(unsigned long num) {
    if (num < 2) return 0;
    unsigned long limit = sqrt(num);
    for (unsigned long i = 2; i <= limit; i++) {
        if (num % i == 0) {
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

    if (r > 1) {
        return 0; // Não há inverso modular
    }
    if (t < 0) {
        t += totient;
    }
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

// Gerenciamento de Memoria
void InitKeys(PrivateKeys** priv, PublicKeys** pub) {
    if (priv) {
        *priv = (struct PrivateKeys*) malloc(sizeof(PrivateKeys));
        if (*priv) {
            (*priv)->d = 0;
            (*priv)->p = 0;
            (*priv)->q = 0;
        }
    }

    if (pub) {
        *pub = (PublicKeys*) malloc(sizeof(PublicKeys));
        if (*pub) {
            (*pub)->e = 0;
            (*pub)->n = 0;
        }
    }
}

void DeleteKeys(PrivateKeys* priv, PublicKeys* pub) {
    if (priv != NULL) {
        free(priv);
    }
    if (pub != NULL) {
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
        Serial.println("Erro ao calcular o inverso modular. Gere novas chaves.");
        exit(1);
    }
}

void MostraChaves(const PrivateKeys* priv, const PublicKeys* pub) {
    if (pub){
        Serial.println("\nChave Pública:");
        Serial.print("\te = ");
        Serial.print(pub->e);
        Serial.print(" | n = ");
        Serial.println(pub->n);
    }
    if (priv){
        Serial.println("Chave Privada:");
        Serial.print("\tp = ");
        Serial.print(priv->p);
        Serial.print(" | q = ");
        Serial.print(priv->q);
        Serial.print(" | d = ");
        Serial.println(priv->d);
    }
}

int SalvarChaves(PrivateKeys *priv, PublicKeys *pub, MFRC522 *rfid) {
    if (rfid == NULL || priv == NULL) {
        return 0;
    }

    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    byte block = 4;
    byte trailerBlock = 7;
    MFRC522::StatusCode status;

    // Authenticate first
    status = rfid->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(rfid->uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.println(F("Falha na autenticação"));
        return 0;
    }

    // Create aligned buffer and write values carefully
    byte buffer[16] = {0}; // Initialize with zeros
    uint32_t values[3] = {priv->p, priv->q, priv->d};
    
    for (int i = 0; i < 3; i++) {
        buffer[i*4]   = (values[i] >> 24) & 0xFF;
        buffer[i*4+1] = (values[i] >> 16) & 0xFF;
        buffer[i*4+2] = (values[i] >> 8) & 0xFF;
        buffer[i*4+3] = values[i] & 0xFF;
    }

    status = rfid->MIFARE_Write(block, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.println(F("Falha ao escrever"));
        return 0;
    }

    return 1;
}

int LerChavesPrivadas(PrivateKeys* priv, MFRC522 *rfid) {
    if (rfid == NULL || priv == NULL) {
        return 0;
    }

    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    byte block = 4;
    byte trailerBlock = 7;
    MFRC522::StatusCode status;

    status = rfid->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(rfid->uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.println(F("Falha na autenticação"));
        return 0;
    }

    byte buffer[18] = {0};
    byte size = sizeof(buffer);

    status = rfid->MIFARE_Read(block, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println(F("Falha na leitura"));
        return 0;
    }

    // Read values carefully byte by byte
    uint32_t values[3] = {0};
    for (int i = 0; i < 3; i++) {
        values[i] = ((uint32_t)buffer[i*4] << 24) |
                   ((uint32_t)buffer[i*4+1] << 16) |
                   ((uint32_t)buffer[i*4+2] << 8) |
                   ((uint32_t)buffer[i*4+3]);
    }

    priv->p = values[0];
    priv->q = values[1];
    priv->d = values[2];

    return 1;
}

// Criptografa uma mensagem caractere por caractere
int EncriptaMensagem(const unsigned char *message, unsigned long *encrypted, const PublicKeys *pbkeys) {
    if (!message || !encrypted || !pbkeys) {
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
    if (!message || !encrypted || !pvkeys) {
        return 0;
    }

    for (unsigned int i = 0; i < length; i++) {
        // Usa as chaves para descriptografar (d e n)
        message[i] = (unsigned char)ExpModular(encrypted[i], pvkeys->d, (pvkeys->p * pvkeys->q));
    }
    message[length] = '\0'; // Adiciona o terminador nulo

    return 1;
}