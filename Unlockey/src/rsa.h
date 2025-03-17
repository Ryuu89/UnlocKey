// rsa.h
#ifndef RSA_H
#define RSA_H

#include <Arduino.h>
#include <MFRC522.h>

typedef struct PublicKeys PublicKeys;
typedef struct PrivateKeys PrivateKeys;

// Funcoes matematicas basicas
int EhPrimo(unsigned long num);
void GeraPrimos(unsigned long *p, unsigned long *q);
unsigned long MDC(unsigned long a, unsigned long b);
unsigned long InversoModular(unsigned long e, unsigned long totient);
unsigned long ExpModular(unsigned long base, unsigned long exp, unsigned long mod);

// Gerenciaamento de memoria
void InitKeys(PrivateKeys** priv, PublicKeys** pub);
void DeleteKeys(PrivateKeys* priv, PublicKeys* pub);

// Operacoes com chaves
void GeraChaves(PrivateKeys* priv, PublicKeys* pub);
void MostraChaves(const PrivateKeys* priv, const PublicKeys* pub);
int SalvarChaves(PrivateKeys *priv, PublicKeys *pub, MFRC522 *rfid);
int LerChavesPrivadas(PrivateKeys* priv, MFRC522 *rfid);

// Operacoes de criptografia
int EncriptaMensagem(const unsigned char* message, unsigned long* encrypted, const PublicKeys* pub);
int DecriptaMensagem(const unsigned long* encrypted, unsigned int length, unsigned char* message, const PrivateKeys* priv);
#endif