// rsa.h
#ifndef RSA_H
#define RSA_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Estrutura das chaves RSA
typedef struct {
  unsigned long e; // Expoente público
  unsigned long n; // Produto p*q
} PublicKeys;

typedef struct {
  unsigned long d; // Expoente privado
  unsigned long p; // Chave privada primo1
  unsigned long q; // Chave privada primo2
  unsigned long n; // Produto p*q
} PrivateKeys;

// Funções do TAD
int EhPrimo(unsigned long num);
void GeraPrimos(unsigned long *p, unsigned long *q);
unsigned long MDC(unsigned long a, unsigned long b);
unsigned long InversoModular(unsigned long e, unsigned long totient);
unsigned long ExpModular(unsigned long base, unsigned long exp, unsigned long mod);
void GeraChaves(PrivateKeys *pvkeys, PublicKeys *pbkeys);
void EncriptaMensagem(const char *message, unsigned long *encrypted, const PublicKeys *pbkeys);
void DecriptaMensagem(const unsigned long *encrypted, size_t length, char *message, const PrivateKeys *pvkeys);

#endif