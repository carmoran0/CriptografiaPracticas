// desencriptador de archivos usando generador beth-piper
// procesa archivos cifrados en tarjeta sd

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "beth_piper_generator.h"

// configuración
#define SD_CS_PIN 5
#define BUFFER_SIZE 4096

// carga clave desde archivo
bool loadKey(const char* filename, uint8_t* key) {
    if (!SD.exists(filename)) {
        Serial.printf("error: %s no existe\n", filename);
        return false;
    }
    
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        Serial.printf("error: no se pudo abrir %s\n", filename);
        return false;
    }
    
    if (file.size() != 27) {
        Serial.printf("error: %s debe tener 27 bytes (tiene %d)\n", filename, file.size());
        file.close();
        return false;
    }
    
    file.read(key, 27);
    file.close();
    Serial.printf("clave cargada desde: %s\n", filename);
    return true;
}

// descifra archivo (idéntico a cifrar, XOR es simétrico)
bool decryptFile(const char* inputFile, const char* outputFile, const uint8_t* key) {
    // verifica que existe el archivo
    if (!SD.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    
    // abre archivo de entrada
    File inFile = SD.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    // abre archivo de salida
    File outFile = SD.open(outputFile, FILE_WRITE);
    if (!outFile) {
        Serial.printf("error: no se pudo crear %s\n", outputFile);
        inFile.close();
        return false;
    }
    
    // crea generador beth-piper con la misma clave
    BethPiper bethPiper(key);
    
    // reserva buffer
    uint8_t* buffer = (uint8_t*)malloc(BUFFER_SIZE);
    if (!buffer) {
        Serial.println("error: no hay memoria suficiente");
        inFile.close();
        outFile.close();
        return false;
    }
    
    // descifra archivo
    size_t totalBytes = 0;
    unsigned long startTime = millis();
    uint8_t lastProgress = 0;
    
    Serial.println("\ndescifrando con beth-piper...");
    
    while (inFile.available()) {
        size_t bytesRead = inFile.read(buffer, BUFFER_SIZE);
        
        if (bytesRead > 0) {
            bethPiper.processBuffer(buffer, bytesRead);
            outFile.write(buffer, bytesRead);
            totalBytes += bytesRead;
            
            // muestra progreso cada 10%
            uint8_t progress = (totalBytes * 100) / fileSize;
            if (progress != lastProgress && progress % 10 == 0) {
                Serial.printf("progreso: %d%%\n", progress);
                lastProgress = progress;
            }
        }
    }
    
    unsigned long elapsedTime = millis() - startTime;
    float speed = (fileSize / 1024.0) / (elapsedTime / 1000.0);
    
    // limpia
    free(buffer);
    inFile.close();
    outFile.close();
    
    Serial.println("\ndescifrado completado");
    Serial.printf("archivo descifrado: %s\n", outputFile);
    Serial.printf("bytes procesados: %d\n", totalBytes);
    Serial.printf("tiempo: %.2f segundos\n", elapsedTime / 1000.0);
    Serial.printf("velocidad: %.2f kb/s\n", speed);
    
    return true;
}

// lista archivos en sd
void listFiles() {
    Serial.println("\narchivos en sd:");
    Serial.println("--------------------------------");
    
    File root = SD.open("/");
    if (!root) {
        Serial.println("error al abrir directorio");
        return;
    }
    
    File file = root.openNextFile();
    int count = 0;
    while (file) {
        if (!file.isDirectory()) {
            Serial.printf("  %s", file.name());
            
            int spaces = 25 - strlen(file.name());
            for (int i = 0; i < spaces; i++) Serial.print(" ");
            
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

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== desencriptador beth-piper esp32 ===\n");
    
    // inicializa sd
    Serial.println("inicializando sd...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("error: no se pudo inicializar sd");
        Serial.println("verifica conexiones: cs=5, mosi=23, miso=19, sck=18");
        return;
    }
    Serial.println("sd montada\n");
    
    // info de la tarjeta
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("capacidad: %llu mb\n", cardSize);
    Serial.printf("usado: %llu mb\n", SD.usedBytes() / (1024 * 1024));
    Serial.printf("libre: %llu mb\n", (cardSize - SD.usedBytes() / (1024 * 1024)));
    
    // carga la clave
    Serial.println("\ncargando clave...");
    uint8_t key[27];
    if (!loadKey("/key_bp.txt", key)) {
        Serial.println("\nerror: no se pudo cargar la clave");
        Serial.println("asegurate de que key_bp.txt existe y tiene 27 bytes");
        return;
    }
    
    // lista archivos
    listFiles();
    
    // configura archivo a descifrar (CAMBIA ESTE NOMBRE)
    const char* inputFile = "/archivo1mb.txt.enc";
    const char* outputFile = "/archivo1mb.txt.dec";
    
    // verifica que existe
    if (!SD.exists(inputFile)) {
        Serial.printf("\narchivo '%s' no encontrado\n", inputFile);
        Serial.println("\ninstrucciones:");
        Serial.println("1. verifica que el archivo cifrado este en la sd");
        Serial.println("2. cambia 'inputFile' en el codigo");
        Serial.println("3. reinicia el esp32");
    } else {
        // descifra
        Serial.println("\niniciando descifrado...");
        delay(1000);
        
        if (decryptFile(inputFile, outputFile, key)) {
            Serial.println("\noperacion exitosa");
            listFiles();
            Serial.println("\ncompara el archivo original con el descifrado");
            Serial.println("deben ser identicos");
        } else {
            Serial.println("\nerror durante el descifrado");
        }
    }
}

void loop() {
    delay(10000);
}

/*
 * DESENCRIPTADOR BETH-PIPER
 * =========================
 * 
 * REQUISITOS:
 * -----------
 * - key_bp.txt (27 bytes) en la SD
 * - archivo.enc (archivo cifrado) en la SD
 * - mismo beth_piper_generator.h usado para cifrar
 * 
 * USO:
 * ----
 * 1. conecta módulo sd con los archivos
 * 2. cambia 'inputFile' con el nombre del .enc
 * 3. compila y sube este código
 * 4. abre serial monitor (115200)
 * 
 * RESULTADO:
 * ----------
 * - archivo.dec (descifrado, idéntico al original)
 * 
 * NOTA:
 * -----
 * XOR es simétrico: cifrar = descifrar
 * Por eso el código es casi idéntico al cifrador
 */
Claude