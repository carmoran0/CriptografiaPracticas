//cifrador de archivos usando generador geffe
//procesa archivos de 1mb en tarjeta sd

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "geffe_generator.h"

//configuración
#define SD_CS_PIN 5
#define BUFFER_SIZE 4096

//estructura para configurar cada lfsr
struct LFSRKey {
    uint8_t size;
    uint32_t state;
    uint32_t feedback;
};

//escribe uint32 en formato little-endian
void write32le(uint8_t* dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFF);
    dst[1] = (uint8_t)((v >> 8) & 0xFF);
    dst[2] = (uint8_t)((v >> 16) & 0xFF);
    dst[3] = (uint8_t)((v >> 24) & 0xFF);
}

//genera clave de 27 bytes
void generateKey(uint8_t* key) {
    //configuración de los tres lfsrs
    LFSRKey k0 = { 8,  0x12345678, 0x0000001D };
    LFSRKey k1 = { 10, 0xABCDEF01, 0x00000205 };
    LFSRKey k2 = { 11, 0x98765432, 0x00000403 };

    //lfsr0 en offset 0
    key[0] = k0.size;
    write32le(&key[1], k0.state);
    write32le(&key[5], k0.feedback);

    //lfsr1 en offset 9
    key[9] = k1.size;
    write32le(&key[10], k1.state);
    write32le(&key[14], k1.feedback);

    //lfsr2 en offset 18
    key[18] = k2.size;
    write32le(&key[19], k2.state);
    write32le(&key[23], k2.feedback);
}

//guarda clave en archivo txt
bool saveKey(const char* filename, const uint8_t* key) {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("error: no se pudo crear key.txt");
        return false;
    }
    
    file.write(key, 27);
    file.close();
    Serial.printf("clave guardada: %s\n", filename);
    return true;
}

//cifra archivo
bool encryptFile(const char* inputFile, const char* outputFile, const uint8_t* key) {
    //verifica que existe el archivo
    if (!SD.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    
    //abre archivo de entrada
    File inFile = SD.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    //abre archivo de salida
    File outFile = SD.open(outputFile, FILE_WRITE);
    if (!outFile) {
        Serial.printf("error: no se pudo crear %s\n", outputFile);
        inFile.close();
        return false;
    }
    
    //crea generador geffe
    Geffe geffe(key);
    
    //reserva buffer
    uint8_t* buffer = (uint8_t*)malloc(BUFFER_SIZE);
    if (!buffer) {
        Serial.println("error: no hay memoria suficiente");
        inFile.close();
        outFile.close();
        return false;
    }
    
    //cifra archivo
    size_t totalBytes = 0;
    unsigned long startTime = millis();
    uint8_t lastProgress = 0;
    
    Serial.println("\ncifrando...");
    
    while (inFile.available()) {
        size_t bytesRead = inFile.read(buffer, BUFFER_SIZE);
        
        if (bytesRead > 0) {
            //APLICACIÓN DEL CIFRADO: XOR con secuencia Geffe
            geffe.processBuffer(buffer, bytesRead);
            outFile.write(buffer, bytesRead);
            totalBytes += bytesRead;
            
            //muestra progreso cada 10%
            uint8_t progress = (totalBytes * 100) / fileSize;
            if (progress != lastProgress && progress % 10 == 0) {
                Serial.printf("progreso: %d%%\n", progress);
                lastProgress = progress;
            }
        }
    }
    
    unsigned long elapsedTime = millis() - startTime;
    float speed = (fileSize / 1024.0) / (elapsedTime / 1000.0);
    
    //limpia
    free(buffer);
    inFile.close();
    outFile.close();
    
    Serial.println("\ncifrado completado");
    Serial.printf("archivo cifrado: %s\n", outputFile);
    Serial.printf("bytes procesados: %d\n", totalBytes);
    Serial.printf("tiempo: %.2f segundos\n", elapsedTime / 1000.0);
    Serial.printf("velocidad: %.2f kb/s\n", speed);
    
    return true;
}

//lista archivos en sd
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
    
    Serial.println("\n=== cifrador geffe esp32 ===\n");
    
    //inicializa sd
    Serial.println("inicializando sd...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("error: no se pudo inicializar sd");
        Serial.println("verifica conexiones: cs=5, mosi=23, miso=19, sck=18");
        return;
    }
    Serial.println("sd montada\n");
    
    //info de la tarjeta
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("capacidad: %llu mb\n", cardSize);
    Serial.printf("usado: %llu mb\n", SD.usedBytes() / (1024 * 1024));
    Serial.printf("libre: %llu mb\n", (cardSize - SD.usedBytes() / (1024 * 1024)));
    
    //verifica si ya existe una clave
    uint8_t key[27];
    if (SD.exists("/key.txt")) {
        Serial.println("\nclave existente encontrada, cargando...");
        File keyFile = SD.open("/key.txt", FILE_READ);
        if (keyFile && keyFile.size() == 27) {
            keyFile.read(key, 27);
            keyFile.close();
            Serial.println("clave cargada desde key.txt");
        } else {
            Serial.println("error: key.txt corrupto, generando nueva clave");
            if (keyFile) keyFile.close();
            generateKey(key);
            saveKey("/key.txt", key);
        }
    } else {
        //genera y guarda clave nueva
        Serial.println("\ngenerando clave nueva...");
        generateKey(key);
        if (!saveKey("/key.txt", key)) {
            Serial.println("error al guardar clave");
            return;
        }
    }
    
    //lista archivos
    listFiles();
    
    //configura archivo a cifrar (CAMBIA ESTE NOMBRE)
    const char* inputFile = "/archivo1mb.txt";
    const char* outputFile = "/archivo1mb.txt.enc";
    
    //verifica que existe
    if (!SD.exists(inputFile)) {
        Serial.printf("\narchivo '%s' no encontrado\n", inputFile);
        Serial.println("\ninstrucciones:");
        Serial.println("1. copia tu archivo de 1mb a la sd");
        Serial.println("2. cambia 'inputFile' en el codigo");
        Serial.println("3. reinicia el esp32");
    } else {
        //cifra
        Serial.println("\niniciando cifrado...");
        delay(1000);
        
        if (encryptFile(inputFile, outputFile, key)) {
            Serial.println("\noperacion exitosa");
            listFiles();
        } else {
            Serial.println("\nerror durante el cifrado");
        }
    }
    
    Serial.println("\npara descifrar:");
    Serial.println("  - entrada: archivo.enc");
    Serial.println("  - salida: archivo.dec");
    Serial.println("  - misma clave (key.txt)");
}

void loop() {
    delay(10000);
}

/*
 * CONEXIONES SD - ESP32:
 * ----------------------
 * SD CS    -> GPIO 5
 * SD MOSI  -> GPIO 23
 * SD MISO  -> GPIO 19
 * SD SCK   -> GPIO 18
 * SD VCC   -> 3.3V
 * SD GND   -> GND
 * 
 * USO:
 * ----
 * 1. conecta módulo sd
 * 2. copia archivo de 1mb a la sd
 * 3. cambia 'inputFile' con el nombre
 * 4. compila y sube
 * 5. abre serial monitor (115200)
 * 
 * RESULTADO:
 * ----------
 * - archivo.enc (cifrado)
 * - key.txt (clave)
 */