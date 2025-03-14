#ifndef UNLOCKEYLCDLIB_H
#define UNLOCKEYLCDLIB_H

/*
PROJECT PINS:
TFT_MISO  19  // SPI MISO
TFT_MOSI  23  // SPI MOSI
TFT_SCLK  18  // SPI Clock
TFT_CS    5   // Chip Select
TFT_DC    21  // Data/Command
TFT_RST   22  // Reset 
TOUCH_CLK 18
TOUCH_CS 4
TOUCH_DIN 23
TOUCH_DO 19

*/

#include <TFT_eSPI.h>
#include <Ui_defines.h>
#include <cstring>
#include <XPT2046_Touchscreen.h>
typedef (int*) *ptrLerCartao(char*);
typedef (int*) *ptrCadastrarCartao(char*);

class UnlockeyUI{
  public:
    UnlockeyUI(ptrLerCartao tempLerCartao, ptrCadastrarCartao tempCadastrarCartao){
      tft = TFT_eSPI();
      strcpy(UUID, "NQO65");
      //Serial.begin(115200); //USER NEED TO DO IT IN MAIN PROGRAM!!!!
      tft.begin();
      tft.setCursor(0,0);
      tft.setRotation(3);
      ui_thresholdPast = millis();
      actualPage = 1;
      setSystemStatus(1, "Inicializando");

      // DISPLAY TOUCH CALIBRATION
      uint16_t calData[5] = { 149, 3536, 268, 3569, 1 };
      tft.setTouch(calData);

      lerCartao = tempLerCartao;
      cadastrarCartao = tempCadastrarCartao;
    }

    void runScreen(){
      update_timers();
      page_controller();
      updateTouch();
        //DEBUG AREA
        if(DEBUG_PARAMS){
          Serial.print("Touch pair: (");
          Serial.print(touchX);
          Serial.print(", ");
          Serial.print(touchY);
          Serial.print(")   ");
          Serial.print("Touch Status: ");
          Serial.print(isTouch);
          Serial.print("    Actual Page: ");
          Serial.println(actualPage);
        }
  
        if(DEMO_MODE && actualPage == 2){
          while(1){
            strcpy("Deslocamento de tropas BD-21 a noroeste", message);
            drawShowMessage();
            delay(3000);
            tft.fillScreen(BLACK);
            drawReadRFID();
            delay(3000);
            tft.fillScreen(BLACK);
            drawReadRFIDNoMessages();
            delay(3000);
            tft.fillScreen(BLACK);
            actualPage = 2;
            page_controller();
            delay(3000);
            tft.fillScreen(BLACK);
            drawReadRFIDUUID();
            delay(3000);
          }
      }
      
      
    }
  private: 
    //TIME-RELATED VARIABLES --> ALL PARAMETERS KEEP IN MILLISECONDS
    const int DEMO_MODE 0
    int ui_thresholdTimer;
    int ui_thresholdPast;
    int global_thresholdTimer;
    int global_thresholdPast;
    //UI-RELATED VARIABLES
    int ui_init = 0; //designed to be used as a flag to initialize the UI when the function to page run for the first time
    int step = 0;
    int actualPage;
    int colorStatus = 1; // 0 -> GREEN // 1 -> ORANGE // 2 -> PURPLE
    int ui_updateFlag = 0;
    int ui_pastUpdateFlag = 0;
    int nextReadPage = 0; //WHEN A READ RFID FUNCTION IS CALLED, THE NEXT PAGE (MESSAGE OR REGISTERED) NEED TO BE DEFINED WITH NEXTREADPAGE
    //TOUCH-RELATED FUNCTIONS
    int isTouch = 0;
    int touchX = 0;
    int touchY = 0;
    char message[MAX_MESSAGE_SIZE];
    char UUID[15];
    long int cardModule;
    long int cardExponent;
    //SYSTEM-RELATED VARIABLES --> think to be printed in UI
    char systemStatus[50];
    ptrLerCartao lerCartao;
    ptrCadastrarCartao cadastrarCartao;
    TFT_eSPI tft;

    void setPage(int pg){
      actualPage = pg;
      ui_init = 0;
      isTouch = 0;
      reset_ui_variables();
    }
    
    void setSystemStatus(int color, char *text){
      colorStatus = color;
      strcpy(systemStatus, text);
      ui_init = 0;
      }

    int getColorStatus(){
      switch(colorStatus){
        case 0:
          return GREEN;
          break;
        case 1:
          return ORANGE;
          break;
        case 2:
          return RED;
          break;
      }
      }

    void reset_ui_variables(){
        ui_thresholdPast = millis();
        ui_init = 0;
        step = 0;
        }
      
    void page_controller(){
      int actualTimerLimit;
      int animationCursor = ui_thresholdTimer - ui_thresholdPast;
      switch(actualPage){ //If the limit is -1, there is no limit to display refered page
        case 1:
          actualTimerLimit = PAGE_1_TOTAL_STEP;
          break;
        case DEFAULT_PAGE:
          actualTimerLimit = -1;
          break;
        case 3:
          actualTimerLimit = PAGE_3_TOTAL_STEP;
          break;
        case 4:
          actualTimerLimit = PAGE_4_TOTAL_STEP;
          break;
        case 5:
          actualTimerLimit = PAGE_5_TOTAL_STEP;
          break;
        case 6:
          actualTimerLimit = PAGE_6_TOTAL_STEP;
          break;
      }
      if(actualTimerLimit != -1){
      if(animationCursor >= actualTimerLimit){
          setPage(2);
      }
      }
      switch(actualPage){
              case 1:
                loadingScreen();
                break;
              case 2: //MAIN UI
                mainUi();
                break;
              case 3: //READING RFID
                drawReadRFID();
                break;
              case 4: //MESSAGE BOX
                drawShowMessage();
                break;
              case 5: //WHEN THERE IS NO MESSAGE FOR USER
                drawReadRFIDNoMessages();
                break;
              case 6: //SHOW THE READ UID
                drawReadRFIDUUID();
                break;
        }
      }
    
    void drawRFIDIcon() {
      int x = PAGE_2_RFID_CENTER_X;
      int y = PAGE_2_RFID_CENTER_Y;
      int size = PAGE_2_RFID_SIZE;
      uint16_t color = PAGE_2_RFID_COLOR;
      // Drawing selection area
      tft.fillRoundRect(PAGE_2_DEFAULT_SELECTIONAREA_X, PAGE_2_RFID_SELECTIONAREA_Y,PAGE_2_SELECTIONAREA_WIDTH, PAGE_2_SELECTIONAREA_HEIGHT, PAGE_2_SELECTIONAREA_CORNER_RADIUS, PAGE_2_SELECTIONAREA_COLOR);
      //tft.setCursor(PAGE_2_DEFAULT_SELECTIONAREA_X + PAGE_2_SELECTIONAREA_TEXT_SPACING_X, (int)((PAGE_2_SELECTIONAREA_WIDTH/2) + PAGE_2_RFID_SELECTIONAREA_Y));
      tft.setCursor(PAGE_2_DEFAULT_SELECTIONAREA_X + PAGE_2_SELECTIONAREA_TEXT_SPACING_X, (int)(PAGE_2_RFID_SELECTIONAREA_Y + 20));
      tft.setTextColor(PAGE_2_SELECTIONAREA_TEXT_COLOR);
      tft.setTextSize(PAGE_2_SELECTIONAREA_TEXT_SIZE);
      tft.println("Cadastrar");
      // Draw RFID card (rectangle)
      tft.drawRect(x, y, size, size * 0.6, color);
      
      // Convert float to integer for Y position
      int y_center = y + (size * 3) / 10;  // Equivalent to y + size * 0.3
      
      // Draw RFID signal waves
      for (int i = 0; i < 3; i++) {
        int waveOffset = 5 + (i * 5);  // Adjust spacing for each wave
        tft.drawArc(x + size + 10, y_center, waveOffset, waveOffset + 3, 240, 300, color, TFT_BLACK, false);
      }
      
      // Draw small rectangle to represent RFID chip area
      tft.fillRect(x + size / 4, y + size / 3, size / 4, size / 6, color);
      }

    void drawTextIcon() {
      int x = PAGE_2_MESSAGEICON_LEFTCORNER_X;
      int y = PAGE_2_MESSAGEICON_LEFTCORNER_Y;
      uint16_t color = PAGE_2_MESSAGEICON_COLOR;
      int size = PAGE_2_MESSAGEICON_SIZE;
      int radius = size / 5;  // Corner roundness
      // Drawing selection area
      tft.fillRoundRect(PAGE_2_DEFAULT_SELECTIONAREA_X, PAGE_2_MESSAGE_SELECTIONAREA_Y, PAGE_2_SELECTIONAREA_WIDTH, PAGE_2_SELECTIONAREA_HEIGHT, PAGE_2_SELECTIONAREA_CORNER_RADIUS, PAGE_2_SELECTIONAREA_COLOR);
      //tft.setCursor(PAGE_2_DEFAULT_SELECTIONAREA_X + PAGE_2_SELECTIONAREA_TEXT_SPACING_X, (int)((PAGE_2_SELECTIONAREA_WIDTH/2) + PAGE_2_RFID_SELECTIONAREA_Y));
      tft.setCursor(PAGE_2_DEFAULT_SELECTIONAREA_X + PAGE_2_SELECTIONAREA_TEXT_SPACING_X, (int)(PAGE_2_MESSAGE_SELECTIONAREA_Y + 20));
      tft.setTextColor(PAGE_2_SELECTIONAREA_TEXT_COLOR);
      tft.setTextSize(PAGE_2_SELECTIONAREA_TEXT_SIZE);
      tft.println("Ler mensagem");
      
      // Draw the filled rounded rectangle
      tft.fillRoundRect(x, y, size, size, radius, color);
      
      // Define line properties
      int line_spacing = size / 5;  // Space between lines
      int text_margin = size / 6;   // Left margin for text effect
      int line_length = (size * 2) / 3;  // How long the text lines are
      
      // Draw four horizontal lines inside the icon
      for (int i = 1; i <= 4; i++) {
        int lineY = y + (i * line_spacing);
        tft.drawLine(x + text_margin, lineY, x + text_margin + line_length, lineY, TFT_WHITE);
      }
      }
    
    void drawReadRFID(){ //PAGE 3
      if(!ui_init){
      tft.fillScreen(BLACK);
      tft.drawRoundRect(PAGE_3_RECTANGLE_LEFTCORNER_X, PAGE_3_RECTANGLE_LEFTCORNER_Y, PAGE_3_RECTANGLE_WIDTH, PAGE_3_RECTANGLE_HEIGHT, PAGE_3_RECTANGLE_RADIUS, PAGE_3_RECTANGLE_COLOR);
      tft.setCursor(PAGE_3_TEXT_CENTER_X, PAGE_3_TEXT_CENTER_Y);
      tft.setTextSize(PAGE_3_TEXT_SIZE);
      tft.setTextColor(PAGE_3_TEXT_COLOR);
      tft.print("Aproxime a tag");
      //Drawing Icons
      int x = PAGE_3_RFID_CENTER_X;
      int y = PAGE_3_RFID_CENTER_Y;
      int size = PAGE_3_RFID_SIZE;
      uint16_t color = PAGE_3_RFID_COLOR;
      
      tft.drawRect(x, y, size, size * 0.6, color);
      
      // Convert float to integer for Y position
      int y_center = y + (size * 3) / 10;  // Equivalent to y + size * 0.3
      
      // Draw RFID signal waves
      for (int i = 0; i < 3; i++) {
        int waveOffset = 5 + (i * 5);  // Adjust spacing for each wave
        tft.drawArc(x + size + 10, y_center, waveOffset, waveOffset + 3, 240, 300, color, TFT_BLACK, false);
      }
      
      // Draw small rectangle to represent RFID chip area
      tft.fillRect(x + size / 4, y + size / 3, size / 4, size / 6, color);
      ui_init = 1;
      }
      /*
        -- PARA_IMPLEMENTAR -- 
      SE O CARTÃO RFID NÃO PUDER SER LIDO, PROCESSADO, E COMPARADO COM O BANCO DE DADOS DISPONÍVEL NO ESP, NÃO PRECISA FAZER NADA!! A TELA VAI SAIR SOZINHA PARA O HOME
      
      A condicional aqui é a seguinte, a próxima tela vai ser definida em torno da variável nextReadPage, porque a mesma tela de "ler o cartão RFID" vai ser exibida
      tanto para o usuário que quer cadastrar um novo cartão, quanto pelo que quer ler a mensagem. Antes de chegar aqui, a variável de qual será a próxima tela já foi definida:
      
      Se nextReadPage = 4, o que o driver gráfico vai fazer é mostrar a tela de mensagens com a string contida na variável "message". Portanto, aqui a função é ler o UID
      do RFID, buscar se há mensagens não lidas. O efeito de não ter mensagem não lida vai ser o mesmo para o usuário não cadastrado.
        --> Se não tiver mensagem para mostrar: setPage(5); --> vai para tela de "não há mensagems"
        --> Se tiver mensagem para mostrar: 
              1. Copiar a mensagem a ser mostrada para a variável "message" com strcpy(message, stringRecebida); -> tomar cuidado com o limite de MAX_MESSAGE_SIZE
              2. Usar o comando: setPage(4); --> vai para tela de "mensagem recebida", imprimindo a string presente em "message"
      
      Se nextReadPage = 6, o que o driver gráfico vai fazer é cadastrar a UID do cartão RFID, que vai ser usado para o usuário anotar e transmitir a mensagem no tópico dessa UID, além de
      gravar a chave privada do usuário no cartão RFID e também mostrar para ele. Assim, o programa vai ter condições de comparar qual é a mensagem disponível que o usuário pode ler, e o
      usuário vai ter condições de criptografar em uma ferramenta externa a mensagem usando sua chave pública
        1. Cadastrar a nova UID do cartão do usuário no banco de dados
        2. Copiar a string da nova UID para a variável "UUID" usando strcpy novamente
        3. Gerar as chaves públicas com expoente e módulo:
          1. Expoente --> copiar para a variável "cardExponent"
          2. Module --> copar para a variável "cardModule"
        4. Dar o comando: setPage(6); --> Tela que vai mostrar as informações
      
        ACABOU!!!
      */

      if(nextReadPage == 4){
          //A COISA TODA DE CIMA
          int tempState;
          tempState = lerCartao(message);
          if(tempState == -1){ //IF THERE IS NO MESSAGE TO READ OR THE USER IS NOT REGISTERED
            setPage(5);
          } else if(tempState == 1){ //THERE IS A MESSAGE, PRINT IT PLEASE!!
            setPage(4);
          }
          //IF THE ANSWER IS NONE OF ABOVE, SKIP AND KEEP RUNNING THE READ ATTEMPTS
      } else if(nextReadPage == 6){
        //A COISA TODA DE CIMA
        if(cadastrarCartao(UUID)){
            setPage();
        }
      }

      }
    
    void drawReadRFIDNoMessages(){ //PAGE 3
      if(!ui_init){
      tft.fillScreen(BLACK);
      tft.drawRoundRect(PAGE_3_RECTANGLE_LEFTCORNER_X, PAGE_3_RECTANGLE_LEFTCORNER_Y, PAGE_3_RECTANGLE_WIDTH, PAGE_3_RECTANGLE_HEIGHT, PAGE_3_RECTANGLE_RADIUS, PAGE_3_RECTANGLE_COLOR);
      tft.setCursor(PAGE_3_TEXT_CENTER_X, PAGE_3_TEXT_CENTER_Y);
      tft.setTextSize(PAGE_3_TEXT_SIZE);
      tft.setTextColor(PAGE_3_TEXT_COLOR);
      tft.print("Sem mensagens");
      //Drawing Icons
      int x = PAGE_3_RFID_CENTER_X;
      int y = PAGE_3_RFID_CENTER_Y;
      int size = PAGE_3_RFID_SIZE;
      uint16_t color = PAGE_3_RFID_COLOR;
      
        tft.drawRect(x, y, size, size * 0.6, color);
      
      // Convert float to integer for Y position
      int y_center = y + (size * 3) / 10;  // Equivalent to y + size * 0.3
      
      // Draw RFID signal waves
      for (int i = 0; i < 3; i++) {
        int waveOffset = 5 + (i * 5);  // Adjust spacing for each wave
        tft.drawArc(x + size + 10, y_center, waveOffset, waveOffset + 3, 240, 300, color, TFT_BLACK, false);
      }
      
      // Draw small rectangle to represent RFID chip area
      tft.fillRect(x + size / 4, y + size / 3, size / 4, size / 6, color);
      ui_init = 1;
      delay(300);
      }
      }
    
    void drawReadRFIDUUID(){ //PAGE 6
      tft.drawRoundRect(PAGE_3_RECTANGLE_LEFTCORNER_X, PAGE_3_RECTANGLE_LEFTCORNER_Y, PAGE_3_RECTANGLE_WIDTH, PAGE_3_RECTANGLE_HEIGHT, PAGE_3_RECTANGLE_RADIUS, PAGE_3_RECTANGLE_COLOR);
      tft.setCursor(PAGE_3_TEXT_CENTER_X -51, PAGE_3_TEXT_CENTER_Y);
      tft.setTextSize(PAGE_3_TEXT_SIZE);
      tft.setTextColor(PAGE_3_TEXT_COLOR);
      tft.print("UID: ");
      tft.println(UUID);
      tft.print("E:");
      tft.println(cardExponent);
      tft.print("M:");
      tft.println(cardModule);
      //Drawing Icons
      int x = PAGE_3_RFID_CENTER_X;
      int y = PAGE_3_RFID_CENTER_Y;
      int size = PAGE_3_RFID_SIZE;
      uint16_t color = PAGE_3_RFID_COLOR;
      
        tft.drawRect(x, y, size, size * 0.6, color);
      
      // Convert float to integer for Y position
      int y_center = y + (size * 3) / 10;  // Equivalent to y + size * 0.3
      
      // Draw RFID signal waves
      for (int i = 0; i < 3; i++) {
        int waveOffset = 5 + (i * 5);  // Adjust spacing for each wave
        tft.drawArc(x + size + 10, y_center, waveOffset, waveOffset + 3, 240, 300, color, TFT_BLACK, false);
      }
      
      // Draw small rectangle to represent RFID chip area
      tft.fillRect(x + size / 4, y + size / 3, size / 4, size / 6, color);
      
      }
    
    void drawShowMessage(){ //PAGE 4
      tft.fillScreen(PAGE_4_BACKGROUND_COLOR);
      tft.fillRect(0,0, tft.width(), PAGE_4_RECTANGLE_HEIGHT, PAGE_4_RECTANGLE_COLOR);
      tft.setCursor(PAGE_4_TEXT_CENTER_X, PAGE_4_TEXT_CENTER_Y);
      tft.setTextSize(PAGE_4_TEXT_SIZE);
      tft.setTextColor(PAGE_4_TEXT_COLOR);
      tft.print("MENSAGEM RECEBIDA");
      tft.setCursor(PAGE_4_MESSAGETEXT_CENTER_X, PAGE_4_MESSAGETEXT_CENTER_Y);
      tft.setTextSize(PAGE_4_MESSAGETEXT_SIZE);
      tft.setTextColor(PAGE_4_MESSAGETEXT_COLOR);
      tft.print(message);
      }
    
    void update_timers(){
      global_thresholdTimer = millis();
      ui_thresholdTimer = millis();
      }
    
    void loadingScreen(){
      //static
      int animationCursor = (ui_thresholdTimer - ui_thresholdPast);
      float proportion = ((float)animationCursor/(float)PAGE_1_ANIMATION_DURATION);
      if(proportion > 1)
        proportion = 1;
      proportion = proportion * proportion;
      int widhtToDraw = proportion * PAGE_1_RECT_MAX_SIZE;
        if(ui_init == 0){
          tft.fillScreen(BLACK);
          tft.setCursor(PAGE_1_TEXT_CENTER_X, PAGE_1_TEXT_CENTER_Y);
          tft.setTextColor(WHITE);
          tft.setTextSize(5);
          tft.println("UNLOCKEY");
          ui_init = 1;
        }
        //dynamic
        tft.fillRect(PAGE_1_LEFTCORNER_X, PAGE_1_LEFTCORNER_Y, widhtToDraw, PAGE_1_RECT_HEIGHT, PAGE_1_RECT_COLOR);
        if(animationCursor >= PAGE_1_STEP_2){
        if(ui_init == 1){
          ui_init++;
          tft.fillScreen(BLACK);
          tft.setCursor(PAGE_1_TEXT_CENTER_X + 40, PAGE_1_TEXT_CENTER_Y);
          tft.setTextColor(WHITE);
          tft.setTextSize(5);
          tft.println("READY");
        }
        }
    }
  
    void mainUi(){
      if(ui_init == 0 || DEMO_MODE == 1){
         tft.fillScreen(BLACK);
         ui_init = 1;
         drawRFIDIcon();
         drawTextIcon();
         tft.setCursor(PAGE_2_TEXT_CENTER_X, PAGE_2_TEXT_CENTER_Y);
         tft.fillCircle(PAGE_2_CIRCLE_CENTER_X, PAGE_2_CIRCLE_CENTER_Y, PAGE_2_CIRCLE_RADIUS, getColorStatus());
         tft.setTextSize(PAGE_2_TEXT_SIZE);
         tft.setTextColor(TFT_BLUE);
         tft.setCursor(PAGE_2_CIRCLE_CENTER_X + PAGE_2_CIRCLE_RADIUS + PAGE_2_CIRCLE_SPACING, PAGE_2_CIRCLE_CENTER_Y -5);
         tft.print(systemStatus);
      }
     
      // IMPLEMENT THE ONLY TOUCH AREA SENSIBLE OF ALL UI
      if(isTouch){
       //CHECKING THE "LER MENSAGEM" BUTTON
       if((touchX > PAGE_2_DEFAULT_SELECTIONAREA_X) && (touchX < (PAGE_2_DEFAULT_SELECTIONAREA_X + PAGE_2_SELECTIONAREA_WIDTH)) && (touchY > PAGE_2_MESSAGE_SELECTIONAREA_Y) && (touchY < (PAGE_2_MESSAGE_SELECTIONAREA_Y + PAGE_2_SELECTIONAREA_HEIGHT))){
         setPage(3);
         nextReadPage = 4;
       } else if((touchX > PAGE_2_DEFAULT_SELECTIONAREA_X) && (touchX < (PAGE_2_DEFAULT_SELECTIONAREA_X + PAGE_2_SELECTIONAREA_WIDTH)) && (touchY > PAGE_2_RFID_SELECTIONAREA_Y) && (touchY < (PAGE_2_RFID_SELECTIONAREA_Y + PAGE_2_SELECTIONAREA_HEIGHT))){
         setPage(3);
         nextReadPage = 6;
      }
      }
     }
    void updateTouch(){
    uint16_t tempX, tempY;
    int tempIsTouch;
    isTouch = tft.getTouch(&tempX, &tempY);
    if(isTouch){
      tempX = map(tempX, 0, 330, 320, 0);
      tempY = map(tempY, 0, 240, 240, 0);
      touchX = tempX;
      touchY = tempY;
      isTouch = 1;
    }
    }
     
}

#endif