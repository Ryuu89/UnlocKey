#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "DataTypes.h"
#include "icons.h" // Incluir ícones

// Definições para o display ILI9341
#define TFT_CS   15
#define TFT_DC   2
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  4
#define TFT_MISO 19

// Definições para o touchscreen
#define TOUCH_CS 5
#define TOUCH_IRQ 27

// Constantes para a UI
#define COLOR_BACKGROUND 0x0000   // Preto
#define COLOR_TEXT 0xFFFF         // Branco
#define COLOR_BUTTON 0x03E0       // Verde
#define COLOR_BUTTON_TEXT 0xFFFF  // Branco
#define COLOR_HIGHLIGHT 0xFD20    // Amarelo
#define COLOR_BORDER 0x7BEF       // Cinza
#define COLOR_INPUT_BG 0x31A6     // Azul escuro

// Estados da interface
enum UIState {
    MENU_PRINCIPAL,
    CADASTRAR_USUARIO,
    LER_MENSAGENS,
    LER_MENSAGEM_DETALHE,
    STATUS_SISTEMA,
    MENSAGEM_INFO,
    TECLADO_VIRTUAL,
    DESLIGAR_SISTEMA
};

// Estrutura para campo de texto
typedef struct {
    char text[MAX_MESSAGE_LENGTH + 1];
    int cursorPos;
    int maxLength;
    bool active;
} TextField;

// Classe para gerenciar a interface do display
class DisplayInterface {
private:
    Adafruit_ILI9341* tft;
    XPT2046_Touchscreen* touch;
    UIState currentState;
    UIState previousState;
    int selectedOption;
    int currentPage;
    int totalPages;
    int selectedUser;
    bool decryptMode;
    char messageBuffer[128];
    TextField usernameField;
    TextField* activeTextField;
    int numUsuarios;
    
    // Funções de desenho internas
    void drawMainMenu();
    void drawUserRegistration();
    void drawReadMessages();
    void drawMessageDetail();
    void drawSystemStatus();
    void drawInfoMessage();
    void drawVirtualKeyboard();
    
    void handleVirtualKeyboard(int x, int y);
    
    void drawButton(int x, int y, int w, int h, const char* label, bool selected);
    void drawIconButton(int x, int y, int w, int h, const uint8_t* icon, const char* label, bool selected);
    void drawTextField(int x, int y, int w, int h, TextField* field, const char* label, bool selected);
    void drawProgressBar(int x, int y, int w, int h, int value, int maxValue, uint16_t color);
    void drawTitle(const char* title);

public:
    DisplayInterface();
    ~DisplayInterface();
    
    void begin();
    void update();
    bool handleTouch();
    void setState(UIState newState);
    UIState getState();
    
    // Interfaces para obter dados dos formulários
    bool getUserRegistrationData();
    const char* getFormUsername();
    
    // Interfaces para leitura de mensagens
    void showReadMessages(Usuario* usuarios, int numUsuarios, Mensagem* mensagens, int numMensagens);
    bool isDecryptMode();
    int getSelectedUser();
    void setDecryptMode(bool mode);
    void resetUserSelection();
    
    // Interface para status
    void showSystemStatus(int numUsuarios, int numMensagens);
    
    // Funções de interface geral
    void showMainMenu();
    void showMessage(const char* message);
    void appendMessage(const char* msg);
    void clearMessageBuffer();
    bool showRFIDReadingStatus(bool success);
    void showDecryptedMessages(const char* messages);
    void setNumUsuarios(int num);
    void clearAllFields();
};

extern DisplayInterface display;