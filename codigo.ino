
//  ESP8266_ILI9341_Adafruit_Bodmers_clock
//
// Interfacing ESP8266 NodeMCU with ILI9341 TFT display (240x320 pixel).
// clock is adaptation of Bodmer's Clock example in the TFT_eSPI librray 
//
//
// pins: TFT SPI ILI9341 to ESP8266 NodeMCU
// VCC       ------------     VCC  - note- wemos - 5V
// GND       ------------     GND 
// CS        ------------     D2
// RST       ------------     D3
// D/C       ------------     D4
// MOSI      ------------     D7  
// SCK       ------------     D5       
// BL        ------------     VCC - wnote - emos 5V
//
// open source - thanks to all contributors
 
   #include <Adafruit_GFX.h>                                                    // include Adafruit graphics library
   #include <Adafruit_ILI9341.h>                                                // include Adafruit ILI9341 TFT library
 
   #define TFT_CS    D2                                                         // TFT CS  pin is connected to NodeMCU pin D2
   #define TFT_RST   D3                                                         // TFT RST pin is connected to NodeMCU pin D3
   #define TFT_DC    D4                                                         // TFT DC  pin is connected to NodeMCU pin D4

// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)

   Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
   int 
// define some RGB 585 colors
   #define BLACK    0x0000
   #define WHITE    0xFFFF
   #define RED      0xF800
   #define GREEN    0x07E0
   #define BLUE     0x001F
   #define MAGENTA  0xF81F
   #define ORANGE   0xFBE0
   #define GREY     0x5AEB
 
void setup() {
  tft.begin();
 
}
void loadingScreen();
void graphicEngine(int page, int parameter){
   //PAGE 1: LOADING SCREEN
}
void loop(void) {


}
