#include "DisplayInterface.h"
#include <stdio.h>

// Instância global
DisplayInterface display;

DisplayInterface::DisplayInterface() {
    tft = nullptr;
    touch = nullptr;
    currentState = MENU_PRINCIPAL;
    previousState = MENU_PRINCIPAL;
    selectedOption = 0;
    currentPage = 0;
    totalPages = 0;
    selectedUser = -1;
    decryptMode = false;
    activeTextField = nullptr;
    numUsuarios = 0;
    
    // Inicializa campos de texto
    memset(&usernameField, 0, sizeof(TextField));
    usernameField.maxLength = MAX_USERNAME_LENGTH - 1;
    
    clearMessageBuffer();
}

DisplayInterface::~DisplayInterface() {
    if (tft) delete tft;
    if (touch) delete touch;
}

void DisplayInterface::begin() {
    tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
    touch = new XPT2046_Touchscreen(TOUCH_CS);
    
    tft->begin();
    touch->begin();
    
    // Rotação do display para modo paisagem
    tft->setRotation(1);
    
    // Limpa a tela
    tft->fillScreen(COLOR_BACKGROUND);
    
    // Exibe tela inicial
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(3);
    tft->setCursor(60, 30);
    tft->println("UnlocKey");
    tft->setTextSize(2);
    tft->setCursor(40, 90);
    tft->println("Sistema de Mensagens");
    tft->setCursor(60, 120);
    tft->println("Criptografadas");
    
    // Desenha ícone de cadeado
    tft->drawBitmap(140, 160, iconLock, 16, 16, COLOR_TEXT);
    
    delay(2000);
    showMainMenu();
}

void DisplayInterface::update() {
    switch (currentState) {
        case MENU_PRINCIPAL:
            drawMainMenu();
            break;
        case CADASTRAR_USUARIO:
            drawUserRegistration();
            break;
        case LER_MENSAGENS:
            drawReadMessages();
            break;
        case LER_MENSAGEM_DETALHE:
            drawMessageDetail();
            break;
        case STATUS_SISTEMA:
            drawSystemStatus();
            break;
        case MENSAGEM_INFO:
            drawInfoMessage();
            break;
        case TECLADO_VIRTUAL:
            drawVirtualKeyboard();
            break;
        default:
            break;
    }
}

bool DisplayInterface::handleTouch() {
    if (!touch->touched()) {
        return false;
    }
    
    // Debounce
    delay(50);
    if (!touch->touched()) {
        return false;
    }
    
    TS_Point p = touch->getPoint();
    
    // Converter coordenadas do toque para coordenadas da tela
    // Nota: Isso pode precisar de calibração para seu display específico
    int y = map(p.x, 240, 3800, 0, tft->width());
    int x = map(p.y, 3800, 240, 0, tft->height());
    
    // Processar toque com base no estado atual
    switch (currentState) {
        case MENU_PRINCIPAL:
            // Verificar botões do menu principal (3 botões)
            if (y > 80 && y < 120) { // Primeiro botão - Cadastrar
                if (x > 20 && x < 300) {
                    setState(CADASTRAR_USUARIO);
                    return true;
                }
            } 
            else if (y > 130 && y < 170) { // Segundo botão - Ler mensagens
                if (x > 20 && x < 300) {
                    setState(LER_MENSAGENS);
                    return true;
                }
            }
            else if (y > 180 && y < 220) { // Terceiro botão - Status
                if (x > 20 && x < 300) {
                    setState(STATUS_SISTEMA);
                    return true;
                }
            }
            break;
            
        case MENSAGEM_INFO:
            // Verificar botão OK
            if (y > 180 && y < 210 && x > 100 && x < 220) {
                setState(previousState);
                return true;
            }
            break;
            
        case CADASTRAR_USUARIO:
            // Verificar campo de texto
            if (y > 65 && y < 95 && x > 20 && x < 300) {
                usernameField.active = true;
                activeTextField = &usernameField;
                setState(TECLADO_VIRTUAL);
                return true;
            }
            
            // Verificar botões cancelar/ok
            if (y > 240 && y < 270) {
                if (x > 20 && x < 120) { // Cancelar
                    setState(MENU_PRINCIPAL);
                    memset(usernameField.text, 0, sizeof(usernameField.text));
                    usernameField.cursorPos = 0;
                    return true;
                }
                else if (x > 200 && x < 300) { // OK
                    // Verificar se o nome foi preenchido
                    if (strlen(usernameField.text) > 0) {
                        return true;
                    } else {
                        showMessage("Nome não pode ser vazio!");
                    }
                    return true;
                }
            }
            break;
            
        case LER_MENSAGENS:
            // Verificar botões de seleção de usuário
            if (y > 70 && y < 220) {
                int userIndex = (y - 70) / 30 + currentPage * 5; // 5 usuários por página
                if (userIndex < numUsuarios) {
                    selectedUser = userIndex;
                    selectedOption = userIndex;
                    tft->fillScreen(COLOR_BACKGROUND);
                    drawReadMessages();
                    return false;
                }
            }
            
            // Botões de paginação
            if (totalPages > 1 && y > 225 && y < 245) {
                if (x > 20 && x < 100) { // Anterior
                    if (currentPage > 0) {
                        currentPage--;
                        tft->fillScreen(COLOR_BACKGROUND);
                        drawReadMessages();
                    }
                    return false;
                }
                else if (x > 220 && x < 300) { // Próximo
                    if (currentPage < totalPages - 1) {
                        currentPage++;
                        tft->fillScreen(COLOR_BACKGROUND);
                        drawReadMessages();
                    }
                    return false;
                }
            }
            
            // Botão descriptografar/voltar
            if (y > 230 && y < 250 && x > 110 && x < 210) {
                if (decryptMode) {
                    decryptMode = false;
                    selectedUser = -1;
                    tft->fillScreen(COLOR_BACKGROUND);
                    drawReadMessages();
                } else {
                    if (selectedUser >= 0) {
                        decryptMode = true;
                        return true;
                    } else {
                        setState(MENU_PRINCIPAL);
                    }
                }
                return false;
            }
            break;
            
        case LER_MENSAGEM_DETALHE:
            // Botão voltar
            if (y > 220 && y < 250 && x > 110 && x < 210) {
                setState(LER_MENSAGENS);
                return true;
            }
            break;
            
        case STATUS_SISTEMA:
            // Verificar botão voltar
            if (y > 200 && y < 230 && x > 110 && x < 210) {
                setState(MENU_PRINCIPAL);
                return true;
            }
            break;
            
        case TECLADO_VIRTUAL:
            // Processar toques no teclado virtual
            handleVirtualKeyboard(x, y);
            return false;
            
        default:
            break;
    }
    
    return false;
}

void DisplayInterface::handleVirtualKeyboard(int x, int y) {
    if (!activeTextField) {
        setState(previousState);
        return;
    }
    
    // Verificar área do teclado
    if (y > 100 && y < 220) {
        int row = (y - 100) / 30;
        int col = (x - 10) / 30;
        
        if (row >= 0 && row < 4 && col >= 0 && col < 10) {
            char keys[4][11] = {
                {'1','2','3','4','5','6','7','8','9','0','\0'},
                {'q','w','e','r','t','y','u','i','o','p','\0'},
                {'a','s','d','f','g','h','j','k','l','\0','\0'},
                {'z','x','c','v','b','n','m','.','-','\0','\0'}
            };
            
            char key = keys[row][col];
            if (key != '\0') {
                if (activeTextField->cursorPos < activeTextField->maxLength) {
                    activeTextField->text[activeTextField->cursorPos++] = key;
                    activeTextField->text[activeTextField->cursorPos] = '\0';
                }
            }
        }
    }
    
    // Verificar botões de controle
    if (y > 220 && y < 240) {
        if (x > 10 && x < 70) { // Espaço
            if (activeTextField->cursorPos < activeTextField->maxLength) {
                activeTextField->text[activeTextField->cursorPos++] = ' ';
                activeTextField->text[activeTextField->cursorPos] = '\0';
            }
        }
        else if (x > 80 && x < 140) { // Apagar
            if (activeTextField->cursorPos > 0) {
                activeTextField->text[--activeTextField->cursorPos] = '\0';
            }
        }
        else if (x > 150 && x < 210) { // Limpar
            memset(activeTextField->text, 0, activeTextField->maxLength + 1);
            activeTextField->cursorPos = 0;
        }
        else if (x > 220 && x < 310) { // OK
            setState(previousState);
            activeTextField->active = false;
        }
    }
    
    // Atualizar teclado
    tft->fillScreen(COLOR_BACKGROUND);
    drawVirtualKeyboard();
}

void DisplayInterface::setState(UIState newState) {
    if (newState != TECLADO_VIRTUAL) {
        previousState = currentState;
    }
    currentState = newState;
    selectedOption = 0;
    tft->fillScreen(COLOR_BACKGROUND);
    update();
}

UIState DisplayInterface::getState() {
    return currentState;
}

void DisplayInterface::showMainMenu() {
    setState(MENU_PRINCIPAL);
}

void DisplayInterface::drawMainMenu() {
    drawTitle("Menu Principal");
    
    // Desenha botões com ícones - apenas os 3 botões necessários
    drawIconButton(20, 80, 280, 40, iconUser, "1-Cadastrar Usuario", selectedOption == 0);
    drawIconButton(20, 130, 280, 40, iconLock, "2-Ler Mensagens", selectedOption == 1);
    drawIconButton(20, 180, 280, 40, iconWifi, "3-Status do Sistema", selectedOption == 2);
    
    // Rodapé
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(10, 220);
    tft->print("UnlocKey v1.0");
    tft->setCursor(220, 220);
    tft->print("ESP32");
}

void DisplayInterface::drawTitle(const char* title) {
    tft->fillRect(0, 0, tft->width(), 30, COLOR_BUTTON);
    tft->setTextColor(COLOR_BUTTON_TEXT);
    tft->setTextSize(2);
    
    int textWidth = strlen(title) * 12; // Estimativa aproximada
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
    
    // Centralizar texto
    int textWidth = strlen(label) * 6; // Estimativa aproximada
    int textX = x + (w - textWidth) / 2;
    int textY = y + (h - 8) / 2;
    
    tft->setCursor(textX, textY);
    tft->println(label);
}

void DisplayInterface::drawIconButton(int x, int y, int w, int h, const uint8_t* icon, const char* label, bool selected) {
    drawButton(x, y, w, h, label, selected);
    
    // Adiciona o ícone à esquerda
    if (icon) {
        tft->drawBitmap(x + 10, y + (h - 16) / 2, icon, 16, 16, COLOR_BUTTON_TEXT);
    }
}

void DisplayInterface::drawTextField(int x, int y, int w, int h, TextField* field, const char* label, bool selected) {
    // Desenha o rótulo
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(x, y - 15);
    tft->print(label);
    
    // Desenha o campo
    tft->drawRect(x, y, w, h, selected ? COLOR_HIGHLIGHT : COLOR_BORDER);
    tft->fillRect(x + 1, y + 1, w - 2, h - 2, COLOR_INPUT_BG);
    
    // Texto dentro do campo
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 5, y + (h - 8) / 2);
    tft->print(field->text);
    
    // Se estiver selecionado, desenha cursor
    if (selected) {
        int cursorX = x + 5 + field->cursorPos * 6;
        tft->drawFastVLine(cursorX, y + 3, h - 6, COLOR_HIGHLIGHT);
    }
}

void DisplayInterface::drawProgressBar(int x, int y, int w, int h, int value, int maxValue, uint16_t color) {
    tft->drawRect(x, y, w, h, COLOR_BORDER);
    
    int fillWidth = map(value, 0, maxValue, 0, w - 2);
    tft->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
    
    // Texto de porcentagem
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

void DisplayInterface::showMessage(const char* message) {
    clearMessageBuffer();
    appendMessage(message);
    setState(MENSAGEM_INFO);
}

void DisplayInterface::drawInfoMessage() {
    drawTitle("Informacao");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    // Quebrar texto em linhas se necessário
    char buffer[128];
    strncpy(buffer, messageBuffer, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* line = strtok(buffer, "\n");
    int y = 50;
    while (line != NULL) {
        // Centraliza o texto
        int textWidth = strlen(line) * 6;
        int textX = (tft->width() - textWidth) / 2;
        tft->setCursor(textX, y);
        tft->println(line);
        y += 20;
        line = strtok(NULL, "\n");
    }
    
    drawButton(100, 180, 120, 30, "OK", true);
}

void DisplayInterface::drawMessageDetail() {
    drawTitle("Mensagem");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    // Quebrar texto em linhas se necessário
    char buffer[128];
    strncpy(buffer, messageBuffer, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* line = strtok(buffer, "\n");
    int y = 40;
    while (line != NULL) {
        tft->setCursor(10, y);
        tft->println(line);
        y += 15;
        line = strtok(NULL, "\n");
        
        // Evitar ultrapassar os limites da tela
        if (y > 200) break;
    }
    
    // Botão para voltar
    drawButton(110, 220, 100, 30, "Voltar", false);
}

void DisplayInterface::drawUserRegistration() {
    drawTitle("Cadastro de Usuario");
    
    // Campo de nome
    drawTextField(20, 70, 280, 30, &usernameField, "Nome do usuario:", true);
    
    // Botões de ação
    drawButton(20, 240, 100, 30, "Cancelar", false);
    drawButton(200, 240, 100, 30, "OK", false);
    
    // Instruções
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(20, 120);
    tft->println("Toque no campo para editar o texto.");
    tft->setCursor(20, 140);
    tft->println("Ao confirmar, aproxime o cartao RFID");
    tft->setCursor(20, 160);
    tft->println("para salvar as chaves de criptografia.");
}

bool DisplayInterface::getUserRegistrationData() {
    // Verifica se o formulário tem dados
    if (strlen(usernameField.text) > 0) {
        return true;
    }
    return false;
}

const char* DisplayInterface::getFormUsername() {
    return usernameField.text;
}

// Método para exibir usuários e suas mensagens
void DisplayInterface::showReadMessages(Usuario* usuarios, int numUsuarios, Mensagem* mensagens, int numMensagens) {
    this->numUsuarios = numUsuarios;  // Armazena para uso interno
    
    if (!decryptMode) {
        drawTitle("Ler Mensagens");
        
        // Calcular paginação
        int usersPerPage = 5;
        totalPages = (numUsuarios + usersPerPage - 1) / usersPerPage;
        if (totalPages == 0){
            totalPages = 1;
        }
        
        // Desenhar lista de usuários
        tft->setTextColor(COLOR_TEXT);
        tft->setTextSize(1);
        tft->setCursor(20, 40);
        tft->print("Selecione um usuario:");
        
        int startIdx = currentPage * usersPerPage;
        int endIdx = min(startIdx + usersPerPage, numUsuarios);
        
        for (int i = startIdx; i < endIdx; i++) {
            int y = 70 + (i - startIdx) * 30;
            bool isSelected = (i == selectedUser);
            
            tft->fillRect(20, y, 280, 25, isSelected ? COLOR_HIGHLIGHT : COLOR_BACKGROUND);
            tft->setTextColor(isSelected ? COLOR_BACKGROUND : COLOR_TEXT);
            tft->setCursor(25, y + 10);
            tft->print(usuarios[i].username);
        }
        
        // Botões de navegação
        if (totalPages > 1) {
            drawButton(20, 225, 80, 20, "Anterior", false);
            
            tft->setTextColor(COLOR_TEXT);
            tft->setCursor(140, 230);
            tft->print(String(currentPage + 1) + "/" + String(totalPages));
            
            drawButton(220, 225, 80, 20, "Proximo", false);
        }
        
        // Botão de ação
        if (selectedUser >= 0) {
            drawButton(110, 230, 100, 20, "Descriptografar", true);
        } else {
            drawButton(110, 230, 100, 20, "Voltar", false);
        }
    } else {
        // Modo de descriptografia - exibir mensagens para o usuário selecionado
        drawTitle("Mensagens");
        
        if (selectedUser < 0 || selectedUser >= numUsuarios) {
            showMessage("Usuario invalido");
            return;
        }
        
        tft->setTextColor(COLOR_TEXT);
        tft->setTextSize(1);
        tft->setCursor(20, 40);
        tft->print("Mensagens para: ");
        tft->print(usuarios[selectedUser].username);
        
        // Contar mensagens para este usuário
        int msgCount = 0;
        for (int i = 0; i < numMensagens; i++) {
            if (strcmp(mensagens[i].destinatario, usuarios[selectedUser].username) == 0) {
                msgCount++;
            }
        }
        
        if (msgCount == 0) {
            tft->setCursor(20, 100);
            tft->print("Nenhuma mensagem encontrada");
        } else {
            // Desenha ícone de RFID
            tft->drawBitmap(140, 70, iconLock, 16, 16, COLOR_HIGHLIGHT);
            
            tft->setCursor(20, 60);
            tft->print("Aproxime o cartao RFID para descriptografar");
            
            // Exibir lista de mensagens criptografadas
            int y = 90;
            for (int i = 0; i < numMensagens; i++) {
                if (strcmp(mensagens[i].destinatario, usuarios[selectedUser].username) == 0) {
                    tft->setCursor(20, y);
                    tft->print("De: ");
                    tft->print(mensagens[i].remetente);
                    tft->print(" [");
                    tft->print(mensagens[i].data);
                    tft->print("]");
                    y += 20;
                    
                    if (y > 200) break; // Limitar o número de mensagens exibidas
                }
            }
        }
        
        drawButton(110, 230, 100, 20, "Voltar", false);
    }
}

// Implementar o método drawReadMessages, que é chamado pelo update
void DisplayInterface::drawReadMessages() {
    // Esta função é apenas um espaço reservado - a renderização real é feita por showReadMessages
    // que recebe os dados necessários
}

// Exibir status do sistema
void DisplayInterface::showSystemStatus(int numUsuarios, int numMensagens) {
    drawTitle("Status do Sistema");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    // Exibir informações
    tft->setCursor(20, 50);
    tft->print("Usuarios: ");
    tft->print(numUsuarios);
    tft->print("/");
    tft->print(MAX_USERS);
    
    // Barra de progresso para usuários
    drawProgressBar(120, 48, 180, 15, numUsuarios, MAX_USERS, COLOR_BUTTON);
    
    tft->setCursor(20, 80);
    tft->print("Mensagens: ");
    tft->print(numMensagens);
    tft->print("/");
    tft->print(MAX_MESSAGES);
    
    // Barra de progresso para mensagens
    drawProgressBar(120, 78, 180, 15, numMensagens, MAX_MESSAGES, COLOR_HIGHLIGHT);
    
    // Informações de memória
    tft->setCursor(20, 110);
    tft->print("Memoria livre: ");
    tft->print(ESP.getFreeHeap());
    tft->print(" bytes");
    
    // Status WiFi
    tft->setCursor(20, 140);
    tft->print("WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
        tft->setTextColor(COLOR_BUTTON);
        tft->print("Conectado");
        tft->setTextColor(COLOR_TEXT);
        
        tft->setCursor(20, 160);
        tft->print("IP: ");
        tft->print(WiFi.localIP().toString());
    } else {
        tft->setTextColor(0xF800); // Vermelho
        tft->print("Desconectado");
        tft->setTextColor(COLOR_TEXT);
    }
    
    // Botão para voltar
    drawButton(110, 200, 100, 30, "Voltar", false);
}

// Implementação do teclado virtual
void DisplayInterface::drawVirtualKeyboard() {
    drawTitle("Teclado Virtual");
    
    // Campo de texto atual
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(20, 40);
    tft->print("Texto: ");
    
    // Exibir o texto atual
    tft->drawRect(70, 40, 230, 25, COLOR_HIGHLIGHT);
    tft->fillRect(71, 41, 228, 23, COLOR_INPUT_BG);
    tft->setCursor(75, 50);
    tft->print(activeTextField->text);
    
    // Desenhar cursor
    int cursorX = 75 + activeTextField->cursorPos * 6;
    tft->drawFastVLine(cursorX, 43, 19, COLOR_HIGHLIGHT);
    
    // Desenhar teclado
    char keys[4][11] = {
        {'1','2','3','4','5','6','7','8','9','0','\0'},
        {'q','w','e','r','t','y','u','i','o','p','\0'},
        {'a','s','d','f','g','h','j','k','l','\0','\0'},
        {'z','x','c','v','b','n','m','.','-','\0','\0'}
    };
    
    // Desenhar as teclas
    tft->setTextColor(COLOR_TEXT);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 10; col++) {
            if (keys[row][col] != '\0') {
                int x = 10 + col * 30;
                int y = 100 + row * 30;
                
                tft->drawRect(x, y, 28, 28, COLOR_BORDER);
                tft->fillRect(x + 1, y + 1, 26, 26, COLOR_BUTTON);
                
                tft->setCursor(x + 11, y + 10);
                tft->print(keys[row][col]);
            }
        }
    }
    
    // Desenhar botões de controle
    tft->drawRect(10, 220, 60, 20, COLOR_BORDER);
    tft->fillRect(11, 221, 58, 18, COLOR_BUTTON);
    tft->setCursor(15, 225);
    tft->print("Espaco");

    tft->drawRect(80, 220, 60, 20, COLOR_BORDER);
    tft->fillRect(81, 221, 58, 18, COLOR_BUTTON);
    tft->setCursor(85, 225);
    tft->print("Apagar");
    
    tft->drawRect(150, 220, 60, 20, COLOR_BORDER);
    tft->fillRect(151, 221, 58, 18, COLOR_BUTTON);
    tft->setCursor(155, 225);
    tft->print("Limpar");
    
    tft->drawRect(220, 220, 90, 20, COLOR_BORDER);
    tft->fillRect(221, 221, 88, 18, COLOR_HIGHLIGHT);
    tft->setCursor(255, 225);
    tft->print("OK");
    
    tft->drawRect(80, 220, 60, 20, COLOR_BORDER);
#include "DisplayInterface.h"
#include <stdio.h>

// Instância global
DisplayInterface display;

DisplayInterface::DisplayInterface() {
    tft = nullptr;
    touch = nullptr;
    currentState = MENU_PRINCIPAL;
    previousState = MENU_PRINCIPAL;
    selectedOption = 0;
    currentPage = 0;
    totalPages = 0;
    selectedUser = -1;
    decryptMode = false;
    activeTextField = nullptr;
    numUsuarios = 0;
    
    // Inicializa campos de texto
    memset(&usernameField, 0, sizeof(TextField));
    usernameField.maxLength = MAX_USERNAME_LENGTH - 1;
    
    clearMessageBuffer();
}

DisplayInterface::~DisplayInterface() {
    if (tft) delete tft;
    if (touch) delete touch;
}

void DisplayInterface::begin() {
    tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
    touch = new XPT2046_Touchscreen(TOUCH_CS);
    
    tft->begin();
    touch->begin();
    
    // Rotação do display para modo paisagem
    tft->setRotation(1);
    
    // Limpa a tela
    tft->fillScreen(COLOR_BACKGROUND);
    
    // Exibe tela inicial
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(3);
    tft->setCursor(60, 30);
    tft->println("UnlocKey");
    tft->setTextSize(2);
    tft->setCursor(40, 90);
    tft->println("Sistema de Mensagens");
    tft->setCursor(60, 120);
    tft->println("Criptografadas");
    
    // Desenha ícone de cadeado
    tft->drawBitmap(140, 160, iconLock, 16, 16, COLOR_TEXT);
    
    delay(2000);
    showMainMenu();
}

void DisplayInterface::update() {
    switch (currentState) {
        case MENU_PRINCIPAL:
            drawMainMenu();
            break;
        case CADASTRAR_USUARIO:
            drawUserRegistration();
            break;
        case LER_MENSAGENS:
            drawReadMessages();
            break;
        case LER_MENSAGEM_DETALHE:
            drawMessageDetail();
            break;
        case STATUS_SISTEMA:
            drawSystemStatus();
            break;
        case MENSAGEM_INFO:
            drawInfoMessage();
            break;
        case TECLADO_VIRTUAL:
            drawVirtualKeyboard();
            break;
        default:
            break;
    }
}

bool DisplayInterface::handleTouch() {
    if (!touch->touched()) {
        return false;
    }
    
    // Debounce
    delay(50);
    if (!touch->touched()) {
        return false;
    }
    
    TS_Point p = touch->getPoint();
    
    // Converter coordenadas do toque para coordenadas da tela
    // Nota: Isso pode precisar de calibração para seu display específico
    int y = map(p.x, 240, 3800, 0, tft->width());
    int x = map(p.y, 3800, 240, 0, tft->height());
    
    // Processar toque com base no estado atual
    switch (currentState) {
        case MENU_PRINCIPAL:
            // Verificar botões do menu principal (3 botões)
            if (y > 80 && y < 120) { // Primeiro botão - Cadastrar
                if (x > 20 && x < 300) {
                    setState(CADASTRAR_USUARIO);
                    return true;
                }
            } 
            else if (y > 130 && y < 170) { // Segundo botão - Ler mensagens
                if (x > 20 && x < 300) {
                    setState(LER_MENSAGENS);
                    return true;
                }
            }
            else if (y > 180 && y < 220) { // Terceiro botão - Status
                if (x > 20 && x < 300) {
                    setState(STATUS_SISTEMA);
                    return true;
                }
            }
            break;
            
        case MENSAGEM_INFO:
            // Verificar botão OK
            if (y > 180 && y < 210 && x > 100 && x < 220) {
                setState(previousState);
                return true;
            }
            break;
            
        case CADASTRAR_USUARIO:
            // Verificar campo de texto
            if (y > 65 && y < 95 && x > 20 && x < 300) {
                usernameField.active = true;
                activeTextField = &usernameField;
                setState(TECLADO_VIRTUAL);
                return true;
            }
            
            // Verificar botões cancelar/ok
            if (y > 240 && y < 270) {
                if (x > 20 && x < 120) { // Cancelar
                    setState(MENU_PRINCIPAL);
                    memset(usernameField.text, 0, sizeof(usernameField.text));
                    usernameField.cursorPos = 0;
                    return true;
                }
                else if (x > 200 && x < 300) { // OK
                    // Verificar se o nome foi preenchido
                    if (strlen(usernameField.text) > 0) {
                        return true;
                    } else {
                        showMessage("Nome não pode ser vazio!");
                    }
                    return true;
                }
            }
            break;
            
        case LER_MENSAGENS:
            // Verificar botões de seleção de usuário
            if (y > 70 && y < 220) {
                int userIndex = (y - 70) / 30 + currentPage * 5; // 5 usuários por página
                if (userIndex < numUsuarios) {
                    selectedUser = userIndex;
                    selectedOption = userIndex;
                    tft->fillScreen(COLOR_BACKGROUND);
                    drawReadMessages();
                    return false;
                }
            }
            
            // Botões de paginação
            if (totalPages > 1 && y > 225 && y < 245) {
                if (x > 20 && x < 100) { // Anterior
                    if (currentPage > 0) {
                        currentPage--;
                        tft->fillScreen(COLOR_BACKGROUND);
                        drawReadMessages();
                    }
                    return false;
                }
                else if (x > 220 && x < 300) { // Próximo
                    if (currentPage < totalPages - 1) {
                        currentPage++;
                        tft->fillScreen(COLOR_BACKGROUND);
                        drawReadMessages();
                    }
                    return false;
                }
            }
            
            // Botão descriptografar/voltar
            if (y > 230 && y < 250 && x > 110 && x < 210) {
                if (decryptMode) {
                    decryptMode = false;
                    selectedUser = -1;
                    tft->fillScreen(COLOR_BACKGROUND);
                    drawReadMessages();
                } else {
                    if (selectedUser >= 0) {
                        decryptMode = true;
                        return true;
                    } else {
                        setState(MENU_PRINCIPAL);
                    }
                }
                return false;
            }
            break;
            
        case LER_MENSAGEM_DETALHE:
            // Botão voltar
            if (y > 220 && y < 250 && x > 110 && x < 210) {
                setState(LER_MENSAGENS);
                return true;
            }
            break;
            
        case STATUS_SISTEMA:
            // Verificar botão voltar
            if (y > 200 && y < 230 && x > 110 && x < 210) {
                setState(MENU_PRINCIPAL);
                return true;
            }
            break;
            
        case TECLADO_VIRTUAL:
            // Processar toques no teclado virtual
            handleVirtualKeyboard(x, y);
            return false;
            
        default:
            break;
    }
    
    return false;
}

void DisplayInterface::handleVirtualKeyboard(int x, int y) {
    if (!activeTextField) {
        setState(previousState);
        return;
    }
    
    // Verificar área do teclado
    if (y > 100 && y < 220) {
        int row = (y - 100) / 30;
        int col = (x - 10) / 30;
        
        if (row >= 0 && row < 4 && col >= 0 && col < 10) {
            char keys[4][11] = {
                {'1','2','3','4','5','6','7','8','9','0','\0'},
                {'q','w','e','r','t','y','u','i','o','p','\0'},
                {'a','s','d','f','g','h','j','k','l','\0','\0'},
                {'z','x','c','v','b','n','m','.','-','\0','\0'}
            };
            
            char key = keys[row][col];
            if (key != '\0') {
                if (activeTextField->cursorPos < activeTextField->maxLength) {
                    activeTextField->text[activeTextField->cursorPos++] = key;
                    activeTextField->text[activeTextField->cursorPos] = '\0';
                }
            }
        }
    }
    
    // Verificar botões de controle
    if (y > 220 && y < 240) {
        if (x > 10 && x < 70) { // Espaço
            if (activeTextField->cursorPos < activeTextField->maxLength) {
                activeTextField->text[activeTextField->cursorPos++] = ' ';
                activeTextField->text[activeTextField->cursorPos] = '\0';
            }
        }
        else if (x > 80 && x < 140) { // Apagar
            if (activeTextField->cursorPos > 0) {
                activeTextField->text[--activeTextField->cursorPos] = '\0';
            }
        }
        else if (x > 150 && x < 210) { // Limpar
            memset(activeTextField->text, 0, activeTextField->maxLength + 1);
            activeTextField->cursorPos = 0;
        }
        else if (x > 220 && x < 310) { // OK
            setState(previousState);
            activeTextField->active = false;
        }
    }
    
    // Atualizar teclado
    tft->fillScreen(COLOR_BACKGROUND);
    drawVirtualKeyboard();
}

void DisplayInterface::setState(UIState newState) {
    if (newState != TECLADO_VIRTUAL) {
        previousState = currentState;
    }
    currentState = newState;
    selectedOption = 0;
    tft->fillScreen(COLOR_BACKGROUND);
    update();
}

UIState DisplayInterface::getState() {
    return currentState;
}

void DisplayInterface::showMainMenu() {
    setState(MENU_PRINCIPAL);
}

void DisplayInterface::drawMainMenu() {
    drawTitle("Menu Principal");
    
    // Desenha botões com ícones - apenas os 3 botões necessários
    drawIconButton(20, 80, 280, 40, iconUser, "1-Cadastrar Usuario", selectedOption == 0);
    drawIconButton(20, 130, 280, 40, iconLock, "2-Ler Mensagens", selectedOption == 1);
    drawIconButton(20, 180, 280, 40, iconWifi, "3-Status do Sistema", selectedOption == 2);
    
    // Rodapé
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(10, 220);
    tft->print("UnlocKey v1.0");
    tft->setCursor(220, 220);
    tft->print("ESP32");
}

void DisplayInterface::drawTitle(const char* title) {
    tft->fillRect(0, 0, tft->width(), 30, COLOR_BUTTON);
    tft->setTextColor(COLOR_BUTTON_TEXT);
    tft->setTextSize(2);
    
    int textWidth = strlen(title) * 12; // Estimativa aproximada
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
    
    // Centralizar texto
    int textWidth = strlen(label) * 6; // Estimativa aproximada
    int textX = x + (w - textWidth) / 2;
    int textY = y + (h - 8) / 2;
    
    tft->setCursor(textX, textY);
    tft->println(label);
}

void DisplayInterface::drawIconButton(int x, int y, int w, int h, const uint8_t* icon, const char* label, bool selected) {
    drawButton(x, y, w, h, label, selected);
    
    // Adiciona o ícone à esquerda
    if (icon) {
        tft->drawBitmap(x + 10, y + (h - 16) / 2, icon, 16, 16, COLOR_BUTTON_TEXT);
    }
}

void DisplayInterface::drawTextField(int x, int y, int w, int h, TextField* field, const char* label, bool selected) {
    // Desenha o rótulo
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(x, y - 15);
    tft->print(label);
    
    // Desenha o campo
    tft->drawRect(x, y, w, h, selected ? COLOR_HIGHLIGHT : COLOR_BORDER);
    tft->fillRect(x + 1, y + 1, w - 2, h - 2, COLOR_INPUT_BG);
    
    // Texto dentro do campo
    tft->setTextColor(COLOR_TEXT);
    tft->setCursor(x + 5, y + (h - 8) / 2);
    tft->print(field->text);
    
    // Se estiver selecionado, desenha cursor
    if (selected) {
        int cursorX = x + 5 + field->cursorPos * 6;
        tft->drawFastVLine(cursorX, y + 3, h - 6, COLOR_HIGHLIGHT);
    }
}

void DisplayInterface::drawProgressBar(int x, int y, int w, int h, int value, int maxValue, uint16_t color) {
    tft->drawRect(x, y, w, h, COLOR_BORDER);
    
    int fillWidth = map(value, 0, maxValue, 0, w - 2);
    tft->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
    
    // Texto de porcentagem
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

void DisplayInterface::showMessage(const char* message) {
    clearMessageBuffer();
    appendMessage(message);
    setState(MENSAGEM_INFO);
}

void DisplayInterface::drawInfoMessage() {
    drawTitle("Informacao");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    // Quebrar texto em linhas se necessário
    char buffer[128];
    strncpy(buffer, messageBuffer, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* line = strtok(buffer, "\n");
    int y = 50;
    while (line != NULL) {
        // Centraliza o texto
        int textWidth = strlen(line) * 6;
        int textX = (tft->width() - textWidth) / 2;
        tft->setCursor(textX, y);
        tft->println(line);
        y += 20;
        line = strtok(NULL, "\n");
    }
    
    drawButton(100, 180, 120, 30, "OK", true);
}

void DisplayInterface::drawMessageDetail() {
    drawTitle("Mensagem");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    // Quebrar texto em linhas se necessário
    char buffer[128];
    strncpy(buffer, messageBuffer, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* line = strtok(buffer, "\n");
    int y = 40;
    while (line != NULL) {
        tft->setCursor(10, y);
        tft->println(line);
        y += 15;
        line = strtok(NULL, "\n");
        
        // Evitar ultrapassar os limites da tela
        if (y > 200) break;
    }
    
    // Botão para voltar
    drawButton(110, 220, 100, 30, "Voltar", false);
}

void DisplayInterface::drawUserRegistration() {
    drawTitle("Cadastro de Usuario");
    
    // Campo de nome
    drawTextField(20, 70, 280, 30, &usernameField, "Nome do usuario:", true);
    
    // Botões de ação
    drawButton(20, 240, 100, 30, "Cancelar", false);
    drawButton(200, 240, 100, 30, "OK", false);
    
    // Instruções
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(20, 120);
    tft->println("Toque no campo para editar o texto.");
    tft->setCursor(20, 140);
    tft->println("Ao confirmar, aproxime o cartao RFID");
    tft->setCursor(20, 160);
    tft->println("para salvar as chaves de criptografia.");
}

bool DisplayInterface::getUserRegistrationData() {
    // Verifica se o formulário tem dados
    if (strlen(usernameField.text) > 0) {
        return true;
    }
    return false;
}


// Método para exibir usuários e suas mensagens
void DisplayInterface::showReadMessages(Usuario* usuarios, int numUsuarios, Mensagem* mensagens, int numMensagens) {
    this->numUsuarios = numUsuarios;  // Armazena para uso interno
    
    if (!decryptMode) {
        drawTitle("Ler Mensagens");
        
        // Calcular paginação
        int usersPerPage = 5;
        totalPages = (numUsuarios + usersPerPage - 1) / usersPerPage;
        if (totalPages == 0) totalPages = 1;
        
        // Desenhar lista de usuários
        tft->setTextColor(COLOR_TEXT);
        tft->setTextSize(1);
        tft->setCursor(20, 40);
        tft->print("Selecione um usuario:");
        
        int startIdx = currentPage * usersPerPage;
        int endIdx = min(startIdx + usersPerPage, numUsuarios);
        
        for (int i = startIdx; i < endIdx; i++) {
            int y = 70 + (i - startIdx) * 30;
            bool isSelected = (i == selectedUser);
            
            tft->fillRect(20, y, 280, 25, isSelected ? COLOR_HIGHLIGHT : COLOR_BACKGROUND);
            tft->setTextColor(isSelected ? COLOR_BACKGROUND : COLOR_TEXT);
            tft->setCursor(25, y + 10);
            tft->print(usuarios[i].username);
        }
        
        // Botões de navegação
        if (totalPages > 1) {
            drawButton(20, 225, 80, 20, "Anterior", false);
            
            tft->setTextColor(COLOR_TEXT);
            tft->setCursor(140, 230);
            tft->print(String(currentPage + 1) + "/" + String(totalPages));
            
            drawButton(220, 225, 80, 20, "Proximo", false);
        }
        
        // Botão de ação
        if (selectedUser >= 0) {
            drawButton(110, 230, 100, 20, "Descriptografar", true);
        } else {
            drawButton(110, 230, 100, 20, "Voltar", false);
        }
    } else {
        // Modo de descriptografia - exibir mensagens para o usuário selecionado
        drawTitle("Mensagens");
        
        if (selectedUser < 0 || selectedUser >= numUsuarios) {
            showMessage("Usuario invalido");
            return;
        }
        
        tft->setTextColor(COLOR_TEXT);
        tft->setTextSize(1);
        tft->setCursor(20, 40);
        tft->print("Mensagens para: ");
        tft->print(usuarios[selectedUser].username);
        
        // Contar mensagens para este usuário
        int msgCount = 0;
        for (int i = 0; i < numMensagens; i++) {
            if (strcmp(mensagens[i].destinatario, usuarios[selectedUser].username) == 0) {
                msgCount++;
            }
        }
        
        if (msgCount == 0) {
            tft->setCursor(20, 100);
            tft->print("Nenhuma mensagem encontrada");
        } else {
            // Desenha ícone de RFID
            tft->drawBitmap(140, 70, iconLock, 16, 16, COLOR_HIGHLIGHT);
            
            tft->setCursor(20, 60);
            tft->print("Aproxime o cartao RFID para descriptografar");
            
            // Exibir lista de mensagens criptografadas
            int y = 90;
            for (int i = 0; i < numMensagens; i++) {
                if (strcmp(mensagens[i].destinatario, usuarios[selectedUser].username) == 0) {
                    tft->setCursor(20, y);
                    tft->print("De: ");
                    tft->print(mensagens[i].remetente);
                    tft->print(" [");
                    tft->print(mensagens[i].data);
                    tft->print("]");
                    y += 20;
                    
                    if (y > 200) break; // Limitar o número de mensagens exibidas
                }
            }
        }
        
        drawButton(110, 230, 100, 20, "Voltar", false);
    }
}

// Implementar o método drawReadMessages, que é chamado pelo update
void DisplayInterface::drawReadMessages() {
    // Esta função é apenas um espaço reservado - a renderização real é feita por showReadMessages
    // que recebe os dados necessários
}

// Exibir status do sistema
void DisplayInterface::showSystemStatus(int numUsuarios, int numMensagens) {
    drawTitle("Status do Sistema");
    
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    
    // Exibir informações
    tft->setCursor(20, 50);
    tft->print("Usuarios: ");
    tft->print(numUsuarios);
    tft->print("/");
    tft->print(MAX_USERS);
    
    // Barra de progresso para usuários
    drawProgressBar(120, 48, 180, 15, numUsuarios, MAX_USERS, COLOR_BUTTON);
    
    tft->setCursor(20, 80);
    tft->print("Mensagens: ");
    tft->print(numMensagens);
    tft->print("/");
    tft->print(MAX_MESSAGES);
    
    // Barra de progresso para mensagens
    drawProgressBar(120, 78, 180, 15, numMensagens, MAX_MESSAGES, COLOR_HIGHLIGHT);
    
    // Informações de memória
    tft->setCursor(20, 110);
    tft->print("Memoria livre: ");
    tft->print(ESP.getFreeHeap());
    tft->print(" bytes");
    
    // Status WiFi
    tft->setCursor(20, 140);
    tft->print("WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
        tft->setTextColor(COLOR_BUTTON);
        tft->print("Conectado");
        tft->setTextColor(COLOR_TEXT);
        
        tft->setCursor(20, 160);
        tft->print("IP: ");
        tft->print(WiFi.localIP().toString());
    } else {
        tft->setTextColor(0xF800); // Vermelho
        tft->print("Desconectado");
        tft->setTextColor(COLOR_TEXT);
    }
    
    // Botão para voltar
    drawButton(110, 200, 100, 30, "Voltar", false);
}

// Implementação do teclado virtual
void DisplayInterface::drawVirtualKeyboard() {
    drawTitle("Teclado Virtual");
    
    // Campo de texto atual
    tft->setTextColor(COLOR_TEXT);
    tft->setTextSize(1);
    tft->setCursor(20, 40);
    tft->print("Texto: ");
    
    // Exibir o texto atual
    tft->drawRect(70, 40, 230, 25, COLOR_HIGHLIGHT);
    tft->fillRect(71, 41, 228, 23, COLOR_INPUT_BG);
    tft->setCursor(75, 50);
    tft->print(activeTextField->text);
    
    // Desenhar cursor
    int cursorX = 75 + activeTextField->cursorPos * 6;
    tft->drawFastVLine(cursorX, 43, 19, COLOR_HIGHLIGHT);
    
    // Desenhar teclado
    char keys[4][11] = {
        {'1','2','3','4','5','6','7','8','9','0','\0'},
        {'q','w','e','r','t','y','u','i','o','p','\0'},
        {'a','s','d','f','g','h','j','k','l','\0','\0'},
        {'z','x','c','v','b','n','m','.','-','\0','\0'}
    };
    
    // Desenhar as teclas
    tft->setTextColor(COLOR_TEXT);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 10; col++) {
            if (keys[row][col] != '\0') {
                int x = 10 + col * 30;
                int y = 100 + row * 30;
                
                tft->drawRect(x, y, 28, 28, COLOR_BORDER);
                tft->fillRect(x + 1, y + 1, 26, 26, COLOR_BUTTON);
                
                tft->setCursor(x + 11, y + 10);
                tft->print(keys[row][col]);
            }
        }
    }
    
    // Desenhar botões de controle
    tft->drawRect(10, 220, 60, 20, COLOR_BORDER);
    tft->fillRect(11, 221, 58, 18, COLOR_BUTTON);
    tft->setCursor(15, 225);
    tft->print("Espaco");
    
    tft->drawRect(80, 220, 60, 20, COLOR_BORDER);
    tft->fillRect(81, 221, 58, 18, COLOR_BUTTON);
    tft->setCursor(85, 225);
    tft->print("Apagar");
    
    tft->drawRect(150, 220, 60, 20, COLOR_BORDER);
    tft->fillRect(151, 221, 58, 18, COLOR_BUTTON);
    tft->setCursor(155, 225);
    tft->print("Limpar");
    
    tft->drawRect(220, 220, 90, 20, COLOR_BORDER);
    tft->fillRect(221, 221, 88, 18, COLOR_HIGHLIGHT);
    tft->setCursor(255, 225);
    tft->print("OK");
}

void DisplayInterface::drawReadMessages() {
    // Esta função é chamada quando o estado atual é LER_MENSAGENS
    // Mas o conteúdo real é fornecido pelo método showReadMessages
    // que precisa dos dados de usuários e mensagens
}

// Métodos getters para facilitar o acesso ao estado atual do display
bool DisplayInterface::isDecryptMode() {
    return decryptMode;
}

int DisplayInterface::getSelectedUser() {
    return selectedUser;
}

// Método para definir a variável numUsuarios para uso interno
void DisplayInterface::setNumUsuarios(int num) {
    numUsuarios = num;
}

void DisplayInterface::clearAllFields() {
    memset(usernameField.text, 0, sizeof(usernameField.text));
    usernameField.cursorPos = 0;
}

// Método para o estado normal de leitura de mensagens
void DisplayInterface::setDecryptMode(bool mode) {
    decryptMode = mode;
    tft->fillScreen(COLOR_BACKGROUND);
    update();
}

// Método para resetar a seleção de usuário
void DisplayInterface::resetUserSelection() {
    selectedUser = -1;
    currentPage = 0;
}

// Verifica se o cartão RFID está sendo usado corretamente
bool DisplayInterface::showRFIDReadingStatus(bool success) {
    if (success) {
        showMessage("Cartao RFID lido com sucesso!");
    } else {
        showMessage("Falha na leitura do cartao RFID!\nTente novamente.");
    }
    return success;
}

// Método para exibir mensagens descriptografadas
void DisplayInterface::showDecryptedMessages(const char* messages) {
    clearMessageBuffer();
    appendMessage(messages);
    setState(LER_MENSAGEM_DETALHE);
}