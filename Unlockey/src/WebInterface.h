#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <time.h>
#include "DataTypes.h"

// Variáveis externas
extern WebServer server;
extern bool novaMensagemWeb;
extern String webRemetente;
extern String webDestinatario;
extern String webMensagem;
extern bool wifiEnabled;
extern const char *ssid;
extern const char *password;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

// Funções relacionadas ao sistema de arquivos
void inicializarSPIFFS();
bool verificarArquivosNecessarios();

// Funções relacionadas ao servidor web
void inicializarServidor();
void handleRoot();
void handleEnviarMensagem();
void ativarWiFi();
void verificarWiFi();
void processarMensagemWeb();
// Funções compartilhadas
extern void SetDataAtual(char* data);