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
#define JOY_UP 13
#define JOY_DOWN 12
#define JOY_LEFT 14
#define JOY_RIGHT 27
#define JOY_BTN 26

// TFT
#define TFT_CS_PIN 5

// SD
#define SD_CS 15

// SPI ESP32
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

// =====================================================
// ARCHIVOS
// =====================================================

#define JPG_MENU "/img/star.jpg"

// =====================================================
// CONFIGURACION
// =====================================================

#define MAX_VERSIONES 10
#define MAX_LIBROS 80
#define MAX_CAPITULOS 200

// Historial de páginas
#define MAX_PAGINAS_HISTORIAL 1000

// =====================================================
// ESTADOS
// =====================================================

enum EstadoSistema {
  MENU_VERSION,
  MENU_LIBRO,
  MENU_CAPITULO,
  LEYENDO
};

EstadoSistema estadoActual = MENU_VERSION;

// =====================================================
// MENU PRINCIPAL
// =====================================================

// Al arrancar mostramos la pantalla principal.
// El primer OK solamente entra al menú de versiones.
bool menuPrincipalActivo = true;

// =====================================================
// VARIABLES
// =====================================================

File archivoActual;

uint32_t posicionArchivo = 0;

unsigned long ultimoTiempo = 0;

// =====================================================
// HISTORIAL DE PAGINAS
// =====================================================

uint32_t historialPaginas[MAX_PAGINAS_HISTORIAL];

int paginaActual = 0;
int paginasGuardadas = 0;

// =====================================================
// LISTAS
// =====================================================

String versiones[MAX_VERSIONES];
int cantidadVersiones = 0;
int indiceVersion = 0;

String libros[MAX_LIBROS];
int cantidadLibros = 0;
int indiceLibro = 0;

String capitulos[MAX_CAPITULOS];
int cantidadCapitulos = 0;
int indiceCapitulo = 0;

// =====================================================
// SELECCION ACTUAL
// =====================================================

String versionActual = "";
String libroActual = "";
String capituloActual = "";

// =====================================================
// CALLBACK JPEG
// =====================================================

bool tft_output(
  int16_t x,
  int16_t y,
  uint16_t w,
  uint16_t h,
  uint16_t *bitmap) {

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

  pinMode(TFT_CS_PIN, OUTPUT);
  digitalWrite(TFT_CS_PIN, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  delay(50);

  SPI.end();

  delay(100);

  SPI.begin(
    SPI_SCK,
    SPI_MISO,
    SPI_MOSI,
    -1);

  delay(100);

  Serial.println("Inicializando SD...");

  if (!SD.begin(SD_CS, SPI, 4000000)) {

    Serial.println("ERROR: SD.begin() fallo");

    return false;
  }

  Serial.println("SD OK");

  return true;
}

// =====================================================
// MOSTRAR MENU PRINCIPAL
// =====================================================

void mostrarMenuPrincipal() {

  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(SD_CS, HIGH);

  tft.fillScreen(TFT_BLACK);

  TJpgDec.setJpgScale(2);

  TJpgDec.drawSdJpg(
    40,
    5,
    JPG_MENU);

  digitalWrite(SD_CS, HIGH);
  digitalWrite(TFT_CS_PIN, HIGH);

  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK);

  tft.drawCentreString(
    "BIBLIA",
    80,
    78,
    2);

  tft.drawCentreString(
    "OK = Comenzar",
    80,
    98,
    1);
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
// REINICIAR HISTORIAL DE PAGINAS
// =====================================================

void reiniciarHistorialPaginas() {

  paginaActual = 0;

  paginasGuardadas = 1;

  historialPaginas[0] = 0;

  posicionArchivo = 0;

  Serial.println(
    "Historial de paginas reiniciado");
}

// =====================================================
// COMPROBAR SI UNA CARPETA ES PERMITIDA
// =====================================================

bool carpetaPermitida(String nombre) {

  String nombreComparar = nombre;

  nombreComparar.toLowerCase();

  // ---------------------------------------------------
  // Carpeta de imagenes
  // ---------------------------------------------------

  if (nombreComparar == "img") {
    return false;
  }
  if (nombreComparar == "ima") {
    return false;
  }
  if (nombreComparar == "images") {
    return false;
  }

  // ---------------------------------------------------
  // Carpeta creada por Windows
  // ---------------------------------------------------

  if (nombreComparar == "system volume information") {
    return false;
  }

  // ---------------------------------------------------
  // Carpetas ocultas
  // ---------------------------------------------------

  if (nombre.length() > 0) {

    if (nombre.charAt(0) == '.') {
      return false;
    }
  }

  return true;
}

// =====================================================
// BUSCAR VERSIONES
// =====================================================

void cargarVersiones() {

  cantidadVersiones = 0;

  File root = SD.open("/");

  if (!root) {

    Serial.println(
      "ERROR abriendo raiz SD");

    return;
  }

  File entry = root.openNextFile();

  while (
    entry && cantidadVersiones < MAX_VERSIONES) {

    if (entry.isDirectory()) {

      String nombre = entry.name();

      // ------------------------------------------------
      // IGNORAR CARPETAS QUE NO SON BIBLIAS
      // ------------------------------------------------

      if (carpetaPermitida(nombre)) {

        versiones[cantidadVersiones] = nombre;

        Serial.print(
          "Version encontrada: ");

        Serial.println(nombre);

        cantidadVersiones++;
      } else {

        Serial.print(
          "Carpeta ignorada: ");

        Serial.println(nombre);
      }
    }

    entry.close();

    entry = root.openNextFile();
  }

  root.close();

  Serial.print(
    "Total versiones: ");

  Serial.println(
    cantidadVersiones);
}

// =====================================================
// BUSCAR LIBROS
// =====================================================

void cargarLibros() {

  cantidadLibros = 0;

  String ruta =
    "/" + versionActual;

  File carpeta =
    SD.open(ruta);

  if (!carpeta) {

    Serial.print(
      "ERROR abriendo version: ");

    Serial.println(ruta);

    return;
  }

  File entry =
    carpeta.openNextFile();

  while (
    entry && cantidadLibros < MAX_LIBROS) {

    if (entry.isDirectory()) {

      String nombre =
        entry.name();

      // ------------------------------------------------
      // IGNORAR CARPETAS NO VALIDAS
      // ------------------------------------------------

      if (carpetaPermitida(nombre)) {

        libros[cantidadLibros] =
          nombre;

        // Serial.print(
        //   "Libro encontrado: "
        // );

        // Serial.println(
        //   nombre
        // );

        cantidadLibros++;
      }
    }

    entry.close();

    entry =
      carpeta.openNextFile();
  }

  carpeta.close();

  // Serial.print(
  //   "Total libros: "
  // );

  // Serial.println(
  //   cantidadLibros
  // );
}

// =====================================================
// BUSCAR CAPITULOS
// =====================================================

void cargarCapitulos() {

  cantidadCapitulos = 0;

  String ruta =
    "/" + versionActual + "/" + libroActual;

  File carpeta =
    SD.open(ruta);

  if (!carpeta) {

    Serial.print(
      "ERROR abriendo libro: ");

    Serial.println(ruta);

    return;
  }

  File entry =
    carpeta.openNextFile();

  while (
    entry && cantidadCapitulos < MAX_CAPITULOS) {

    if (!entry.isDirectory()) {

      String nombre =
        entry.name();

      String nombreComparar =
        nombre;

      nombreComparar.toLowerCase();

      if (
        nombreComparar.endsWith(".txt")) {

        capitulos[cantidadCapitulos] =
          nombre;

        Serial.print(
          "Capitulo encontrado: ");

        Serial.println(nombre);

        cantidadCapitulos++;
      }
    }

    entry.close();

    entry =
      carpeta.openNextFile();
  }

  carpeta.close();

  Serial.print(
    "Total capitulos: ");

  Serial.println(
    cantidadCapitulos);
}

// =====================================================
// MOSTRAR LISTA
// =====================================================

void mostrarLista(
  String titulo,
  String lista[],
  int cantidad,
  int seleccionado) {

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(
    TFT_YELLOW,
    TFT_BLACK);

  tft.drawCentreString(
    titulo,
    80,
    3,
    2);

  // ---------------------------------------------------
  // SIN DATOS
  // ---------------------------------------------------

  if (cantidad <= 0) {

    tft.setTextColor(
      TFT_RED,
      TFT_BLACK);

    tft.drawCentreString(
      "SIN DATOS",
      80,
      60,
      2);

    return;
  }

  // ---------------------------------------------------
  // ASEGURAR INDICE VALIDO
  // ---------------------------------------------------

  if (seleccionado < 0) {
    seleccionado = 0;
  }

  if (seleccionado >= cantidad) {
    seleccionado = cantidad - 1;
  }

  // ---------------------------------------------------
  // CALCULAR VENTANA
  // ---------------------------------------------------

  int inicio =
    seleccionado - 3;

  if (inicio < 0) {
    inicio = 0;
  }

  int fin =
    inicio + 7;

  if (fin > cantidad) {
    fin = cantidad;
  }

  // ---------------------------------------------------
  // MOSTRAR ELEMENTOS
  // ---------------------------------------------------

  for (
    int i = inicio;
    i < fin;
    i++) {

    int y =
      25 + ((i - inicio) * 14);

    if (i == seleccionado) {

      tft.setTextColor(
        TFT_BLACK,
        TFT_WHITE);
    } else {

      tft.setTextColor(
        TFT_WHITE,
        TFT_BLACK);
    }

    String texto =
      lista[i];

    if (texto.length() > 20) {

      texto =
        texto.substring(0, 20);
    }

    tft.drawCentreString(
      texto,
      80,
      y,
      1);
  }
}

// =====================================================
// MOSTRAR VERSIONES
// =====================================================

void mostrarVersiones() {

  mostrarLista(
    "VERSIONES BIBLICAS",
    versiones,
    cantidadVersiones,
    indiceVersion);
}

// =====================================================
// MOSTRAR LIBROS
// =====================================================

void mostrarLibros() {

  mostrarLista(
    versionActual,
    libros,
    cantidadLibros,
    indiceLibro);
}

// =====================================================
// MOSTRAR CAPITULOS
// =====================================================

void mostrarCapitulos() {

  mostrarLista(
    libroActual,
    capitulos,
    cantidadCapitulos,
    indiceCapitulo);
}

// =====================================================
// ABRIR CAPITULO
// =====================================================

void abrirCapitulo() {

  if (archivoActual) {
    archivoActual.close();
  }

  String ruta =
    "/" + versionActual + "/" + libroActual + "/" + capituloActual;

  Serial.println();

  Serial.print(
    "Abriendo: ");

  Serial.println(ruta);

  digitalWrite(
    TFT_CS_PIN,
    HIGH);

  digitalWrite(
    SD_CS,
    LOW);

  archivoActual =
    SD.open(
      ruta,
      FILE_READ);

  digitalWrite(
    SD_CS,
    HIGH);

  if (!archivoActual) {

    Serial.println(
      "ERROR: No se pudo abrir TXT");

    tft.fillScreen(
      TFT_BLACK);

    tft.setTextColor(
      TFT_RED);

    tft.setCursor(
      5,
      5);

    tft.println(
      "Error abriendo TXT");

    tft.setTextColor(
      TFT_WHITE);

    tft.setCursor(
      5,
      20);

    tft.println(
      ruta);

    return;
  }

  Serial.println(
    "TXT abierto correctamente");

  Serial.print(
    "Tamano: ");

  Serial.println(
    archivoActual.size());

  // ---------------------------------------------------
  // REINICIAR HISTORIAL
  // ---------------------------------------------------

  reiniciarHistorialPaginas();

  // ---------------------------------------------------
  // MOSTRAR PRIMERA PAGINA
  // ---------------------------------------------------

  cargarPagina();
}

// =====================================================
// CARGAR PAGINA
// =====================================================

void cargarPagina() {

  if (!archivoActual) {
    return;
  }

  // ---------------------------------------------------
  // COMPROBAR FINAL DEL ARCHIVO
  // ---------------------------------------------------

  if (
    posicionArchivo >= archivoActual.size()) {

    Serial.println(
      "Fin del archivo");

    return;
  }

  // ---------------------------------------------------
  // LIMPIAR PANTALLA
  // ---------------------------------------------------

  tft.fillScreen(
    TFT_BLACK);

  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK);

  tft.setTextWrap(false);

  // ---------------------------------------------------
  // FUENTE
  // ---------------------------------------------------

  tft.setTextFont(1);

  tft.setTextSize(1);

  // ---------------------------------------------------
  // CONFIGURACION
  // ---------------------------------------------------

  int margenX = 5;

  int margenY = 5;

  int interlineado = 12;

  int limiteDerecho = 150;

  int cursorY = margenY;

  tft.setCursor(
    margenX,
    cursorY);

  // ---------------------------------------------------
  // POSICION INICIAL
  // ---------------------------------------------------

  uint32_t inicioPagina =
    posicionArchivo;

  Serial.print(
    "Cargando pagina ");

  Serial.print(
    paginaActual + 1);

  Serial.print(
    " desde posicion ");

  Serial.println(
    inicioPagina);

  // ---------------------------------------------------
  // SD ACTIVA
  // ---------------------------------------------------

  digitalWrite(
    TFT_CS_PIN,
    HIGH);

  digitalWrite(
    SD_CS,
    LOW);

  // ---------------------------------------------------
  // IR AL INICIO EXACTO
  // ---------------------------------------------------

  if (
    !archivoActual.seek(
      posicionArchivo)) {

    Serial.println(
      "ERROR: seek inicial fallo");

    digitalWrite(
      SD_CS,
      HIGH);

    digitalWrite(
      TFT_CS_PIN,
      HIGH);

    return;
  }

  // ---------------------------------------------------
  // BUFFER PARA UNA PALABRA
  // ---------------------------------------------------

  String palabra = "";

  // ===================================================
  // LEER TEXTO
  // ===================================================

  while (
    archivoActual.available()) {

    char c =
      archivoActual.read();

    // -------------------------------------------------
    // IGNORAR CR
    // -------------------------------------------------

    if (c == '\r') {
      continue;
    }

    // -------------------------------------------------
    // NUEVA LINEA
    // -------------------------------------------------

    if (c == '\n') {

      // -----------------------------------------------
      // IMPRIMIR PALABRA PENDIENTE
      // -----------------------------------------------

      if (
        palabra.length() > 0) {

        digitalWrite(
          SD_CS,
          HIGH);

        digitalWrite(
          TFT_CS_PIN,
          LOW);

        int anchoPalabra =
          tft.textWidth(
            palabra);

        int anchoEspacio =
          tft.textWidth(" ");

        int posicionX =
          tft.getCursorX();

        // ---------------------------------------------
        // SI NO CABE
        // ---------------------------------------------

        if (
          posicionX + anchoPalabra > limiteDerecho) {

          cursorY +=
            interlineado;

          if (
            cursorY > (128 - interlineado)) {

            digitalWrite(
              TFT_CS_PIN,
              HIGH);

            digitalWrite(
              SD_CS,
              LOW);

            break;
          }

          tft.setCursor(
            margenX,
            cursorY);
        }

        // ---------------------------------------------
        // IMPRIMIR PALABRA COMPLETA
        // ---------------------------------------------

        tft.print(
          palabra);

        // ---------------------------------------------
        // ESPACIO
        // ---------------------------------------------

        if (
          tft.getCursorX() + anchoEspacio <= limiteDerecho) {

          tft.print(" ");
        }

        digitalWrite(
          TFT_CS_PIN,
          HIGH);

        digitalWrite(
          SD_CS,
          LOW);

        palabra = "";
      }

      // -----------------------------------------------
      // SALTO DE LINEA REAL
      // -----------------------------------------------

      cursorY +=
        interlineado;

      if (
        cursorY > (128 - interlineado)) {

        break;
      }

      digitalWrite(
        SD_CS,
        HIGH);

      digitalWrite(
        TFT_CS_PIN,
        LOW);

      tft.setCursor(
        margenX,
        cursorY);

      digitalWrite(
        TFT_CS_PIN,
        HIGH);

      digitalWrite(
        SD_CS,
        LOW);

      continue;
    }

    // -------------------------------------------------
    // ESPACIO = TERMINO PALABRA
    // -------------------------------------------------

    if (
      c == ' ' || c == '\t') {

      if (
        palabra.length() == 0) {

        continue;
      }

      digitalWrite(
        SD_CS,
        HIGH);

      digitalWrite(
        TFT_CS_PIN,
        LOW);

      int anchoPalabra =
        tft.textWidth(
          palabra);

      int anchoEspacio =
        tft.textWidth(" ");

      int posicionX =
        tft.getCursorX();

      // -----------------------------------------------
      // LA PALABRA NO CABE
      // -----------------------------------------------

      if (
        posicionX + anchoPalabra > limiteDerecho) {

        cursorY +=
          interlineado;

        if (
          cursorY > (128 - interlineado)) {

          digitalWrite(
            TFT_CS_PIN,
            HIGH);

          digitalWrite(
            SD_CS,
            LOW);

          break;
        }

        tft.setCursor(
          margenX,
          cursorY);
      }

      // -----------------------------------------------
      // IMPRIMIR PALABRA COMPLETA
      // -----------------------------------------------

      tft.print(
        palabra);

      // -----------------------------------------------
      // ESPACIO
      // -----------------------------------------------

      if (
        tft.getCursorX() + anchoEspacio <= limiteDerecho) {

        tft.print(" ");
      }

      digitalWrite(
        TFT_CS_PIN,
        HIGH);

      digitalWrite(
        SD_CS,
        LOW);

      palabra = "";

      continue;
    }

    // -------------------------------------------------
    // AGREGAR CARACTER
    // -------------------------------------------------

    palabra += c;
  }

  // ===================================================
  // IMPRIMIR ULTIMA PALABRA PENDIENTE
  // ===================================================

  if (
    palabra.length() > 0 && cursorY <= (128 - interlineado)) {

    digitalWrite(
      SD_CS,
      HIGH);

    digitalWrite(
      TFT_CS_PIN,
      LOW);

    int anchoPalabra =
      tft.textWidth(
        palabra);

    int posicionX =
      tft.getCursorX();

    // -------------------------------------------------
    // SI NO CABE
    // -------------------------------------------------

    if (
      posicionX + anchoPalabra > limiteDerecho) {

      cursorY +=
        interlineado;

      if (
        cursorY <= (128 - interlineado)) {

        tft.setCursor(
          margenX,
          cursorY);

        tft.print(
          palabra);
      }

    } else {

      tft.print(
        palabra);
    }

    digitalWrite(
      TFT_CS_PIN,
      HIGH);

    digitalWrite(
      SD_CS,
      LOW);
  }

  // ===================================================
  // POSICION FINAL
  // ===================================================

  uint32_t nuevaPosicion =
    archivoActual.position();

  // ---------------------------------------------------
  // DESACTIVAR SD
  // ---------------------------------------------------

  digitalWrite(
    SD_CS,
    HIGH);

  digitalWrite(
    TFT_CS_PIN,
    HIGH);

  // ---------------------------------------------------
  // GUARDAR POSICION
  // ---------------------------------------------------

  posicionArchivo =
    nuevaPosicion;

  Serial.print(
    "Pagina termina en: ");

  Serial.println(
    posicionArchivo);
}

// =====================================================
// PAGINA SIGUIENTE
// =====================================================

void paginaSiguiente() {

  if (!archivoActual) {
    return;
  }

  // ---------------------------------------------------
  // COMPROBAR FINAL
  // ---------------------------------------------------

  if (
    posicionArchivo >= archivoActual.size()) {

    Serial.println(
      "Fin del capitulo");

    return;
  }

  // ---------------------------------------------------
  // AVANZAR PAGINA
  // ---------------------------------------------------

  paginaActual++;

  // ---------------------------------------------------
  // GUARDAR POSICION
  // ---------------------------------------------------

  if (
    paginaActual >= paginasGuardadas) {

    if (
      paginasGuardadas < MAX_PAGINAS_HISTORIAL) {

      historialPaginas[paginasGuardadas] =
        posicionArchivo;

      paginasGuardadas++;

      Serial.print(
        "Nueva pagina guardada: ");

      Serial.println(
        paginasGuardadas);

    } else {

      Serial.println(
        "ADVERTENCIA: historial lleno");

      paginaActual =
        paginasGuardadas - 1;

      return;
    }
  }

  // ---------------------------------------------------
  // RECUPERAR POSICION
  // ---------------------------------------------------

  posicionArchivo =
    historialPaginas[paginaActual];

  Serial.print(
    "Pagina siguiente: ");

  Serial.println(
    paginaActual + 1);

  // ---------------------------------------------------
  // MOSTRAR
  // ---------------------------------------------------

  cargarPagina();
}

// =====================================================
// PAGINA ANTERIOR
// =====================================================

void paginaAnterior() {

  if (!archivoActual) {
    return;
  }

  // ---------------------------------------------------
  // PRIMERA PAGINA
  // ---------------------------------------------------

  if (
    paginaActual <= 0) {

    Serial.println(
      "Ya estamos en la primera pagina");

    posicionArchivo =
      historialPaginas[0];

    cargarPagina();

    return;
  }

  // ---------------------------------------------------
  // RETROCEDER
  // ---------------------------------------------------

  paginaActual--;

  // ---------------------------------------------------
  // RECUPERAR POSICION
  // ---------------------------------------------------

  posicionArchivo =
    historialPaginas[paginaActual];

  Serial.print(
    "Volviendo a pagina: ");

  Serial.println(
    paginaActual + 1);

  Serial.print(
    "Posicion exacta: ");

  Serial.println(
    posicionArchivo);

  // ---------------------------------------------------
  // MOSTRAR
  // ---------------------------------------------------

  cargarPagina();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200);

  delay(1000);

  Serial.println();

  Serial.println(
    "================================");

  Serial.println(
    " ESP32 BIBLIA");

  Serial.println(
    "================================");

  // ===================================================
  // JOYSTICK
  // ===================================================

  pinMode(
    JOY_UP,
    INPUT_PULLUP);

  pinMode(
    JOY_DOWN,
    INPUT_PULLUP);

  pinMode(
    JOY_LEFT,
    INPUT_PULLUP);

  pinMode(
    JOY_RIGHT,
    INPUT_PULLUP);

  pinMode(
    JOY_BTN,
    INPUT_PULLUP);

  // ===================================================
  // CS
  // ===================================================

  pinMode(
    TFT_CS_PIN,
    OUTPUT);

  digitalWrite(
    TFT_CS_PIN,
    HIGH);

  pinMode(
    SD_CS,
    OUTPUT);

  digitalWrite(
    SD_CS,
    HIGH);

  // ===================================================
  // SPI
  // ===================================================

  SPI.begin(
    SPI_SCK,
    SPI_MISO,
    SPI_MOSI,
    -1);

  // ===================================================
  // TFT
  // ===================================================

  Serial.println(
    "Inicializando TFT...");

  tft.init();

  tft.setRotation(
    1);

  digitalWrite(
    TFT_CS_PIN,
    HIGH);

  digitalWrite(
    SD_CS,
    HIGH);

  tft.fillScreen(
    TFT_BLACK);

  Serial.println(
    "TFT OK");

  // ===================================================
  // SD
  // ===================================================

  if (!iniciarSD()) {

    tft.fillScreen(
      TFT_BLACK);

    tft.setTextColor(
      TFT_RED);

    tft.setCursor(
      5,
      5);

    tft.println(
      "ERROR SD");

    tft.setCursor(
      5,
      20);

    tft.println(
      "No se pudo iniciar");

    while (true) {

      delay(1000);
    }
  }

  // ===================================================
  // JPEG DECODER
  // ===================================================

  TJpgDec.setJpgScale(
    1);

  TJpgDec.setSwapBytes(
    true);

  TJpgDec.setCallback(
    tft_output);

  // ===================================================
  // BUSCAR VERSIONES
  // ===================================================

  cargarVersiones();

  if (
    cantidadVersiones == 0) {

    tft.fillScreen(
      TFT_BLACK);

    tft.setTextColor(
      TFT_RED);

    tft.setCursor(
      5,
      5);

    tft.println(
      "No hay versiones");

    tft.setCursor(
      5,
      20);

    tft.println(
      "Revise la SD");

    while (true) {

      delay(1000);
    }
  }

  // ===================================================
  // MENU PRINCIPAL
  // ===================================================

  menuPrincipalActivo = true;

  estadoActual =
    MENU_VERSION;

  indiceVersion = 0;

  mostrarMenuPrincipal();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // PANTALLA PRINCIPAL
  // ===================================================
  //
  // IMPORTANTE:
  // El primer OK NO selecciona una version.
  // Solamente sale de la pantalla inicial y muestra
  // el menu donde el usuario puede escoger la Biblia.
  // ===================================================

  if (menuPrincipalActivo) {

    if (
      leerBoton(JOY_BTN)) {

      menuPrincipalActivo =
        false;

      indiceVersion = 0;

      estadoActual =
        MENU_VERSION;

      mostrarVersiones();

      return;
    }

    return;
  }

  // ===================================================
  // MENU VERSIONES
  // ===================================================

  if (
    estadoActual == MENU_VERSION) {

    // -------------------------------------------------
    // ENTRAR
    // -------------------------------------------------

    if (
      leerBoton(JOY_BTN)) {

      if (
        cantidadVersiones <= 0) {

        mostrarVersiones();

        return;
      }

      versionActual =
        versiones[indiceVersion];

      Serial.print(
        "Version seleccionada: ");

      Serial.println(
        versionActual);

      indiceLibro = 0;

      cargarLibros();

      estadoActual =
        MENU_LIBRO;

      mostrarLibros();

      return;
    }

    // -------------------------------------------------
    // ABAJO
    // -------------------------------------------------

    if (
      leerBoton(JOY_DOWN)) {

      indiceVersion++;

      if (
        indiceVersion >= cantidadVersiones) {

        indiceVersion = 0;
      }

      mostrarVersiones();

      return;
    }

    // -------------------------------------------------
    // ARRIBA
    // -------------------------------------------------

    if (
      leerBoton(JOY_UP)) {

      indiceVersion--;

      if (
        indiceVersion < 0) {

        indiceVersion =
          cantidadVersiones - 1;
      }

      mostrarVersiones();

      return;
    }
  }

  // ===================================================
  // MENU LIBROS
  // ===================================================

  else if (
    estadoActual == MENU_LIBRO) {

    // -------------------------------------------------
    // ENTRAR
    // -------------------------------------------------

    if (
      leerBoton(JOY_BTN)) {

      if (
        cantidadLibros <= 0) {

        mostrarLibros();

        return;
      }

      libroActual =
        libros[indiceLibro];

      Serial.print(
        "Libro seleccionado: ");

      Serial.println(
        libroActual);

      indiceCapitulo = 0;

      cargarCapitulos();

      estadoActual =
        MENU_CAPITULO;

      mostrarCapitulos();

      return;
    }

    // -------------------------------------------------
    // ABAJO
    // -------------------------------------------------

    if (
      leerBoton(JOY_DOWN)) {

      if (
        cantidadLibros <= 0) {

        return;
      }

      indiceLibro++;

      if (
        indiceLibro >= cantidadLibros) {

        indiceLibro = 0;
      }

      mostrarLibros();

      return;
    }

    // -------------------------------------------------
    // ARRIBA
    // -------------------------------------------------

    if (
      leerBoton(JOY_UP)) {

      if (
        cantidadLibros <= 0) {

        return;
      }

      indiceLibro--;

      if (
        indiceLibro < 0) {

        indiceLibro =
          cantidadLibros - 1;
      }

      mostrarLibros();

      return;
    }

    // -------------------------------------------------
    // VOLVER
    // -------------------------------------------------

    if (
      leerBoton(JOY_LEFT)) {

      estadoActual =
        MENU_VERSION;

      mostrarVersiones();

      return;
    }
  }

  // ===================================================
  // MENU CAPITULOS
  // ===================================================

  else if (
    estadoActual == MENU_CAPITULO) {

    // -------------------------------------------------
    // ENTRAR
    // -------------------------------------------------

    if (
      leerBoton(JOY_BTN)) {

      if (
        cantidadCapitulos <= 0) {

        mostrarCapitulos();

        return;
      }

      capituloActual =
        capitulos[indiceCapitulo];

      Serial.print(
        "Capitulo seleccionado: ");

      Serial.println(
        capituloActual);

      estadoActual =
        LEYENDO;

      abrirCapitulo();

      return;
    }

    // -------------------------------------------------
    // ABAJO
    // -------------------------------------------------

    if (
      leerBoton(JOY_DOWN)) {

      if (
        cantidadCapitulos <= 0) {

        return;
      }

      indiceCapitulo++;

      if (
        indiceCapitulo >= cantidadCapitulos) {

        indiceCapitulo = 0;
      }

      mostrarCapitulos();

      return;
    }

    // -------------------------------------------------
    // ARRIBA
    // -------------------------------------------------

    if (
      leerBoton(JOY_UP)) {

      if (
        cantidadCapitulos <= 0) {

        return;
      }

      indiceCapitulo--;

      if (
        indiceCapitulo < 0) {

        indiceCapitulo =
          cantidadCapitulos - 1;
      }

      mostrarCapitulos();

      return;
    }

    // -------------------------------------------------
    // VOLVER
    // -------------------------------------------------

    if (
      leerBoton(JOY_LEFT)) {

      estadoActual =
        MENU_LIBRO;

      mostrarLibros();

      return;
    }
  }

  // ===================================================
  // LECTURA
  // ===================================================

  else if (
    estadoActual == LEYENDO) {

    // -------------------------------------------------
    // PAGINA SIGUIENTE
    // -------------------------------------------------

    if (
      leerBoton(JOY_DOWN)) {

      Serial.println(
        "PAGINA SIGUIENTE");

      paginaSiguiente();

      return;
    }

    // -------------------------------------------------
    // PAGINA ANTERIOR
    // -------------------------------------------------

    if (
      leerBoton(JOY_UP)) {

      Serial.println(
        "PAGINA ANTERIOR");

      paginaAnterior();

      return;
    }

    // -------------------------------------------------
    // VOLVER A CAPITULOS
    // -------------------------------------------------

    if (
      leerBoton(JOY_LEFT)) {

      Serial.println(
        "VOLVIENDO A CAPITULOS");

      if (archivoActual) {

        archivoActual.close();
      }

      estadoActual =
        MENU_CAPITULO;

      mostrarCapitulos();

      return;
    }

    // -------------------------------------------------
    // SIGUIENTE CAPITULO
    // -------------------------------------------------

    if (
      leerBoton(JOY_RIGHT)) {

      if (
        indiceCapitulo < cantidadCapitulos - 1) {

        indiceCapitulo++;

        capituloActual =
          capitulos[indiceCapitulo];

        abrirCapitulo();
      } else {

        Serial.println(
          "Ultimo capitulo");
      }

      return;
    }
  }
}