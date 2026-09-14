const fs = require("fs");
const path = require("path");

// ============================================================
// CONFIGURACION
// ============================================================

const inputFile = path.join(__dirname, "Jsons", "RVR1960-Spanish.json");

const outputBaseDir = path.join(__dirname, "Bibles-ready");

// ============================================================
// ORDEN DE LOS 66 LIBROS DE LA BIBLIA
// ============================================================

const ordenBiblia = [
  // ==========================
  // ANTIGUO TESTAMENTO
  // ==========================

  "GEN",
  "EXO",
  "LEV",
  "NUM",
  "DEU",
  "JOS",
  "JDG",
  "RUT",
  "1SA",
  "2SA",
  "1KI",
  "2KI",
  "1CH",
  "2CH",
  "EZR",
  "NEH",
  "EST",
  "JOB",
  "PSA",
  "PRO",
  "ECC",
  "SNG",
  "ISA",
  "JER",
  "LAM",
  "EZK",
  "DAN",
  "HOS",
  "JOL",
  "AMO",
  "OBA",
  "JON",
  "MIC",
  "NAM",
  "HAB",
  "ZEP",
  "HAG",
  "ZEC",
  "MAL",

  // ==========================
  // NUEVO TESTAMENTO
  // ==========================

  "MAT",
  "MRK",
  "LUK",
  "JHN",
  "ACT",
  "ROM",
  "1CO",
  "2CO",
  "GAL",
  "EPH",
  "PHP",
  "COL",
  "1TH",
  "2TH",
  "1TI",
  "2TI",
  "TIT",
  "PHM",
  "HEB",
  "JAS",
  "1PE",
  "2PE",
  "1JN",
  "2JN",
  "3JN",
  "JUD",
  "REV",
];

// ============================================================
// CONVERTIR TEXTO A ASCII
// ============================================================
//
// El ESP32/TFT puede tener problemas mostrando:
//
// á é í ó ú
// ñ
// ü
// ¿
// ¡
// ©
// ®
// ™
//
// Por eso todo se convierte a ASCII.
//
// ============================================================

function convertirParaESP32(texto) {
  if (texto === null || texto === undefined) {
    return "";
  }

  let resultado = String(texto);

  // ----------------------------------------------------------
  // Eliminar acentos y diacriticos
  // ----------------------------------------------------------

  resultado = resultado.normalize("NFD").replace(/[\u0300-\u036f]/g, "");

  // ----------------------------------------------------------
  // Signos especiales
  // ----------------------------------------------------------

  resultado = resultado
    .replace(/¿/g, "?")
    .replace(/¡/g, "!")

    // Comillas curvas
    .replace(/[“”„‟]/g, '"')
    .replace(/[‘’‚‛]/g, "'")

    // Guiones especiales
    .replace(/[–—―]/g, "-")

    // Puntos suspensivos
    .replace(/…/g, "...")

    // Espacio no separable
    .replace(/\u00A0/g, " ");

  // ----------------------------------------------------------
  // Simbolos de copyright y similares
  // ----------------------------------------------------------

  resultado = resultado.replace(/[®©™℗]/g, "");

  // ----------------------------------------------------------
  // Eliminar cualquier caracter que NO sea ASCII
  // ----------------------------------------------------------

  resultado = resultado.replace(/[^\x00-\x7F]/g, "");

  // ----------------------------------------------------------
  // Limpiar espacios
  // ----------------------------------------------------------

  resultado = resultado
    .replace(/[ \t]+/g, " ")
    .replace(/ *\n */g, "\n")
    .trim();

  return resultado;
}

// ============================================================
// CREAR NOMBRE DE CARPETA DEL LIBRO
// ============================================================
//
// IMPORTANTE:
//
// EL NOMBRE COMPLETO DE LA CARPETA DEBE TENER MAXIMO 7
// CARACTERES.
//
// Ejemplos:
//
// 01_GENE
// 02_EXOD
// 03_LEVI
// 04_NUME
// 05_DEUT
//
// La estructura es:
//
// 2 digitos + "_" + 3 letras
//
// = 6 caracteres
//
// Se utiliza un limite de 7 para permitir hasta 4 letras
// cuando sea posible.
//
// ============================================================

function crearNombreCarpeta(numeroLibro, nombreLibro) {
  // ----------------------------------------------------------
  // Convertir nombre a ASCII
  // ----------------------------------------------------------

  let nombre = convertirParaESP32(nombreLibro);

  // ----------------------------------------------------------
  // Solo letras y numeros
  // ----------------------------------------------------------

  nombre = nombre.replace(/[^A-Za-z0-9]/g, "").toUpperCase();

  // ----------------------------------------------------------
  // Numero del libro
  // ----------------------------------------------------------

  const numero = String(numeroLibro).padStart(2, "0");

  // ----------------------------------------------------------
  // Ya tenemos:
  //
  // 01_
  //
  // Son 3 caracteres.
  //
  // El limite total es 7.
  //
  // Por lo tanto podemos utilizar MAXIMO 4 caracteres
  // del nombre.
  // ----------------------------------------------------------

  const nombreCorto = nombre.substring(0, 4);

  // ----------------------------------------------------------
  // Resultado final
  //
  // Ejemplo:
  //
  // 01_GENE
  //
  // 7 caracteres
  // ----------------------------------------------------------

  const resultado = `${numero}_${nombreCorto}`;

  // ----------------------------------------------------------
  // Seguridad adicional
  //
  // Nunca permitir mas de 7 caracteres.
  // ----------------------------------------------------------

  return resultado.substring(0, 7);
}

// ============================================================
// NUMERO DE DOS DIGITOS
// ============================================================

function dosDigitos(numero) {
  return String(numero).padStart(2, "0");
}

// ============================================================
// PROCESAR BIBLIA
// ============================================================

function procesarBiblia() {
  try {
    console.log("");
    console.log("================================================");
    console.log("      CONVERSOR BIBLIA -> ESP32 / TFT");
    console.log("================================================");
    console.log("");

    // ========================================================
    // VERIFICAR ARCHIVO JSON
    // ========================================================

    if (!fs.existsSync(inputFile)) {
      console.error("ERROR: No se encontro el archivo JSON:");

      console.error(inputFile);

      return;
    }

    // ========================================================
    // LEER JSON
    // ========================================================

    console.log("Leyendo archivo JSON...");

    const rawData = fs.readFileSync(inputFile, "utf8");

    // --------------------------------------------------------
    // Eliminar BOM si existe
    // --------------------------------------------------------

    const bibleData = JSON.parse(rawData.replace(/^\uFEFF/, ""));

    // ========================================================
    // OBTENER NOMBRE DE LA VERSION
    // ========================================================

    let versionFolder = bibleData.local_abbreviation || "BIBLIA";

    // --------------------------------------------------------
    // Limpiar nombre
    // --------------------------------------------------------

    versionFolder = convertirParaESP32(versionFolder)
      .replace(/[^A-Za-z0-9_-]/g, "")
      .toUpperCase();

    // ========================================================
    // CARPETA DE LA VERSION
    // ========================================================

    const versionPath = path.join(outputBaseDir, versionFolder);

    fs.mkdirSync(versionPath, {
      recursive: true,
    });

    console.log(`Version: ${versionFolder}`);

    console.log("");

    // ========================================================
    // CREAR MAPA DE LIBROS
    // ========================================================

    const librosPorUSFM = new Map();

    for (const book of bibleData.books || []) {
      if (book.book_usfm) {
        librosPorUSFM.set(book.book_usfm, book);
      }
    }

    // ========================================================
    // CONTADORES
    // ========================================================

    let librosProcesados = 0;

    let capitulosProcesados = 0;

    let versiculosProcesados = 0;

    // ========================================================
    // PROCESAR LOS 66 LIBROS
    // ========================================================

    for (let indiceLibro = 0; indiceLibro < ordenBiblia.length; indiceLibro++) {
      const numeroLibro = indiceLibro + 1;

      const bookUSFM = ordenBiblia[indiceLibro];

      const book = librosPorUSFM.get(bookUSFM);

      // ======================================================
      // LIBRO NO ENCONTRADO
      // ======================================================

      if (!book) {
        console.warn(`AVISO: No se encontro el libro ${bookUSFM}`);

        continue;
      }

      // ======================================================
      // CREAR NOMBRE DE CARPETA
      // ======================================================
      //
      // Ejemplo:
      //
      // Genesis       -> 01_GENE
      // Exodo         -> 02_EXOD
      // Levitico      -> 03_LEVI
      // Numeros       -> 04_NUME
      // Deuteronomio  -> 05_DEUT
      // Josue         -> 06_JOSU
      //
      // MAXIMO 7 CARACTERES
      //
      // ======================================================

      const carpetaLibro = crearNombreCarpeta(numeroLibro, book.name);

      const bookPath = path.join(versionPath, carpetaLibro);

      // ------------------------------------------------------
      // Crear carpeta
      // ------------------------------------------------------

      fs.mkdirSync(bookPath, {
        recursive: true,
      });

      console.log(
        `LIBRO ${dosDigitos(numeroLibro)}/66: ${book.name} -> ${carpetaLibro}`,
      );

      // ======================================================
      // PROCESAR CAPITULOS
      // ======================================================

      let numeroCapitulo = 0;

      for (const chapter of book.chapters || []) {
        numeroCapitulo++;

        // ----------------------------------------------------
        // Nombre del archivo
        //
        // 01.txt
        // 02.txt
        // 03.txt
        //
        // ====================================================
        //
        // NO se crea carpeta de capitulos.
        //
        // Esto conserva EXACTAMENTE la estructura que ya usa
        // tu ESP32.
        //
        // ====================================================

        const chapterFileName = `${dosDigitos(numeroCapitulo)}.txt`;

        const chapterFilePath = path.join(bookPath, chapterFileName);

        // ----------------------------------------------------
        // Texto del capitulo
        // ----------------------------------------------------

        let chapterText = "";

        // ====================================================
        // PROCESAR ITEMS DEL CAPITULO
        // ====================================================

        for (const item of chapter.items || []) {
          // ==================================================
          // SUBTITULOS
          // ==================================================

          if (
            typeof item.type === "string" &&
            item.type.startsWith("heading")
          ) {
            let headingText = "";

            // ------------------------------------------------
            // Obtener texto del subtitulo
            // ------------------------------------------------

            if (Array.isArray(item.lines)) {
              headingText = item.lines.join(" ").trim();
            } else {
              headingText = String(item.lines || "").trim();
            }

            // ------------------------------------------------
            // Convertir a ASCII
            // ------------------------------------------------

            headingText = convertirParaESP32(headingText);

            // ------------------------------------------------
            // Agregar subtitulo
            //
            // Ejemplo:
            //
            // # La creacion
            //
            // ------------------------------------------------

            if (headingText) {
              chapterText += `\n# ${headingText}\n`;
            }

            continue;
          }

          // ==================================================
          // VERSICULOS
          // ==================================================

          if (item.type === "verse") {
            // ------------------------------------------------
            // Verificar numero
            // ------------------------------------------------

            if (
              !Array.isArray(item.verse_numbers) ||
              item.verse_numbers.length === 0
            ) {
              continue;
            }

            // ------------------------------------------------
            // Numero del versiculo
            // ------------------------------------------------

            const verseNumber = Number(item.verse_numbers[0]);

            if (!Number.isInteger(verseNumber) || verseNumber < 1) {
              continue;
            }

            // ------------------------------------------------
            // Obtener texto
            // ------------------------------------------------

            let verseText = "";

            if (Array.isArray(item.lines)) {
              verseText = item.lines.join(" ").trim();
            } else {
              verseText = String(item.lines || "").trim();
            }

            // ------------------------------------------------
            // Convertir caracteres para ESP32
            // ------------------------------------------------

            verseText = convertirParaESP32(verseText);

            // ------------------------------------------------
            // Ignorar versiculos vacios
            // ------------------------------------------------

            if (!verseText) {
              continue;
            }

            // ------------------------------------------------
            // Agregar versiculo
            //
            // Ejemplo:
            //
            // 1 En el principio creo Dios...
            //
            // ------------------------------------------------

            chapterText += `${verseNumber} ${verseText}\n`;

            versiculosProcesados++;
          }
        }

        // ====================================================
        // LIMPIAR TEXTO FINAL
        // ====================================================

        chapterText = chapterText.replace(/\n{3,}/g, "\n\n").trim();

        // ====================================================
        // GUARDAR CAPITULO
        // ====================================================

        fs.writeFileSync(chapterFilePath, chapterText, {
          encoding: "ascii",
        });

        capitulosProcesados++;
      }

      librosProcesados++;
    }

    // ========================================================
    // RESULTADO
    // ========================================================

    console.log("");
    console.log("================================================");
    console.log("              PROCESO COMPLETADO");
    console.log("================================================");
    console.log("");

    console.log(`Version:      ${versionFolder}`);

    console.log(`Libros:       ${librosProcesados}/66`);

    console.log(`Capitulos:    ${capitulosProcesados}`);

    console.log(`Versiculos:   ${versiculosProcesados}`);

    console.log("");

    console.log("Carpeta de salida:");

    console.log(versionPath);

    console.log("");

    console.log("Caracteres convertidos a ASCII.");

    console.log("Subtitulos conservados con #.");

    console.log("Estructura compatible con el navegador del ESP32.");

    console.log("Nombres de carpetas: MAXIMO 7 caracteres.");

    console.log("");
  } catch (error) {
    console.error("");
    console.error("================================================");
    console.error("       ERROR PROCESANDO LA BIBLIA");
    console.error("================================================");
    console.error("");

    console.error(error);
  }
}

// ============================================================
// EJECUTAR
// ============================================================

procesarBiblia();
