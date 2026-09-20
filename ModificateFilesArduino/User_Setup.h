#define USER_SETUP_INFO "User_Setup"

// 1. Definir el driver correcto
#define ST7735_DRIVER

// 2. Definir la resolución y el tipo de panel (muy importante para el módulo rojo)
#define TFT_WIDTH  128
#define TFT_HEIGHT 160
#define ST7735_GREENTAB2 // o intenta con ST7735_REDTAB si los colores salen invertidos luego

// 3. Definir los pines del ESP32
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

// 4. Pines estándar SPI del ESP32 (MOSI y SCLK)
#define TFT_MOSI 23
#define TFT_SCLK 18

// 5. Configuración de fuentes y velocidad
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SPI_FREQUENCY  27000000