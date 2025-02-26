//ALL VALUES ARE RELATED TO ORIENTATION = 3

#define U_BLACK    0x0000
#define U_WHITE    0xFFFF
#define U_RED      0xF800
#define U_GREEN    0x07E0
#define U_BLUE     0x001F
#define U_MAGENTA  0xF81F
#define U_ORANGE   0xFBE0
#define U_GREY     0x5AEB
/* ORIENTATION FOR SET ROTATION

--> +X

y
\/

*/
//PAGE 1 DEFINES
#define PAGE_1_LEFTCORNER_X 40
#define PAGE_1_LEFTCORNER_Y 150
#define PAGE_1_TEXT_CENTER_X 40
#define PAGE_1_TEXT_CENTER_Y 100
#define PAGE_1_ANIMATION_DURATION 3000 //Duration for the loading animation
#define PAGE_1_RECT_HEIGHT 20
#define PAGE_1_RECT_MAX_SIZE 230
#define PAGE_1_RECT_COLOR U_BLUE
#define PAGE_1_STEP_2 5000 //How many stay in the only loading screen
#define PAGE_1_TOTAL_STEP 5500 //How many stay in this page
#define DEFAULT_PAGE 2

//PAGE 2 DEFINES
#define PAGE_2_TEXT_CENTER_X 40
#define PAGE_2_TEXT_CENTER_Y 40
#define PAGE_2_TEXT_SIZE 2
#define PAGE_2_CIRCLE_CENTER_X 100
#define PAGE_2_CIRCLE_CENTER_Y 100
#define PAGE_2_CIRCLE_RADIUS 13