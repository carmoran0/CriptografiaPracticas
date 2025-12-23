//desencriptador de archivos usando generador massey-rueppel
//procesa archivos cifrados en tarjeta sd
//fórmula: Zn = x0 ⊕ x1 ⊕ x2

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "massey_rueppel_generator.h" 
//configuracion
#define BUFFER_SIZE 4096

//carga clave desde archivo key_mr.txt
bool loadKey(const char* filename, uint8_t* key) {
    if (!SPIFFS.exists(filename)) {
        Serial.printf("error: %s no existe\n", filename);
        return false;
    }
    
    File file = SPIFFS.open(filename, FILE_READ);
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

//descifra archivo
bool decryptFile(const char* inputFile, const char* outputFile, const uint8_t* key) {
    if (!SPIFFS.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    
    File inFile = SPIFFS.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    File outFile = SPIFFS.open(outputFile, FILE_WRITE);
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
    
    Serial.println("\ndescifrando...");
    
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
    
    Serial.println("\ndescifrado completado");
    Serial.printf("archivo descifrado: %s\n", outputFile);
    
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== desencriptador massey-rueppel esp32 ===\n");
    
    Serial.println("montando SPIFFS (/data)...");
    if (!SPIFFS.begin(true)) {
        Serial.println("error: no se pudo montar SPIFFS");
        return;
    }
    Serial.println("SPIFFS montado\n");
    
    Serial.println("\ncargando clave...");
    uint8_t key[27];
    
    if (!loadKey("/key_mr.txt", key)) {
        Serial.println("\nerror: no se pudo cargar la clave");
        Serial.println("asegurate de que key_mr.txt existe y tiene 27 bytes");
        return;
    }
    
    const char* inputFile = "/archivo.txt.enc";
    const char* outputFile = "archivoOG.txt";
    
    if (!SPIFFS.exists(inputFile)) {
        Serial.printf("\narchivo '%s' no encontrado\n", inputFile);
    } else {
        if (decryptFile(inputFile, outputFile, key)) {
            Serial.println("\noperacion exitosa");
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