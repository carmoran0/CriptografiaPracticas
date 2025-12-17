//cifrador de archivos usando generador geffe
//procesa archivos de 1mb en tarjeta sd
//este programa toma un archivo y lo hace secreto usando matemáticas
//luego puede volver a ser leído usando la misma clave secreta

#include <Arduino.h>      //biblioteca básica para ESP32
#include <SD.h>          //para leer/escribir en tarjeta SD
#include <SPI.h>         //comunicación con el módulo SD
#include "geffe_generator.h" //nuestro generador de números secretos

//configuración
#define SD_CS_PIN 5      //pin donde conectamos el cable "CS" de la SD
#define BUFFER_SIZE 4096 //cuántos bytes leemos de una vez (4KB)

//estructura para guardar la configuración de cada generador pequeño (LFSR)
//pensemos en esto como una receta para crear números aleatorios
struct LFSRKey {
    uint8_t size;        //cuántos bits usa este generador (ej: 8 bits)
    uint32_t state;      //número inicial para empezar a generar
    uint32_t feedback;   //cómo se mezclan los bits para crear nuevos
};

//escribe un número grande (32 bits) en formato pequeño (little-endian)
//esto es como escribir un número de atrás hacia adelante
void write32le(uint8_t* dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFF);         //primer byte: parte más baja
    dst[1] = (uint8_t)((v >> 8) & 0xFF);  //segundo byte
    dst[2] = (uint8_t)((v >> 16) & 0xFF); //tercer byte
    dst[3] = (uint8_t)((v >> 24) & 0xFF); //cuarto byte: parte más alta
}

//crea una clave secreta de 27 bytes para el cifrado
//27 bytes = 3 generadores pequeños × 9 bytes cada uno
void generateKey(uint8_t* key) {
    //configuración fija de los tres generadores pequeños
    //esto es como tener tres máquinas de números aleatorios
    LFSRKey k0 = { 8,  0x12345678, 0x0000001D }; //máquina 1: 8 bits
    LFSRKey k1 = { 10, 0xABCDEF01, 0x00000205 }; //máquina 2: 10 bits
    LFSRKey k2 = { 11, 0x98765432, 0x00000403 }; //máquina 3: 11 bits

    //escribe configuración de máquina 1 en bytes 0-8
    key[0] = k0.size;                     //byte 0: tamaño (8 bits)
    write32le(&key[1], k0.state);         //bytes 1-4: número inicial
    write32le(&key[5], k0.feedback);      //bytes 5-8: fórmula de mezcla

    //escribe configuración de máquina 2 en bytes 9-17
    key[9] = k1.size;                     //byte 9: tamaño (10 bits)
    write32le(&key[10], k1.state);        //bytes 10-13: número inicial
    write32le(&key[14], k1.feedback);     //bytes 14-17: fórmula de mezcla

    //escribe configuración de máquina 3 en bytes 18-26
    key[18] = k2.size;                    //byte 18: tamaño (11 bits)
    write32le(&key[19], k2.state);        //bytes 19-22: número inicial
    write32le(&key[23], k2.feedback);     //bytes 23-26: fórmula de mezcla
}

//guarda la clave secreta en un archivo llamado key.txt
//esto es importante porque sin la clave no podemos descifrar
bool saveKey(const char* filename, const uint8_t* key) {
    File file = SD.open(filename, FILE_WRITE); //abre archivo para escribir
    if (!file) {
        Serial.println("error: no se pudo crear key.txt");
        return false;
    }
    
    file.write(key, 27); //escribe los 27 bytes de la clave
    file.close();        //cierra el archivo
    Serial.printf("clave guardada: %s\n", filename);
    return true;
}

//función principal que cifra un archivo
//toma un archivo normal y lo convierte en secreto
bool encryptFile(const char* inputFile, const char* outputFile, const uint8_t* key) {
    //primero verifica que el archivo original existe
    if (!SD.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    
    //abre el archivo original para leerlo
    File inFile = SD.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    //obtiene tamaño del archivo
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    //crea el archivo cifrado (salida)
    File outFile = SD.open(outputFile, FILE_WRITE);
    if (!outFile) {
        Serial.printf("error: no se pudo crear %s\n", outputFile);
        inFile.close();
        return false;
    }
    
    //crea el generador Geffe con nuestra clave secreta
    //esto es como una máquina que produce números aleatorios
    Geffe geffe(key);
    
    //reserva memoria temporal para trabajar (buffer)
    //como no podemos procesar todo de una vez, procesamos por pedazos
    uint8_t* buffer = (uint8_t*)malloc(BUFFER_SIZE);
    if (!buffer) {
        Serial.println("error: no hay memoria suficiente");
        inFile.close();
        outFile.close();
        return false;
    }
    
    //variables para medir progreso y tiempo
    size_t totalBytes = 0;           //cuántos bytes hemos procesado
    unsigned long startTime = millis(); //cuándo empezamos
    uint8_t lastProgress = 0;        //último porcentaje mostrado
    
    Serial.println("\ncifrando...");
    
    //lee el archivo por partes hasta terminar
    while (inFile.available()) {
        //lee un pedazo del archivo (hasta BUFFER_SIZE bytes)
        size_t bytesRead = inFile.read(buffer, BUFFER_SIZE);
        
        if (bytesRead > 0) {
            //¡AQUÍ OCURRE LA MAGIA DEL CIFRADO!
            //mezcla cada byte con un número aleatorio del generador Geffe
            geffe.processBuffer(buffer, bytesRead);
            
            //escribe el resultado cifrado en el nuevo archivo
            outFile.write(buffer, bytesRead);
            totalBytes += bytesRead;
            
            //muestra progreso cada 10% (para saber que está avanzando)
            uint8_t progress = (totalBytes * 100) / fileSize;
            if (progress != lastProgress && progress % 10 == 0) {
                Serial.printf("progreso: %d%%\n", progress);
                lastProgress = progress;
            }
        }
    }
    
    //calcula estadísticas del proceso
    unsigned long elapsedTime = millis() - startTime;
    float speed = (fileSize / 1024.0) / (elapsedTime / 1000.0);
    
    //limpia todo lo que usamos
    free(buffer);      //libera la memoria temporal
    inFile.close();    //cierra archivo original
    outFile.close();   //cierra archivo cifrado
    
    //muestra resultados
    Serial.println("\ncifrado completado");
    Serial.printf("archivo cifrado: %s\n", outputFile);
    Serial.printf("bytes procesados: %d\n", totalBytes);
    Serial.printf("tiempo: %.2f segundos\n", elapsedTime / 1000.0);
    Serial.printf("velocidad: %.2f kb/s\n", speed);
    
    return true;
}

//muestra todos los archivos que hay en la tarjeta SD
//útil para ver qué archivos podemos cifrar
void listFiles() {
    Serial.println("\narchivos en sd:");
    Serial.println("--------------------------------");
    
    //abre el directorio raíz (donde están todos los archivos)
    File root = SD.open("/");
    if (!root) {
        Serial.println("error al abrir directorio");
        return;
    }
    
    //lee archivo por archivo
    File file = root.openNextFile();
    int count = 0;
    while (file) {
        if (!file.isDirectory()) { //solo archivos, no carpetas
            Serial.printf("  %s", file.name()); //nombre del archivo
            
            //agrega espacios para que quede alineado
            int spaces = 25 - strlen(file.name());
            for (int i = 0; i < spaces; i++) Serial.print(" ");
            
            //muestra tamaño en formato legible
            size_t size = file.size();
            if (size >= 1048576) {
                Serial.printf("%.2f mb\n", size / 1048576.0);
            } else if (size >= 1024) {
                Serial.printf("%.2f kb\n", size / 1024.0);
            } else {
                Serial.printf("%d bytes\n", size);
            }
            count++;
        }
        file = root.openNextFile();
    }
    
    if (count == 0) {
        Serial.println("  (vacio)");
    }
    Serial.println("--------------------------------");
}

//función que se ejecuta una vez al iniciar el ESP32
void setup() {
    Serial.begin(115200); //inicia comunicación con la computadora
    delay(2000);          //espera 2 segundos para estabilizar
    
    Serial.println("\n=== cifrador geffe esp32 ===\n");
    
    //inicializa la tarjeta SD
    Serial.println("inicializando sd...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("error: no se pudo inicializar sd");
        Serial.println("verifica conexiones: cs=5, mosi=23, miso=19, sck=18");
        return; //si falla, no podemos continuar
    }
    Serial.println("sd montada\n");
    
    //muestra información de la tarjeta SD
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("capacidad: %llu mb\n", cardSize);
    Serial.printf("usado: %llu mb\n", SD.usedBytes() / (1024 * 1024));
    Serial.printf("libre: %llu mb\n", (cardSize - SD.usedBytes() / (1024 * 1024)));
    
    //manejo de la clave secreta
    uint8_t key[27]; //aquí guardaremos la clave de 27 bytes
    
    if (SD.exists("/key.txt")) {
        //si ya existe una clave guardada, la cargamos
        Serial.println("\nclave existente encontrada, cargando...");
        File keyFile = SD.open("/key.txt", FILE_READ);
        if (keyFile && keyFile.size() == 27) {
            keyFile.read(key, 27); //lee la clave del archivo
            keyFile.close();
            Serial.println("clave cargada desde key.txt");
        } else {
            //si el archivo está dañado, creamos clave nueva
            Serial.println("error: key.txt corrupto, generando nueva clave");
            if (keyFile) keyFile.close();
            generateKey(key);         //crea nueva clave
            saveKey("/key.txt", key); //guarda en archivo
        }
    } else {
        //si no existe clave, creamos una nueva
        Serial.println("\ngenerando clave nueva...");
        generateKey(key); //crea la clave
        if (!saveKey("/key.txt", key)) {
            Serial.println("error al guardar clave");
            return;
        }
    }
    
    //muestra qué archivos hay en la SD
    listFiles();
    
    //nombres de archivos a usar (puedes cambiar estos nombres)
    const char* inputFile = "/archivoOG.txt";   //archivo original
    const char* outputFile = "/archivo.txt.enc"; //archivo cifrado
    
    //verifica que el archivo a cifrar existe
    if (!SD.exists(inputFile)) {
        Serial.printf("\narchivo '%s' no encontrado\n", inputFile);
        Serial.println("\ninstrucciones:");
        Serial.println("1. copia tu archivo de 1mb a la sd");
        Serial.println("2. cambia 'inputFile' en el codigo");
        Serial.println("3. reinicia el esp32");
    } else {
        //si existe, procede a cifrar
        Serial.println("\niniciando cifrado...");
        delay(1000); //pequeña pausa
        
        if (encryptFile(inputFile, outputFile, key)) {
            Serial.println("\noperacion exitosa");
            listFiles(); //muestra archivos otra vez (ahora con el cifrado)
        } else {
            Serial.println("\nerror durante el cifrado");
        }
    }
    
    //instrucciones para descifrar
    Serial.println("\npara descifrar:");
    Serial.println("  - entrada: archivo.enc");
    Serial.println("  - salida: archivo.dec");
    Serial.println("  - misma clave (key.txt)");
}

//función que se repite una y otra vez
//en este programa no hacemos nada en el loop
void loop() {
    delay(10000); //espera 10 segundos
}

/*
 * CÓMO CONECTAR TODO:
 * ----------------------
 * Tarjeta SD -> ESP32:
 * CS   (chip select) -> GPIO 5
 * MOSI (data in)     -> GPIO 23
 * MISO (data out)    -> GPIO 19
 * SCK  (clock)       -> GPIO 18
 * VCC  (power)       -> 3.3V
 * GND  (ground)      -> GND
 * 
 * CÓMO USAR ESTE PROGRAMA:
 * ------------------------
 * 1. Conecta el módulo SD al ESP32 como se indica arriba
 * 2. Copia un archivo de 1MB a la tarjeta SD (desde tu computadora)
 * 3. Cambia 'inputFile' en el código con el nombre de tu archivo
 * 4. Compila y sube el código al ESP32
 * 5. Abre el Serial Monitor (115200 baudios)
 * 
 * QUÉ OBTENDRÁS:
 * --------------
 * - Un archivo .enc (tu archivo cifrado, ilegible)
 * - Un archivo key.txt (la clave para descifrar, ¡GUÁRDALA!)
 * 
 * CÓMO DESCIFRAR:
 * ---------------
 * Ejecuta el mismo programa pero intercambiando los nombres:
 * inputFile = "archivo.enc"
 * outputFile = "archivo.dec"
 * ¡Usa la MISMA clave (key.txt)!
 */