const fs = require("fs");
const path = require("path");

// 1. Configuración de Rutas
const inputFile = path.join(__dirname, "Jsons", "kjv.json");
const outputBaseDir = path.join(__dirname, "Bibles-ready");
const versionFolder = "KJV"; // Nombre de la carpeta principal de esta versión

// 2. Arreglo para forzar el orden bíblico correcto
// (Nota: Si tu JSON tiene los nombres en español, cambia estos nombres por Génesis, Éxodo, etc.)
const bibleOrder = [
  "Genesis",
  "Exodus",
  "Leviticus",
  "Numbers",
  "Deuteronomy",
  "Joshua",
  "Judges",
  "Ruth",
  "1 Samuel",
  "2 Samuel",
  "1 Kings",
  "2 Kings",
  "1 Chronicles",
  "2 Chronicles",
  "Ezra",
  "Nehemiah",
  "Esther",
  "Job",
  "Psalms",
  "Proverbs",
  "Ecclesiastes",
  "Song of Songs",
  "Isaiah",
  "Jeremiah",
  "Lamentations",
  "Ezekiel",
  "Daniel",
  "Hosea",
  "Joel",
  "Amos",
  "Obadiah",
  "Jonah",
  "Micah",
  "Nahum",
  "Habakkuk",
  "Zephaniah",
  "Haggai",
  "Zechariah",
  "Malachi",
  "Matthew",
  "Mark",
  "Luke",
  "John",
  "Acts",
  "Romans",
  "1 Corinthians",
  "2 Corinthians",
  "Galatians",
  "Ephesians",
  "Philippians",
  "Colossians",
  "1 Thessalonians",
  "2 Thessalonians",
  "1 Timothy",
  "2 Timothy",
  "Titus",
  "Philemon",
  "Hebrews",
  "James",
  "1 Peter",
  "2 Peter",
  "1 John",
  "2 John",
  "3 John",
  "Jude",
  "Revelation",
];

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

    // Crear la carpeta de la versión si no existe
    const versionPath = path.join(outputBaseDir, versionFolder);
    if (!fs.existsSync(versionPath)) {
      fs.mkdirSync(versionPath, { recursive: true });
    }

    // Obtener los nombres de los libros del JSON y ordenarlos según bibleOrder
    const bookNames = Object.keys(bibleData);
    bookNames.sort((a, b) => {
      const normalize = (name) => name.toLowerCase().trim();
      let indexA = bibleOrder.findIndex(
        (book) => normalize(book) === normalize(a),
      );
      let indexB = bibleOrder.findIndex(
        (book) => normalize(book) === normalize(b),
      );

      // Si un libro no se encuentra en el arreglo (por diferencias de escritura), se manda al final
      indexA = indexA === -1 ? 999 : indexA;
      indexB = indexB === -1 ? 999 : indexB;

      return indexA - indexB;
    });

    // 3. Iterar sobre los Libros en orden
    let countBooks = 0;
    for (const bookName of bookNames) {
      countBooks++;

      // Asegurar que el número tenga 2 dígitos (ej: 01, 02... 66)
      const numberPrefix = String(countBooks).padStart(2, "0");

      // Limpiar el nombre (sin espacios), en mayúsculas, tomar max 4 letras
      let shortName = bookName
        .replace(/\s+/g, "")
        .toUpperCase()
        .substring(0, 4);

      // Si el nombre es corto (ej. Job -> JOB), rellenar con "_" para garantizar exactos 7 caracteres en total
      shortName = shortName.padEnd(4, "_");

      // Construir el nombre final de 7 caracteres (ej: "01_GENE", "18_JOB_", "09_1SAM")
      const safeBookName = `${numberPrefix}_${shortName}`;
      const bookPath = path.join(versionPath, safeBookName);

      // Crear la carpeta del libro
      if (!fs.existsSync(bookPath)) {
        fs.mkdirSync(bookPath, { recursive: true });
      }

      const chapters = bibleData[bookName];

      // 4. Iterar sobre los Capítulos
      for (const chapterNum in chapters) {
        const verses = chapters[chapterNum];
        let chapterText = "";

        // Iterar sobre los Versículos y formatearlos
        for (const verseNum in verses) {
          const cleanVerse = verses[verseNum].trim();
          chapterText += `${verseNum} ${cleanVerse}\n`;
        }

        // Formatear el capítulo para que comience con cero (01.txt, 02.txt... 10.txt)
        const safeChapterNum = String(chapterNum).padStart(2, "0");
        const chapterFilePath = path.join(bookPath, `${safeChapterNum}.txt`);

        fs.writeFileSync(chapterFilePath, chapterText.trim(), "utf-8");
      }

      console.log(
        `✔ Libro procesado: ${bookName} -> Guardado como ${safeBookName}`,
      );
    }

    console.log(
      `\n¡Proceso completado! Revisa tu carpeta Bibles-ready. Libros contados: ${countBooks}`,
    );
  } catch (error) {
    console.error("Ocurrió un error procesando la Biblia:", error);
  }
}

procesarBiblia();
