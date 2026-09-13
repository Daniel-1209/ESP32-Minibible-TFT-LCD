const fs = require("fs");
const path = require("path");

// 1. Configuración de Rutas
const inputFile = path.join(__dirname, "Jsons", "kjv.json");
const outputBaseDir = path.join(__dirname, "Bibles-ready");
const versionFolder = "KJV"; // Nombre de la carpeta principal de esta versión

async function procesarBiblia() {
  try {
    console.log("Iniciando lectura del archivo JSON...");

    // Verificar si el JSON existe
    if (!fs.existsSync(inputFile)) {
      console.error(`Error: No se encontró el archivo en ${inputFile}`);
      return;
    }

    // Leer y parsear el archivo
    const rawData = fs.readFileSync(inputFile, "utf-8");
    const bibleData = JSON.parse(rawData);

    // Crear la carpeta de la versión (ej. Bibles-ready/KJV) si no existe
    const versionPath = path.join(outputBaseDir, versionFolder);
    if (!fs.existsSync(versionPath)) {
      fs.mkdirSync(versionPath, { recursive: true });
    }

    // 2. Iterar sobre los Libros
    let countBooks = 0;
    for (const bookName in bibleData) {
      // Reemplazamos los espacios por guiones bajos para evitar problemas de lectura en el ESP32
      // Ej: "1 Chronicles" se convierte en "1_Chronicles"
      const safeBookName = bookName.replace(/\s+/g, "_");
      const bookPath = path.join(versionPath, safeBookName);

      // Crear la carpeta del libro
      if (!fs.existsSync(bookPath)) {
        fs.mkdirSync(bookPath, { recursive: true });
      }

      const chapters = bibleData[bookName];

      // 3. Iterar sobre los Capítulos
      for (const chapterNum in chapters) {
        const verses = chapters[chapterNum];
        let chapterText = "";

        // 4. Iterar sobre los Versículos y formatearlos
        for (const verseNum in verses) {
          // Limpiamos los espacios extra al final que trae tu JSON original
          const cleanVerse = verses[verseNum].trim();
          chapterText += `${verseNum} ${cleanVerse}\n`;
        }

        // Guardar el texto final en su archivo (ej. 1.txt)
        const chapterFilePath = path.join(bookPath, `${chapterNum}.txt`);
        fs.writeFileSync(chapterFilePath, chapterText.trim(), "utf-8");
      }
      console.log(`✔ Libro procesado: ${bookName}`);
      countBooks++;
    }

    console.log(
      "\n¡Proceso completado! Revisa tu carpeta Bibles-ready.Libros contados ",
      countBooks,
    );
  } catch (error) {
    console.error("Ocurrió un error procesando la Biblia:", error);
  }
}

procesarBiblia();
