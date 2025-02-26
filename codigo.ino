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
   #include <icons.h>
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

void setup() {
  tft.begin();
  tft.setCursor(0,0);
}
void loadingScreen(){
   if(ui_init == 0){
      tft.fillScreen(BLACK);
      tft.setCursor(PAGE_1_CENTER_X, PAGE_1_CENTER_Y);
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.println("UNLOCKEY");
      ui_init = 1;
   }
   if(step == 0){
      tft.fillrect(PAGE_1_LEFTCORNER_X, PAGE_1_LEFTCORNER_Y, PAGE_1_RECT_WIDHT, PAGE_1_RECT_HEIGHT, PAGE_1_RECT_COLOR);
   }
   if(step == 1){
      tft.fillrect(PAGE_1_LEFTCORNER_X + PAGE_1_RECT_SPACING, PAGE_1_LEFTCORNER_Y, PAGE_1_RECT_WIDHT, PAGE_1_RECT_HEIGHT, PAGE_1_RECT_COLOR);
   } 
   if(step == 2){
      tft.fillrect(PAGE_1_LEFTCORNER_X + 2*PAGE_1_RECT_SPACING, PAGE_1_LEFTCORNER_Y, PAGE_1_RECT_WIDHT, PAGE_1_RECT_HEIGHT, PAGE_1_RECT_COLOR);
   }
   if(step == 3){
      tft.fillrect(PAGE_1_LEFTCORNER_X + 3*PAGE_1_RECT_SPACING, PAGE_1_LEFTCORNER_Y, PAGE_1_RECT_WIDHT, PAGE_1_RECT_HEIGHT, PAGE_1_RECT_COLOR);
   }
      
}

void mainUi(){

}
void graphicEngine(int page, int parameter){
   //PAGE 1: LOADING SCREEN
   switch(page){
         case 1:
            loadingScreen();
            break;
         case 2:
            mainUi();
            break;
   }
}
void loop() {
   actualPage = 1;
//loop() is resposible to reset temporary io_trehsoldPast, ui_init and global_thresholdPast when graphic engine is called with another page
thresholdTimer = millis();
graphicEngine(actualPage, 0);


timeThreshold();

}
