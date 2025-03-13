#pragma once

#include "rsa.h"

// Constantes compartilhadas
#define MAX_USERS 5
#define MAX_MESSAGES 30
#define MAX_MESSAGE_LENGTH 100
#define MAX_USERNAME_LENGTH 15

// Estrutura para armazenar dados dos usuários
typedef struct {
    char username[MAX_USERNAME_LENGTH];
    PublicKeys *chavePublica;
} Usuario;

// Estrutura para armazenar mensagens
typedef struct {
    char remetente[MAX_USERNAME_LENGTH];
    char destinatario[MAX_USERNAME_LENGTH];
    unsigned long mensagemCriptografada[MAX_MESSAGE_LENGTH];
    unsigned int tamanhoMsg;
    char data[17]; // DD/MM/YYYY HH:MM
} Mensagem;