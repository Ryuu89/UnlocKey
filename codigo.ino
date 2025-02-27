/*
DEFINED PINS FOR ILI9341: --> LOOK TO USER_SETUP.H FILE IN TFT_ESPI LIBRARY
TFT_MISO  19  // SPI MISO
TFT_MOSI  23  // SPI MOSI
TFT_SCLK  18  // SPI Clock
TFT_CS    5   // Chip Select
TFT_DC    21  // Data/Command
TFT_RST   22  // Reset 
*/

/*

Use this calibration code in setup():
uint16_t calData[5] = { 149, 3536, 268, 3569, 1 };
tft.setTouch(calData);
*/

#include <TFT_eSPI.h>  // Include the TFT_eSPI library
#include <Ui_defines.h>
#include <cstring>
// #include <icons.h>
 // define some RGB 585 colors
#define BLACK    0x0000
#define WHITE    0xFFFF
#define RED      0xF800
#define GREEN    0x07E0
#define BLUE     0x001F
#define MAGENTA  0xF81F
#define ORANGE   0xFBE0
#define GREY     0x5AEB

#define TOUCH_CLK 18
#define TOUCH_CS 21
#define TOUCH_DIN 23
#define TOUCH_DO 19

//  UI DEFINES
 #define MAINSCREEN 0
 #define DEMO_MODE 1
 TFT_eSPI tft = TFT_eSPI();
 //TIME-RELATED VARIABLES --> ALL PARAMETERS KEEP IN MILLISECONDS
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
//SYSTEM-RELATED VARIABLES --> think to be printed in UI
char systemStatus[50];
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
}
if(actualTimerLimit != -1){
if(animationCursor >= actualTimerLimit){
    reset_ui_variables();
    actualPage = DEFAULT_PAGE;
}
}
switch(actualPage){
       case 1:
          loadingScreen();
          break;
       case 2: 
          mainUi();
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

}

void drawReadRFIDNoMessages(){ //PAGE 3
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

}
void drawReadRFIDUUID(char *uid){ //PAGE 6
tft.drawRoundRect(PAGE_3_RECTANGLE_LEFTCORNER_X, PAGE_3_RECTANGLE_LEFTCORNER_Y, PAGE_3_RECTANGLE_WIDTH, PAGE_3_RECTANGLE_HEIGHT, PAGE_3_RECTANGLE_RADIUS, PAGE_3_RECTANGLE_COLOR);
tft.setCursor(PAGE_3_TEXT_CENTER_X -51, PAGE_3_TEXT_CENTER_Y);
tft.setTextSize(PAGE_3_TEXT_SIZE);
tft.setTextColor(PAGE_3_TEXT_COLOR);
tft.print("UID: ");
tft.print(uid);
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

void drawShowMessage(char *message){ //PAGE 4
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
void setup() {
Serial.begin(9600);
tft.begin();
tft.setCursor(0,0);
tft.setRotation(3);
ui_thresholdPast = millis();
actualPage = 1;
setSystemStatus(1, "Inicializando");
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
 }
  //UPDATING SYSTEM STATUS
  tft.setCursor(PAGE_2_TEXT_CENTER_X, PAGE_2_TEXT_CENTER_Y);
  tft.fillCircle(PAGE_2_CIRCLE_CENTER_X, PAGE_2_CIRCLE_CENTER_Y, PAGE_2_CIRCLE_RADIUS, getColorStatus());
  tft.setTextSize(PAGE_2_TEXT_SIZE);
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(PAGE_2_CIRCLE_CENTER_X + PAGE_2_CIRCLE_RADIUS + PAGE_2_CIRCLE_SPACING, PAGE_2_CIRCLE_CENTER_Y -5);
  tft.print(systemStatus);

}
void loop() {
update_timers();
page_controller();
//Serial.println("actualPage status is:");
if(DEMO_MODE && actualPage == 2){
  while(1){
    drawShowMessage("Deslocamento de tropas BD-21 a noroeste de S. Petsburgo. Recuar posicao");
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
    drawReadRFIDUUID("6A 2D 47 82 45 9C");
    delay(3000);
  }
}
if(ui_thresholdTimer > 15000){
  setSystemStatus(0, "Operacional");
  delay(3000);
}
}
