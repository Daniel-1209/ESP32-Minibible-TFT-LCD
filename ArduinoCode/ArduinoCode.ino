#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

// =====================================================
// OBJETOS
// =====================================================

TFT_eSPI tft = TFT_eSPI();

// =====================================================
// PINES
// =====================================================

// Joystick
#define JOY_UP     13
#define JOY_DOWN   12
#define JOY_LEFT   14
#define JOY_RIGHT  27
#define JOY_BTN    26

// TFT
#define TFT_CS_PIN 5

// SD
#define SD_CS 15

// SPI ESP32
#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23

// =====================================================
// RUTAS
// =====================================================

#define JPG_MENU "/img/star.jpg"
#define TXT_CAPITULO "/RVR1960/1_CORI/1.txt"

// =====================================================
// ESTADOS
// =====================================================

enum EstadoSistema {
  MENU,
  LEYENDO
};

EstadoSistema estadoActual = MENU;

// =====================================================
// VARIABLES
// =====================================================

File archivoActual;

uint32_t posicionArchivo = 0;

unsigned long ultimoTiempo = 0;

// =====================================================
// CALLBACK JPEG
// =====================================================

bool tft_output(
  int16_t x,
  int16_t y,
  uint16_t w,
  uint16_t h,
  uint16_t *bitmap
) {
  if (y >= tft.height()) {
    return false;
  }

  tft.pushImage(x, y, w, h, bitmap);

  return true;
}

// =====================================================
// PREPARAR SPI PARA SD
// =====================================================

bool iniciarSD() {

  // TFT desactivada
  pinMode(TFT_CS_PIN, OUTPUT);
  digitalWrite(TFT_CS_PIN, HIGH);

  // SD desactivada
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  delay(50);

  // Reiniciar SPI
  SPI.end();

  delay(100);

  SPI.begin(
    SPI_SCK,
    SPI_MISO,
    SPI_MOSI,
    -1
  );

  delay(100);

  Serial.println("Inicializando SD...");

  // Velocidad baja para máxima estabilidad
  if (!SD.begin(SD_CS, SPI, 4000000)) {

    Serial.println("ERROR: SD.begin() fallo");

    return false;
  }

  Serial.println("SD OK");

  return true;
}

// =====================================================
// COMPROBAR ARCHIVOS
// =====================================================

void comprobarArchivos() {

  Serial.println();
  Serial.println("================================");
  Serial.println("COMPROBANDO ARCHIVOS");
  Serial.println("================================");

  if (SD.exists(JPG_MENU)) {

    Serial.print("JPG OK: ");
    Serial.println(JPG_MENU);

    File jpg = SD.open(JPG_MENU, FILE_READ);

    if (jpg) {
      Serial.print("Tamano JPG: ");
      Serial.print(jpg.size());
      Serial.println(" bytes");

      jpg.close();
    }

  } else {

    Serial.print("JPG NO ENCONTRADO: ");
    Serial.println(JPG_MENU);
  }

  if (SD.exists(TXT_CAPITULO)) {

    Serial.print("TXT OK: ");
    Serial.println(TXT_CAPITULO);

    File txt = SD.open(TXT_CAPITULO, FILE_READ);

    if (txt) {

      Serial.print("Tamano TXT: ");
      Serial.print(txt.size());
      Serial.println(" bytes");

      txt.close();

    } else {

      Serial.println("ERROR abriendo TXT");
    }

  } else {

    Serial.print("TXT NO ENCONTRADO: ");
    Serial.println(TXT_CAPITULO);
  }
}

// =====================================================
// MOSTRAR MENU
// =====================================================

void mostrarMenu() {

  // Asegurar estado correcto del SPI
  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(SD_CS, HIGH);

  tft.fillScreen(TFT_BLACK);

  Serial.println();
  Serial.println("Cargando imagen del menu...");

  // Dibujar JPG desde SD
  bool resultado = TJpgDec.drawSdJpg(
    0,
    0,
    JPG_MENU
  );

  // Desactivar SD después de leer
  digitalWrite(SD_CS, HIGH);

  // Desactivar TFT
  digitalWrite(TFT_CS_PIN, HIGH);

  if (resultado) {

    Serial.println("JPG mostrado correctamente");

  } else {

    Serial.println("ERROR: JPG no encontrado o no se pudo decodificar");
  }

  // Texto inferior
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  tft.fillRect(
    0,
    105,
    tft.width(),
    35,
    TFT_BLACK
  );

  tft.setCursor(5, 112);
  tft.println("OK = Leer");

  tft.setCursor(5, 125);
  tft.println("LEFT = Menu");
}

// =====================================================
// ABRIR CAPITULO
// =====================================================

void abrirCapitulo(const char *ruta) {

  if (archivoActual) {
    archivoActual.close();
  }

  // Asegurar TFT desactivada antes de SD
  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(SD_CS, HIGH);

  Serial.println();
  Serial.print("Abriendo TXT: ");
  Serial.println(ruta);

  archivoActual = SD.open(
    ruta,
    FILE_READ
  );

  if (!archivoActual) {

    Serial.println("ERROR: No se pudo abrir TXT");

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);

    tft.setCursor(5, 5);
    tft.println("Error abriendo TXT");

    tft.setTextColor(TFT_WHITE);
    tft.setCursor(5, 20);
    tft.println(ruta);

    return;
  }

  Serial.println("TXT abierto correctamente");

  Serial.print("Tamano: ");
  Serial.print(archivoActual.size());
  Serial.println(" bytes");

  posicionArchivo = 0;

  cargarPagina();
}

// =====================================================
// CARGAR PAGINA
// =====================================================

void cargarPagina() {

  if (!archivoActual) {
    return;
  }

  if (!archivoActual.available()) {

    Serial.println("Fin del archivo");

    return;
  }

  // TFT activa
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextWrap(true);

  // SD activa
  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(SD_CS, LOW);

  archivoActual.seek(posicionArchivo);

  int lineasImpresas = 0;
  int charsEnLinea = 0;

  while (
    archivoActual.available() &&
    lineasImpresas < 16
  ) {

    char c = archivoActual.read();

    // Volver a seleccionar TFT para escribir
    digitalWrite(SD_CS, HIGH);
    digitalWrite(TFT_CS_PIN, LOW);

    if (c == '\r') {
      digitalWrite(TFT_CS_PIN, HIGH);
      continue;
    }

    tft.print(c);

    digitalWrite(TFT_CS_PIN, HIGH);

    charsEnLinea++;

    if (c == '\n' || charsEnLinea >= 26) {

      lineasImpresas++;

      charsEnLinea = 0;
    }

    // Volver a seleccionar SD
    digitalWrite(TFT_CS_PIN, HIGH);
    digitalWrite(SD_CS, LOW);
  }

  digitalWrite(SD_CS, HIGH);
  digitalWrite(TFT_CS_PIN, HIGH);

  posicionArchivo = archivoActual.position();

  Serial.print("Posicion archivo: ");
  Serial.println(posicionArchivo);
}

// =====================================================
// LEER BOTON
// =====================================================

bool leerBoton(uint8_t pin) {

  if (digitalRead(pin) == LOW) {

    if (millis() - ultimoTiempo > 250) {

      ultimoTiempo = millis();

      return true;
    }
  }

  return false;
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" ESP32 + ST7735 + SD + JOYSTICK");
  Serial.println("================================");

  // ===================================================
  // JOYSTICK
  // ===================================================

  pinMode(JOY_UP, INPUT_PULLUP);
  pinMode(JOY_DOWN, INPUT_PULLUP);
  pinMode(JOY_LEFT, INPUT_PULLUP);
  pinMode(JOY_RIGHT, INPUT_PULLUP);
  pinMode(JOY_BTN, INPUT_PULLUP);

  // ===================================================
  // CS
  // ===================================================

  pinMode(TFT_CS_PIN, OUTPUT);
  digitalWrite(TFT_CS_PIN, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // ===================================================
  // SPI INICIAL
  // ===================================================

  SPI.begin(
    SPI_SCK,
    SPI_MISO,
    SPI_MOSI,
    -1
  );

  delay(100);

  // ===================================================
  // TFT
  // ===================================================

  Serial.println("Inicializando TFT...");

  tft.init();

  tft.setRotation(1);

  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(SD_CS, HIGH);

  tft.fillScreen(TFT_BLACK);

  Serial.println("TFT OK");

  // ===================================================
  // SD
  // ===================================================

  if (!iniciarSD()) {

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);

    tft.setCursor(5, 5);
    tft.println("ERROR SD");

    tft.setCursor(5, 20);
    tft.println("No se pudo iniciar");

    while (true) {
      delay(1000);
    }
  }

  // ===================================================
  // COMPROBAR ARCHIVOS
  // ===================================================

  comprobarArchivos();

  // ===================================================
  // JPEG DECODER
  // ===================================================

  TJpgDec.setJpgScale(1);

  TJpgDec.setSwapBytes(true);

  TJpgDec.setCallback(tft_output);

  // ===================================================
  // MENU
  // ===================================================

  mostrarMenu();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // MENU
  // ===================================================

  if (estadoActual == MENU) {

    if (leerBoton(JOY_BTN)) {

      Serial.println();
      Serial.println("BOTON OK");

      estadoActual = LEYENDO;

      abrirCapitulo(TXT_CAPITULO);
    }
  }

  // ===================================================
  // LECTURA
  // ===================================================

  else if (estadoActual == LEYENDO) {

    // Pagina siguiente
    if (leerBoton(JOY_DOWN)) {

      Serial.println("PAGINA SIGUIENTE");

      cargarPagina();
    }

    // Pagina anterior
    if (leerBoton(JOY_UP)) {

      Serial.println("BOTON UP");
    }

    // Regresar al menu
    if (leerBoton(JOY_LEFT)) {

      Serial.println("VOLVIENDO AL MENU");

      if (archivoActual) {
        archivoActual.close();
      }

      estadoActual = MENU;

      // Reiniciar SD antes de volver a usar JPG
      digitalWrite(TFT_CS_PIN, HIGH);
      digitalWrite(SD_CS, HIGH);

      mostrarMenu();
    }
  }
}
