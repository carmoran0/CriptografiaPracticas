//cifrador de archivos usando generador massey-rueppel
//procesa archivos de 1mb en tarjeta sd
//este programa toma un archivo y lo hace secreto usando matemáticas
//luego puede volver a ser leído usando la misma clave secreta

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "massey_rueppel_generator.h"  // Cambio: usar MasseyRueppel

//configuración
#define SD_CS_PIN 5
#define BUFFER_SIZE 4096

//estructura para guardar la configuración de cada generador pequeño
struct LFSRKey {
    uint8_t size;
    uint32_t state;
    uint32_t feedback;
};

//escribe un número 4 bytes en formato little-endian
void write32le(uint8_t* dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFF);
    dst[1] = (uint8_t)((v >> 8) & 0xFF);
    dst[2] = (uint8_t)((v >> 16) & 0xFF);
    dst[3] = (uint8_t)((v >> 24) & 0xFF);
}

//crea una clave secreta de 27 bytes para el cifrado
void generateKey(uint8_t* key) {
    //configuración de los tres generadores
    LFSRKey k0 = { 8,  0x12345678, 0x0000001D };
    LFSRKey k1 = { 10, 0xABCDEF01, 0x00000205 };
    LFSRKey k2 = { 11, 0x98765432, 0x00000403 };

    key[0] = k0.size;
    write32le(&key[1], k0.state);
    write32le(&key[5], k0.feedback);

    key[9] = k1.size;
    write32le(&key[10], k1.state);
    write32le(&key[14], k1.feedback);

    key[18] = k2.size;
    write32le(&key[19], k2.state);
    write32le(&key[23], k2.feedback);
}

//guarda la clave secreta en un archivo
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

//función principal que cifra un archivo
bool encryptFile(const char* inputFile, const char* outputFile, const uint8_t* key) {
    if (!SD.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    
    File inFile = SD.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    File outFile = SD.open(outputFile, FILE_WRITE);
    if (!outFile) {
        Serial.printf("error: no se pudo crear %s\n", outputFile);
        inFile.close();
        return false;
    }
    
    MasseyRueppel masseyRueppel(key);
    
    uint8_t* buffer = (uint8_t*)malloc(BUFFER_SIZE);
    if (!buffer) {
        Serial.println("error: no hay memoria suficiente");
        inFile.close();
        outFile.close();
        return false;
    }
    
    Serial.println("\ncifrando...");
    
    while (inFile.available()) {
        size_t bytesRead = inFile.read(buffer, BUFFER_SIZE);
        
        if (bytesRead > 0) {
            masseyRueppel.processBuffer(buffer, bytesRead);
            outFile.write(buffer, bytesRead);
        }
    }
    
    free(buffer);
    inFile.close();
    outFile.close();
    
    Serial.println("\ncifrado completado");
    Serial.printf("archivo cifrado: %s\n", outputFile);
    
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\ncifrador massey-rueppel esp32\n");
    
    Serial.println("inicializando sd");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("error: no se pudo usar sd");
        Serial.println("verifica conexiones: cs=5, mosi=23, miso=19, sck=18");
        return;
    }
    Serial.println("sd montada\n");
    
    uint8_t key[27];
    
    if (SD.exists("/key_mr.txt")) {  
        Serial.println("\nclave existente encontrada, cargando...");
        File keyFile = SD.open("/key_mr.txt", FILE_READ);
        if (keyFile && keyFile.size() == 27) {
            keyFile.read(key, 27);
            keyFile.close();
            Serial.println("clave cargada desde key_mr.txt");
        } else {
            Serial.println("error: key_mr.txt corrupto, generando nueva clave");
            if (keyFile) keyFile.close();
            generateKey(key);
            saveKey("/key_mr.txt", key);
        }
    } else {
        Serial.println("\ngenerando clave nueva...");
        generateKey(key);
        if (!saveKey("/key_mr.txt", key)) {
            Serial.println("error al guardar clave");
            return;
        }
    }
    
    const char* inputFile = "/archivoOG.txt";
    const char* outputFile = "/archivo.txt.enc";
    
    if (!SD.exists(inputFile)) {
        Serial.printf("\narchivo '%s' no encontrado\n", inputFile);
        Serial.println("\nDeberias:");
        Serial.println("1. copiar tu archivo de 1mb a la sd");
        Serial.println("2. cambiar 'inputFile' en el codigo");
        Serial.println("3. reiniciar el esp32");
    } else {
        if (encryptFile(inputFile, outputFile, key)) {
            Serial.println("\noperacion exitosa");
        } else {
            Serial.println("\nerror durante el cifrado");
        }
    }
}

void loop() {
    delay(10000);
}