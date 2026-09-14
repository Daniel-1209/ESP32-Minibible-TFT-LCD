#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI(); 

// Pines del Joystick
#define JOY_UP 13
#define JOY_DOWN 12
#define JOY_LEFT 14
#define JOY_RIGHT 27
#define JOY_BTN 26
#define SD_CS 15

enum EstadoSistema { MENU, LEYENDO };
EstadoSistema estadoActual = MENU;

File archivoActual;
uint32_t posicionArchivo = 0; // Guarda en qué byte del archivo .txt nos quedamos

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void setup() {
  Serial.begin(115200);
  
  pinMode(JOY_UP, INPUT_PULLUP);
  pinMode(JOY_DOWN, INPUT_PULLUP);
  pinMode(JOY_LEFT, INPUT_PULLUP);
  pinMode(JOY_RIGHT, INPUT_PULLUP);
  pinMode(JOY_BTN, INPUT_PULLUP);

  
  // Primero iniciamos sd despues la pantalla sino dara error en la sd
  if (!SD.begin(SD_CS)) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.drawString("Error SD", 10, 10, 1);
    while (1);
  }

  tft.init();
  tft.setRotation(1); // Orientación horizontal (160x128)

  TJpgDec.setJpgScale(1); 
  TJpgDec.setSwapBytes(true); 
  TJpgDec.setCallback(tft_output);

  mostrarMenu();
}

void loop() {
  // Lógica de Máquina de Estados
  if (estadoActual == MENU) {
    if (leerBoton(JOY_BTN)) {
      // Al presionar OK, abrimos un capítulo y cambiamos de estado
      estadoActual = LEYENDO;
      abrirCapitulo("/RVR1960/1_CORI/1.txt");
    }
  } 
  else if (estadoActual == LEYENDO) {
    if (leerBoton(JOY_DOWN)) {
      // Avanzar a la siguiente página del texto
      cargarPagina();
    }
    if (leerBoton(JOY_LEFT)) {
      // Volver al menú
      if(archivoActual) archivoActual.close();
      estadoActual = MENU;
      mostrarMenu();
    }
  }
}

void mostrarMenu() {
  tft.fillScreen(TFT_BLACK);
  // Aquí puedes cargar tu imagen aesthetic de portada
  TJpgDec.drawSdJpg(0, 0, "/img/star.jpg");
  
  // Dibujar UI sobre la imagen (rectángulo semi-transparente no es posible nativamente, 
  // pero podemos poner un cuadro negro)
  tft.fillRect(10, 90, 140, 30, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Presiona OK para leer", 15, 100, 1);
}

void abrirCapitulo(const char* ruta) {
  if(archivoActual) archivoActual.close();
  
  archivoActual = SD.open(ruta);
  if (!archivoActual) {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Error abriendo TXT", 0, 0, 1);
    return;
  }
  posicionArchivo = 0; 
  cargarPagina();
}

void cargarPagina() {
  if (!archivoActual || !archivoActual.available()) return;

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextWrap(true); // Fundamental para que el texto baje a la siguiente línea

  archivoActual.seek(posicionArchivo); // Saltamos a donde nos quedamos la última vez

  int lineasImpresas = 0;
  int charsEnLinea = 0;

  // Leemos hasta llenar la pantalla (aprox 16 líneas)
  while (archivoActual.available() && lineasImpresas < 16) {
    char c = archivoActual.read();
    tft.print(c);
    charsEnLinea++;

    if (c == '\n' || charsEnLinea >= 26) { 
      lineasImpresas++;
      charsEnLinea = 0;
    }
  }
  
  // Guardamos la posición exacta del byte para la siguiente página
  posicionArchivo = archivoActual.position(); 
}

// Variables para el control de botones (debounce no bloqueante)
unsigned long ultimoTiempo = 0;
bool leerBoton(uint8_t pin) {
  if (digitalRead(pin) == LOW) {
    if (millis() - ultimoTiempo > 250) {
      ultimoTiempo = millis();
      return true;
    }
  }
  return false;
}