#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "DataTypes.h"
#include "icons.h"

// Display pin definitions
#define TFT_CS   15
#define TFT_DC   2
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  13
#define TFT_MISO 19

// Use ILI9341 standard color definitions
#define COLOR_BACKGROUND ILI9341_BLACK
#define COLOR_TEXT       ILI9341_WHITE
#define COLOR_BUTTON     ILI9341_GREEN
#define COLOR_BUTTON_TEXT ILI9341_WHITE
#define COLOR_HIGHLIGHT  ILI9341_YELLOW
#define COLOR_BORDER     ILI9341_DARKGREY
#define COLOR_INPUT_BG   ILI9341_NAVY

// Add these constants near the top after the existing #define statements
#define MESSAGE_DISPLAY_TIME 3000    // 3 seconds display time for messages
#define SUCCESS_DISPLAY_TIME 2500    // 2.5 seconds for success messages
#define ERROR_DISPLAY_TIME 4000      // 4 seconds for error messages
#define TRANSITION_DELAY 500         // Delay between UI transitions

// Add missing constants for better memory management
#define MESSAGE_BUFFER_SIZE 128      // Size of message buffer

// UI states
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

// Optimize TextField structure to use less memory
typedef struct {
    char text[MAX_USERNAME_LENGTH];  // Fixed size array instead of pointer
    int cursorPos;
    int maxLength;
    bool active;
} TextField;

// Display interface class
class DisplayInterface {
private:
    Adafruit_ILI9341* tft;
    void* touch;  // Keep as void* since we don't use it but class structure references it
    UIState currentState;
    UIState previousState;
    int selectedOption;
    int currentPage;
    int totalPages;
    bool decryptMode;
    char messageBuffer[128];
    TextField* activeTextField;
    int numUsuarios;
    unsigned long lastActivityTime; // Track last UI interaction

    // Drawing utility functions
    void drawTitle(const char* title);
    void drawButton(int x, int y, int w, int h, const char* label, bool selected);
    void drawMainMenuContent();
    void drawIconButton(int x, int y, int w, int h, const uint8_t* icon, const char* label, bool selected);
    void drawTextField(int x, int y, int w, int h, TextField* field, const char* label, bool selected);
    void drawProgressBar(int x, int y, int w, int h, int value, int maxValue, uint16_t color);
    void drawWrappedText(const char* text, int x, int y, int w, int lineHeight = 15);

public:
    // Make selectedUser public since it needs to be accessed from main.cpp
    int selectedUser;

    DisplayInterface();
    ~DisplayInterface();
    
    // Core UI methods
    void begin();
    void update(bool clearScreen = false);
    void setState(UIState newState);
    UIState getState();
    int getSelectedOption();
    void showSplashScreen();
    void showMainMenu();
    void hardReset();
    void forceRefresh();
    
    // Screen methods
    void showUserRegistration();
    void showUserList(Usuario* usuarios, int numUsuarios, const char* title);
    void showReadMessages(); // Add overload with no arguments for internal use
    void showReadMessages(Usuario* usuarios, int numUsuarios, Mensagem* mensagens, int numMensagens);
    void showSystemStatus();
    void showInfoMessage();
    void showMessageDetail();
    
    void showMessageDetailPaged(const char* message);
    
    // Form methods
    bool getUserRegistrationData();
    const char* getFormUsername();
    
    // Message methods
    void showMessage(const char* message, unsigned long displayTime = 0);
    void showEncryptedMessages(Usuario* usuario, Mensagem* mensagens, int numMensagens);
    void showDecryptedMessages(const char* messages);
    void appendMessage(const char* msg);
    void clearMessageBuffer();
    
    // State methods
    bool isDecryptMode();
    int getSelectedUser();
    void setDecryptMode(bool mode);
    void resetUserSelection();
    void setNumUsuarios(int num);
    void clearScreen();
    void clearAllFields();

    // RFID UI methods
    void showRFIDReadingScreen();
    void showRFIDSuccess();
    void showRFIDTimeout();
    bool showRFIDReadingStatus(bool success);
    
    // Touch handling stubs
    bool handleTouch();
    bool isTouchDetected();
    
    // Public field needed for direct access
    TextField usernameField;
    
    // Resource management methods
    void freeResources();
    void reclaimMemory();
};

extern DisplayInterface display;