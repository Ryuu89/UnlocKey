#include "DisplayInterface.h"
#include <WiFi.h>
#include <stdio.h>
#include <SPIFFS.h>

// Global instance
DisplayInterface display;

DisplayInterface::DisplayInterface() {
    tft = nullptr;
    touch = nullptr; // Mantido apenas para compatibilidade da estrutura
    currentState = MENU_PRINCIPAL;
    previousState = MENU_PRINCIPAL;
    selectedOption = 0;
    currentPage = 0;
    totalPages = 0;
    selectedUser = -1;
    decryptMode = false;
    activeTextField = nullptr;
    numUsuarios = 0;
    lastActivityTime = 0;
    
    // Inicializa username com string vazia
    memset(usernameField.text, 0, sizeof(usernameField.text));
    usernameField.maxLength = MAX_USERNAME_LENGTH - 1;
    usernameField.cursorPos = 0;
    usernameField.active = false;
    
    clearMessageBuffer();
}

DisplayInterface::~DisplayInterface() {
    if (tft) delete tft;
    touch = nullptr; // Apenas define como nullptr já que é um ponteiro void
}

void DisplayInterface::begin() {
    // Libera qualquer instância existente
    if (tft) {
        delete tft;
        tft = nullptr;
    }
    
    // Inicializa display com os pinos corretos
    tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
    if (!tft) {
        Serial.println(F("[DISPLAY] Falha ao alocar memória para o display!"));
        return;
    }
    
    // Reset do display
    if (TFT_RST >= 0) {
        pinMode(TFT_RST, OUTPUT);
        digitalWrite(TFT_RST, LOW);
        delay(50);
        digitalWrite(TFT_RST, HIGH);
        delay(50);
    }
    
    // Inicializa display
    tft->begin(24000000); // 2MHz para melhor estabilidade na primeira inicialização
    delay(50);
    
    // Define rotação (paisagem)
    tft->setRotation(3);
    
    // Teste básico
    tft->fillScreen(COLOR_BACKGROUND);
    
    // Reset do estado da UI
    currentState = MENU_PRINCIPAL;
    previousState = MENU_PRINCIPAL;
    selectedOption = 0;
    
    clearAllFields();
}

void DisplayInterface::showSplashScreen() {
    tft->fillScreen(COLOR_BACKGROUND);
    
    // Nome do app
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(3);
    tft->setCursor(60, 30);
    tft->println("UnlocKey");
    
    tft->setTextSize(2);
    tft->setCursor(40, 90);
    tft->println("Sistema de Mensagens");
    tft->setCursor(60, 120);
    tft->println("Criptografadas");
    
    // Ícone de cadeado
    tft->drawBitmap(140, 160, iconLock, 16, 16, COLOR_TEXT);
    
    // Mensagem de rodapé
    tft->setTextSize(1);
    tft->setTextColor(COLOR_HIGHLIGHT);
    tft->setCursor(60, 200);
    tft->println("Inicializando sistema...");
    
    delay(1000);
}

void DisplayInterface::hardReset() {
    // Salva o estado atual
    UIState savedState = currentState;
    
    // Reset do hardware
    SPI.end(); 
    
    if (tft) {
        delete tft;
        tft = nullptr;
    }
    
    // Reset dos pinos
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, LOW);
    delay(50);
    digitalWrite(TFT_RST, HIGH);
    delay(50);
    
    // Inicializa SPI
    SPI.begin();
    
    // Cria nova instância TFT
    tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
    if (!tft) {
        Serial.println(F("[DISPLAY] Falha ao alocar memória para o display!"));
        return;
    }
    
    tft->begin(1000000);
    tft->setRotation(3);
    
    tft->fillScreen(COLOR_BACKGROUND);
    currentState = savedState;
    lastActivityTime = millis();
}

void DisplayInterface::forceRefresh() {
    tft->fillScreen(COLOR_BACKGROUND);
    lastActivityTime = millis();
    update();
}

void DisplayInterface::update(bool clearScreen) {
    static UIState lastDrawnState = MENU_PRINCIPAL;
    static unsigned long lastDrawTime = 0;
    static int lastSelectedOption = -1;
    static char lastMessageCheck[32] = "";
    
    if (!clearScreen && 
        currentState == lastDrawnState && 
        selectedOption == lastSelectedOption && 
        millis() - lastDrawTime < 1000 && // Increase this from 300ms to 1000ms
        (currentState != MENSAGEM_INFO || strcmp(messageBuffer, lastMessageCheck) == 0)) {
        return;
    }


    if (currentState == MENSAGEM_INFO) {
        strncpy(lastMessageCheck, messageBuffer, sizeof(lastMessageCheck)-1);
        lastMessageCheck[sizeof(lastMessageCheck)-1] = '\0';
    }

    if (clearScreen || currentState != lastDrawnState) {
        tft->startWrite();
        tft->fillScreen(COLOR_BACKGROUND);
        tft->endWrite();
    }

    lastDrawnState = currentState;
    lastSelectedOption = selectedOption;
    lastDrawTime = millis();
    
    switch (currentState) {
        case MENU_PRINCIPAL:
            drawMainMenuContent();
            break;
        case CADASTRAR_USUARIO:
            showUserRegistration();
            break;
        case LER_MENSAGENS:
            static UIState lastLerMsgState = MENU_PRINCIPAL;
            if (lastLerMsgState != LER_MENSAGENS) {
                tft->fillScreen(COLOR_BACKGROUND);
                drawTitle("Ler Mensagens");
                lastLerMsgState = LER_MENSAGENS;
            }
            tft->setTextColor(COLOR_TEXT);
            tft->setTextSize(1);
            tft->setCursor(20, 100);
            tft->print("Digite seu nome de usuario no terminal");
        case LER_MENSAGEM_DETALHE:
            showMessageDetail();
            break;
        case STATUS_SISTEMA:
            showSystemStatus();
            break;
        case MENSAGEM_INFO:
            showInfoMessage();
            break;
        case USER_LIST_VIEW:
            break;
        case USER_LIST_SENDER:
            break;
        case USER_LIST_RECIPIENT:
            break;
        case ENCRYPTED_MESSAGES:
            break;
        default:
            break;
    }
    
    lastDrawnState = currentState;
    lastDrawTime = millis();
}

void DisplayInterface::setState(UIState newState) {
    if (currentState != newState) {
        Serial.printf("[DISPLAY] Mudança de estado: %d → %d\n", currentState, newState);
        previousState = currentState;
        currentState = newState;
        selectedOption = 0;
        
        // No automatic screen clearing here!
        // Instead, just mark the time and update will handle it
        lastActivityTime = millis();
    }
}

UIState DisplayInterface::getState() {
    return currentState;
}

int DisplayInterface::getSelectedOption() {
    return selectedOption;
}

void DisplayInterface::showMainMenu() {
    currentState = MENU_PRINCIPAL;
    previousState = MENU_PRINCIPAL;
    selectedOption = 0;
    
    tft->fillScreen(COLOR_BACKGROUND);
    drawMainMenuContent();
}

void DisplayInterface::drawTitle(const char* title) {
    tft->fillRect(0, 0, tft->width(), 30, COLOR_BUTTON);
    tft->setTextColor(COLOR_BUTTON_TEXT);
    tft->setTextSize(2);
    
    int textWidth = strlen(title) * 12;
    int textX = (tft->width() - textWidth) / 2;
    tft->setCursor(textX, 6);
    tft->println(title);
}

void DisplayInterface::drawButton(int x, int y, int w, int h, const char* label, bool selected) {
    uint16_t color = selected ? COLOR_HIGHLIGHT : COLOR_BUTTON;
    tft->fillRoundRect(x, y, w, h, 5, color);
    tft->drawRoundRect(x, y, w, h, 5, COLOR_BORDER);
    tft->setTextColor(COLOR_BUTTON_TEXT);
    tft->setTextSize(1);
    
    int textWidth = strlen(label) * 6;
    int textX = x + (w - textWidth) / 2;
    int textY = y + (h - 8) / 2;
    
    tft->setCursor(textX, textY);
    tft->println(label);
}

void DisplayInterface::drawMainMenuContent() {
    drawTitle("Menu Principal");
    
    drawIconButton(20, 60, 280, 45, iconUser, "1-Cadastrar Usuario", selectedOption == 0);
    drawIconButton(20, 120, 280, 45, iconLock, "2-Ler Mensagens", selectedOption == 1);
    drawIconButton(20, 180, 280, 45, iconWifi, "3-Status do Sistema", selectedOption == 2);
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(10, 230);
    tft->print("UnlocKey v1.0");
    tft->setCursor(220, 230);
    tft->print("ESP32");
}

void DisplayInterface::drawIconButton(int x, int y, int w, int h, const uint8_t* icon, const char* label, bool selected) {
    drawButton(x, y, w, h, label, selected);
    
    if (icon) {
        tft->drawBitmap(x + 10, y + (h - 16) / 2, icon, 16, 16, COLOR_BUTTON_TEXT);
    }
}

void DisplayInterface::drawTextField(int x, int y, int w, int h, TextField* field, const char* label, bool selected) {
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(x, y - 15);
    tft->print(label);
    
    tft->drawRect(x, y, w, h, selected ? COLOR_HIGHLIGHT : COLOR_BORDER);
    tft->fillRect(x + 1, y + 1, w - 2, h - 2, COLOR_INPUT_BG);
    
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 5, y + (h - 8) / 2);
    tft->print(field->text);
    
    if (selected) {
        int cursorX = x + 5 + field->cursorPos * 6;
        tft->drawFastVLine(cursorX, y + 3, h - 6, COLOR_HIGHLIGHT);
    }
}

void DisplayInterface::drawProgressBar(int x, int y, int w, int h, int value, int maxValue, uint16_t color) {
    tft->drawRect(x, y, w, h, COLOR_BORDER);
    
    int fillWidth = map(value, 0, maxValue, 0, w - 2);
    tft->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    char buffer[10];
    sprintf(buffer, "%d%%", (value * 100) / maxValue);
    
    int textWidth = strlen(buffer) * 6;
    int textX = x + (w - textWidth) / 2;
    int textY = y + (h - 8) / 2;
    
    tft->setCursor(textX, textY);
    tft->print(buffer);
}

void DisplayInterface::appendMessage(const char* msg) {
    strncat(messageBuffer, msg, sizeof(messageBuffer) - strlen(messageBuffer) - 1);
    if (currentState == MENSAGEM_INFO || currentState == LER_MENSAGEM_DETALHE) {
        update();
    }
}

void DisplayInterface::clearMessageBuffer() {
    memset(messageBuffer, 0, sizeof(messageBuffer));
}

void DisplayInterface::showMessage(const char* message, unsigned long displayTime) {
    UIState originalState = currentState;
    resetDisplay();
    
    appendMessage(message);
    currentState = MENSAGEM_INFO;
    
    showInfoMessage();
    
    if (displayTime > 0) {
        // Draw progress bar near bottom of screen
        const int barHeight = 16;  // Thicker bar to fit text inside
        const int barWidth = tft->width() - 60;
        const int barX = 30;
        const int barY = 220;  // Position at bottom of screen
        
        // Draw frame for progress bar
        tft->drawRect(barX-2, barY-2, barWidth+4, barHeight+4, COLOR_BORDER);
        
        // Fill background for bar
        tft->fillRect(barX, barY, barWidth, barHeight, ILI9341_DARKGREY);
        
        // Show progress animation
        for (int i = 0; i <= 10; i++) {
            int fillWidth = (barWidth * i) / 10;
            
            // Select color based on message type
            uint16_t fillColor = COLOR_HIGHLIGHT;
            if (strstr(message, "sucesso") || strstr(message, "OK")) {
                fillColor = ILI9341_GREEN;
            } else if (strstr(message, "erro") || strstr(message, "falha")) {
                fillColor = ILI9341_RED;
            }
            
            // Add percentage text INSIDE the bar
            char percentText[5];
            sprintf(percentText, "%d%%", i*10);
            
            // Calculate center position for text
            int textWidth = strlen(percentText) * 6;
            int textX = barX + (barWidth - textWidth) / 2;
            int textY = barY + (barHeight - 8) / 2; // Center vertically
            
            // Clear previous text area
            if (i < 6){
                tft->fillRect(textX-2, textY-1, textWidth+4, 10, ILI9341_DARKGREY);
            }
            
            // Fill progress portion
            tft->fillRect(barX, barY, fillWidth, barHeight, fillColor);

            // Draw percentage text
            tft->setTextColor(COLOR_BUTTON_TEXT); // White text for contrast
            tft->setTextSize(1);
            tft->setCursor(textX, textY);
            tft->print(percentText);
            
            delay(displayTime/10);
        }
        
        delay(200);
        resetDisplay();
        
        // Instead of just restoring state, check if we were sending a message
        if (originalState == CADASTRAR_USUARIO || 
            strcmp(message, "Mensagem enviada\ncom sucesso!") == 0) {
            // For these specific cases, return to main menu
            currentState = MENU_PRINCIPAL;
        } else {
            // Otherwise restore the original state
            currentState = originalState;
        }
        
        update(true); // Force update with fresh redraw
    }
}

void DisplayInterface::showEncryptedMessages(Usuario* usuario, Mensagem* mensagens, int numMensagens) {
    setState(ENCRYPTED_MESSAGES);
    tft->fillScreen(COLOR_BACKGROUND);
    drawTitle("Mensagens Criptografadas");
    
    tft->setTextColor(COLOR_HIGHLIGHT);
    tft->setTextSize(1);
    tft->setCursor(10, 40);
    tft->print("Usuario: ");
    tft->print(usuario->username);
    
    // Count messages for this user
    int msgCount = 0;
    for (int i = 0; i < numMensagens; i++) {
        if (strcmp(mensagens[i].destinatario, usuario->username) == 0) {
            msgCount++;
        }
    }
    
    if (msgCount == 0) {
        tft->setTextColor(COLOR_TEXT);
        tft->setCursor(10, 100);
        tft->println("Nenhuma mensagem encontrada");
        return;
    }
    
    // Show encrypted messages
    int y = 65;
    int displayedCount = 0;
    for (int i = 0; i < numMensagens && displayedCount < 3; i++) {
        if (strcmp(mensagens[i].destinatario, usuario->username) == 0) {
            // Show sender and date
            tft->setTextColor(COLOR_TEXT);
            tft->setCursor(10, y);
            tft->print("De: ");
            tft->print(mensagens[i].remetente);
            tft->print(" [");
            tft->print(mensagens[i].data);
            tft->print("]");
            
            // Show encrypted content preview (up to 25 chars)
            tft->setCursor(20, y+15);
            tft->setTextColor(ILI9341_CYAN);
            
            // Preview of encrypted content - limited to avoid overflow
            int previewLen = min(40, (int)mensagens[i].tamanhoMsg);
            for (int j = 0; j < previewLen; j++) {
                tft->print((char)(mensagens[i].mensagemCriptografada[j] % 94 + 33));
            }
            if (mensagens[i].tamanhoMsg > previewLen) {
                tft->print(" ...");
            }
            
            // Lock icon
            tft->drawBitmap(280, y, iconLock, 16, 16, COLOR_HIGHLIGHT);
            
            y += 45; // More space between messages
            displayedCount++;
        }
    }
    
    // Show count if there are more messages
    if (msgCount > displayedCount) {
        tft->setTextColor(COLOR_TEXT);
        tft->setCursor(10, 190);
        tft->print("+ ");
        tft->print(msgCount - displayedCount);
        tft->print(" mais mensagens");
    }
    
    // Instructions for decryption
    tft->setTextColor(COLOR_HIGHLIGHT);
    tft->setCursor(10, 210);
    tft->println("Descriptografar? 1-Sim, 0-Nao");
}

void DisplayInterface::showInfoMessage() {
    drawTitle("Informacao");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    char buffer[128];
    strncpy(buffer, messageBuffer, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* line = strtok(buffer, "\n");
    int y = 50;
    while (line != NULL) {
        int textWidth = strlen(line) * 6;
        int textX = (tft->width() - textWidth) / 2;
        tft->setCursor(textX, y);
        tft->println(line);
        y += 20;
        line = strtok(NULL, "\n");
    }
    
    drawButton(100, 180, 120, 30, "OK", true);
}

void DisplayInterface::showMessageDetail() {
    if (strlen(messageBuffer) == 0) {
        tft->setCursor(10, 50);
        tft->setTextColor(COLOR_TEXT);
        tft->setTextSize(1);
        tft->print("Nenhuma mensagem para exibir.");
        return;
    }
    
    drawTitle("Mensagem");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    drawWrappedText(messageBuffer, 10, 40, tft->width() - 20);
    
    drawButton(110, 220, 100, 30, "Voltar", false);
}

void DisplayInterface::showMessageDetailPaged(const char* message) {
    if (!message || strlen(message) == 0) {
        showMessage("Nenhuma mensagem para exibir");
        return;
    }
    
    // Always clear screen first
    tft->fillScreen(COLOR_BACKGROUND);
    
    clearMessageBuffer();
    strncpy(messageBuffer, message, sizeof(messageBuffer) - 1);
    messageBuffer[sizeof(messageBuffer) - 1] = '\0';
    
    const int CHARS_PER_PAGE = 300; // Simplified calculation - just show chunks
    int totalLen = strlen(messageBuffer);
    int pages = (totalLen + CHARS_PER_PAGE - 1) / CHARS_PER_PAGE;
    int currentPage = 0;
    
    while (currentPage < pages) {
        // Clear screen with each page change
        tft->fillScreen(COLOR_BACKGROUND);
        drawTitle("Mensagem Decriptada");
        
        // Calculate the text chunk for this page
        int startIdx = currentPage * CHARS_PER_PAGE;
        int charsToShow = min(CHARS_PER_PAGE, totalLen - startIdx);
        
        // Create a temporary working buffer on heap
        char* pageText = (char*)malloc(charsToShow + 1);
        if (pageText) {
            strncpy(pageText, messageBuffer + startIdx, charsToShow);
            pageText[charsToShow] = '\0';
            
            // Display the text
            drawWrappedText(pageText, 10, 40, tft->width() - 20, 15);
            free(pageText);
        }
        
        // Page navigation info
        tft->setTextColor(COLOR_TEXT);
        tft->setCursor(10, 210);
        tft->print("Pagina ");
        tft->print(currentPage + 1);
        tft->print(" de ");
        tft->print(pages);
        
        // Display for a fixed time
        delay(6000); // 6 seconds per page
        
        currentPage++;
    }
    
    // Clear screen when done
    tft->fillScreen(COLOR_BACKGROUND);
    
    // Return to main menu
    resetDisplay(); // Add this line
    setState(MENU_PRINCIPAL);
    update(true);
}

void DisplayInterface::showUserRegistration() {
    currentState = CADASTRAR_USUARIO;
    tft->fillScreen(COLOR_BACKGROUND);
    
    drawTitle("Cadastro de Usuario");
    
    // Mostra campo de entrada atual
    drawTextField(20, 70, 280, 30, &usernameField, "Nome do usuario:", true);
    
    // Instruções claras de que a entrada é via terminal
    tft->setTextColor(COLOR_HIGHLIGHT);
    tft->setTextSize(1);
    tft->setCursor(20, 110);
    tft->println("DIGITE o nome no TERMINAL SERIAL");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(20, 140);
    tft->println("Apos confirmar, APROXIME o cartao RFID");
    tft->setCursor(20, 160);
    tft->println("do lado DIREITO do dispositivo -->");
    
    // Ícone RFID no lado direito
    tft->drawBitmap(280, 140, iconLock, 16, 16, COLOR_HIGHLIGHT);
    
    drawButton(110, 210, 100, 30, "Aguardando...", true);
}

void DisplayInterface::showUserList(Usuario* usuarios, int numUsuarios, const char* title) {
    if (strstr(title, "Destinatario")) {
        setState(USER_LIST_RECIPIENT);
    } else {
        setState(USER_LIST_SENDER);
    }
    
    tft->fillScreen(COLOR_BACKGROUND);
    drawTitle(title);
    
    // Section title
    tft->setTextColor(COLOR_HIGHLIGHT);
    tft->setTextSize(1);
    tft->setCursor(10, 40);
    tft->print("Usuarios disponiveis:");
    
    // Show users in a list
    int y = 60;
    int displayedUsers = 0;
    
    for (int i = 0; i < numUsuarios && displayedUsers < 8; i++) {
        tft->setTextColor(COLOR_TEXT);
        tft->setCursor(20, y);
        tft->print(i+1);
        tft->print(". ");
        tft->print(usuarios[i].username);
        
        y += 20;
        displayedUsers++;
        
        if (y > 200 && i < numUsuarios-1) {
            tft->setTextColor(COLOR_HIGHLIGHT);
            tft->setCursor(10, 220);
            tft->print("...mais usuarios");
            break;
        }
    }
    
    // Instructions
    tft->setTextColor(COLOR_BUTTON);
    tft->setCursor(10, 210);
    tft->print("Digite o nome no terminal");
}

bool DisplayInterface::getUserRegistrationData() {
    return strlen(usernameField.text) > 0;
}

const char* DisplayInterface::getFormUsername() {
    return usernameField.text;
}

void DisplayInterface::showReadMessages(Usuario* usuarios, int numUsuarios, Mensagem* mensagens, int numMensagens) {
    currentState = LER_MENSAGENS;
    tft->fillScreen(COLOR_BACKGROUND);
    
    drawTitle("Ler Mensagens");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(20, 60);
    tft->println("Digite seu nome de usuario");
    tft->setCursor(20, 80);
    tft->println("no terminal para visualizar");
    tft->setCursor(20, 100);
    tft->println("suas mensagens.");
    
    // Add animated cursor effect
    static unsigned long lastBlinkTime = 0;
    static bool cursorVisible = true;
    
    if (millis() - lastBlinkTime > 500) {
        cursorVisible = !cursorVisible;
        lastBlinkTime = millis();
    }
    
    tft->setTextSize(2);
    tft->setCursor(160, 150);
    tft->setTextColor(COLOR_HIGHLIGHT);
    if (cursorVisible) {
        tft->print("_");
    } else {
        tft->print(" ");
    }
}

void DisplayInterface::showSystemStatus() {
    currentState = STATUS_SISTEMA;
    tft->fillScreen(COLOR_BACKGROUND);
    
    drawTitle("Status do Sistema");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    // Status da memória
    tft->setCursor(20, 40);
    tft->print("Memoria livre: ");
    tft->print(ESP.getFreeHeap());
    tft->println(" bytes");

    // Barra de memória
    int memPercent = map(ESP.getFreeHeap(), 0, 320000, 0, 100);
    tft->drawRect(20, 55, 280, 10, COLOR_TEXT);
    tft->fillRect(21, 56, memPercent * 2.78, 8, COLOR_BUTTON);
    
    // Status do RFID
    tft->setCursor(20, 80);
    tft->println("RFID: Operacional");
    
    // Status do WiFi
    tft->setCursor(20, 100);
    tft->print("WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
        tft->setTextColor(COLOR_BUTTON);
        tft->print("Conectado");
        tft->setTextColor(COLOR_TEXT);
        tft->setCursor(20, 115);
        tft->print("IP: ");
        tft->println(WiFi.localIP().toString());
    } else {
        tft->setTextColor(ILI9341_RED);
        tft->print("Desconectado");
        tft->setTextColor(COLOR_TEXT);
    }
    
    // Informações de versão
    tft->setTextColor(ILI9341_CYAN);
    tft->setCursor(20, 200);
    tft->println("UnlocKey v1.0 - Sistema em execucao");
    
    drawButton(110, 220, 100, 30, "Voltar", true);
}

// Funções stub - mantidas apenas para compatibilidade de interface
bool DisplayInterface::handleTouch() {
    return false;
}

bool DisplayInterface::isTouchDetected() {
    return false;
}

bool DisplayInterface::isDecryptMode() {
    return decryptMode;
}

int DisplayInterface::getSelectedUser() {
    return selectedUser;
}

void DisplayInterface::setNumUsuarios(int num) {
    numUsuarios = num;
}

void DisplayInterface::clearAllFields() {
    memset(usernameField.text, 0, sizeof(usernameField.text));
    usernameField.cursorPos = 0;
}

void DisplayInterface::setDecryptMode(bool mode) {
    decryptMode = mode;
    tft->fillScreen(COLOR_BACKGROUND);
    update();
}

void DisplayInterface::resetDisplay() {
    // Clear message buffer completely
    memset(messageBuffer, 0, sizeof(messageBuffer));
    
    // Clear screen
    tft->fillScreen(COLOR_BACKGROUND);
    
    // Reset any other display state
    lastActivityTime = millis();
}

void DisplayInterface::resetUserSelection() {
    selectedUser = -1;
    currentPage = 0;
}

bool DisplayInterface::showRFIDReadingStatus(bool success) {
    if (success) {
        showMessage("Cartao RFID lido com sucesso!");
    } else {
        showMessage("Falha na leitura do cartao RFID!\nTente novamente.");
    }
    return success;
}

void DisplayInterface::showDecryptedMessages(const char* messages) {
    resetDisplay();
    setState(LER_MENSAGEM_DETALHE);
    appendMessage(messages);

    drawTitle("Mensagem Decriptada");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(10, 40);
    
    drawWrappedText(messageBuffer, 10, 40, tft->width() - 20, 15);
    drawButton(110, 220, 100, 30, "Voltar", true);
}

void DisplayInterface::freeResources() {
    clearMessageBuffer();
    currentPage = 0;
    selectedUser = -1;
}

void DisplayInterface::clearScreen() {
    if (tft) {
        tft->fillScreen(COLOR_BACKGROUND);
    }
}

void DisplayInterface::reclaimMemory() {
    freeResources();
    clearMessageBuffer();
}

void DisplayInterface::drawWrappedText(const char* text, int x, int y, int w, int lineHeight) {
    if (!text || !tft) return;
    
    tft->setTextColor(COLOR_TEXT);
    
    int cursorX = x;
    int cursorY = y;
    const char* p = text;
    
    int maxLines = (tft->height() - y - 20) / lineHeight;
    int currentLine = 0;

    while (*p) {
        // Verifica se precisamos quebrar linha
        if (cursorX + 6 >= x + w) {
            cursorX = x;
            cursorY += lineHeight;
            currentLine++;
            if (currentLine >= maxLines) {
                tft->setCursor(x, cursorY);
                tft->print("..."); // Indicate that text was truncated
                break;
            }
        }
        
        // Verifica se é uma quebra de linha
        if (*p == '\n') {
            cursorX = x;
            cursorY += lineHeight;
            p++;
            continue;
        }
        
        // Desenha o caractere
        tft->setCursor(cursorX, cursorY);
        tft->print(*p);
        cursorX += 6; // Avança cursor
        p++;
    }
}

void DisplayInterface::showRFIDReadingScreen() {
    tft->fillScreen(COLOR_BACKGROUND);
    drawTitle("Leitura de RFID");
    
    // Instruções claras
    tft->setTextColor(COLOR_HIGHLIGHT);
    tft->setTextSize(2);
    tft->setCursor(20, 60);
    tft->println("APROXIME O CARTAO");
    
    tft->setTextColor(ILI9341_GREEN);
    tft->setTextSize(2);
    tft->setCursor(20, 100); 
    tft->println("RFID -->");
    
    // Seta apontando para o leitor RFID
    tft->fillTriangle(210, 100, 270, 120, 210, 140, COLOR_HIGHLIGHT);
    
    // Quadro de animação
    tft->drawRect(50, 170, 220, 30, COLOR_BORDER);
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(55, 190);
    tft->println("Aguardando leitura do cartao...");
}

void DisplayInterface::showRFIDSuccess() {
    tft->fillScreen(COLOR_BACKGROUND);
    tft->fillRoundRect(50, 70, 220, 100, 15, ILI9341_GREEN);
    
    tft->setTextColor(COLOR_BUTTON_TEXT);
    tft->setTextSize(2);
    tft->setCursor(65, 90);
    tft->println("CARTAO LIDO");
    tft->setCursor(65, 115);
    tft->println("COM SUCESSO!");
    
    // Add checkmark symbol
    tft->fillCircle(160, 150, 15, COLOR_TEXT);
    tft->drawLine(145, 150, 155, 160, ILI9341_GREEN);
    tft->drawLine(155, 160, 175, 140, ILI9341_GREEN);
    // Longer delay - this screen is important feedback
    delay(2000);
}

void DisplayInterface::showRFIDTimeout() {
    tft->fillRect(20, 180, 280, 50, ILI9341_RED);
    tft->setTextColor(COLOR_BUTTON_TEXT);
    tft->setTextSize(2);
    tft->setCursor(40, 195);
    tft->print("TIMEOUT! TENTE NOVAMENTE");
    
    delay(2000);
}

// Remove completamente estas funções que não são necessárias para o seu caso
// pois o display é apenas para instruções visuais:
// - showSendMessages (envio é feito via web)
// - showReadMessages (simplificado e já implementado em outra função)
// - drawVirtualKeyboard (não há touchscreen)

// Add this no-parameter version for internal use
void DisplayInterface::showReadMessages() {
    currentState = LER_MENSAGENS;
    tft->fillScreen(COLOR_BACKGROUND);
    
    drawTitle("Ler Mensagens");
    
    tft->setTextColor(COLOR_HIGHLIGHT);
    tft->setTextSize(1);
    tft->setCursor(10, 70);
    tft->println("Para visualizar suas mensagens:");
    tft->setCursor(10, 90);
    tft->println("1. Digite seu nome de usuario");
    tft->setCursor(10, 110); 
    tft->println("2. Aproxime cartao RFID");
    tft->setCursor(10, 130);
    tft->println("3. Aguarde descriptografia");
    
    // Desenha ícone RFID
    tft->drawBitmap(260, 100, iconLock, 16, 16, COLOR_HIGHLIGHT);
    
    // Animação simples (pisca o texto)
    if ((millis() / 500) % 2 == 0) {
        tft->setTextColor(COLOR_BUTTON);
    } else {
        tft->setTextColor(ILI9341_CYAN);
    }
    tft->setCursor(20, 170);
    tft->println("Aguardando entrada no terminal...");
    
    drawButton(110, 220, 100, 30, "Voltar", false);
}