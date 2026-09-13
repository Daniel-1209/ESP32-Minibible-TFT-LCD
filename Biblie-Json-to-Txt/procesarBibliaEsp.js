const fs = require("fs");
const path = require("path");

// 1. Configuración de Rutas
const inputFile = path.join(__dirname, "Jsons", "RVR1960-Spanish.json");
const outputBaseDir = path.join(__dirname, "Bibles-ready");

async function procesarRVR1960() {
  try {
    console.log(
      "Iniciando lectura del archivo JSON RVR1960 (con subtítulos)...",
    );

    if (!fs.existsSync(inputFile)) {
      console.error(`Error: No se encontró el archivo en ${inputFile}`);
      return;
    }

    const rawData = fs.readFileSync(inputFile, "utf-8");
    const bibleData = JSON.parse(rawData);

    const versionFolder = bibleData.local_abbreviation || "RVR1960";
    const versionPath = path.join(outputBaseDir, versionFolder);

    if (!fs.existsSync(versionPath)) {
      fs.mkdirSync(versionPath, { recursive: true });
    }

    // 2. Iterar sobre los Libros
    let countBooks = 0;
    for (const book of bibleData.books) {
      const safeBookName = book.name.replace(/\s+/g, "_");
      const bookPath = path.join(versionPath, safeBookName);

      if (!fs.existsSync(bookPath)) {
        fs.mkdirSync(bookPath, { recursive: true });
      }

      // 3. Iterar sobre los Capítulos
      for (const chapter of book.chapters) {
        const chapterNum = chapter.chapter_usfm.split(".").pop();
        let chapterText = "";

        // 4. Iterar sobre los Items (Versículos y Subtítulos)
        for (const item of chapter.items) {
          // Si es un versículo normal
          if (item.type === "verse") {
            const verseNum = item.verse_numbers[0];
            const verseText = item.lines.join(" ").trim();
            chapterText += `${verseNum} ${verseText}\n`;
          }
          // Si es un subtítulo (heading1, heading2, etc.)
          else if (item.type.startsWith("heading")) {
            const headingText = item.lines.join(" ").trim();
            // Le agregamos un salto de línea antes, el # para identificarlo, y otro salto después
            chapterText += `\n# ${headingText}\n`;
          }
        }

        // Guardar el texto final eliminando espacios en blanco sobrantes al inicio/final
        const chapterFilePath = path.join(bookPath, `${chapterNum}.txt`);
        fs.writeFileSync(chapterFilePath, chapterText.trim(), "utf-8");
      }
      console.log(`✔ Libro procesado con subtítulos: ${book.name}`);
      countBooks++;
    }

    console.log(
      "\n¡Proceso completado! Tus textos ahora incluyen los títulos. Total de libros ",
      countBooks,
    );
  } catch (error) {
    console.error("Ocurrió un error procesando la Biblia:", error);
  }
}

procesarRVR1960();
