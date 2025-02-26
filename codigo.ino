/*
DEFINED PINS FOR ILI9341: --> LOOK TO USER_SETUP.H FILE IN TFT_ESPI LIBRARY
TFT_MISO  19  // SPI MISO
TFT_MOSI  23  // SPI MOSI
TFT_SCLK  18  // SPI Clock
TFT_CS    5   // Chip Select
TFT_DC    21  // Data/Command
TFT_RST   22  // Reset 
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
//  UI DEFINES
#define MAINSCREEN 0

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
//SYSTEM-RELATED VARIABLES --> think to be printed in UI
char systemStatus[50];
void setSystemStatus(int color, char *text){
colorStatus = color;
strcpy(systemStatus, text);
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
if(ui_init == 0){
   tft.fillScreen(BLACK);
   ui_init = 1;
   tft.setCursor(PAGE_2_TEXT_CENTER_X, PAGE_2_TEXT_CENTER_Y);
   tft.fillCircle(PAGE_2_CIRCLE_CENTER_X, PAGE_2_CIRCLE_CENTER_Y, PAGE_2_CIRCLE_RADIUS, getColorStatus());
   tft.setTextSize(PAGE_2_TEXT_SIZE);
   tft.setTextColor(TFT_BLUE);
   tft.setCursor(50, 50);
   tft.println(systemStatus);
}
}
void loop() {
page_controller();
//Serial.println("actualPage status is:");
update_timers();
}
