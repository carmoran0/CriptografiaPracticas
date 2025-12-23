//desencriptador de archivos usando generador geffe
//procesa archivos cifrados en tarjeta sd
//Este programa toma un archivo cifrado y lo vuelve a su forma original
//Usa la misma clave que se usó para cifrar - ¡sin la clave no funciona!

#include <Arduino.h>      //biblioteca básica para ESP32
#include <FS.h>           //interfaz de sistema de ficheros
#include <SPIFFS.h>       //SPIFFS (usa /data del proyecto)
#include "geffe_generator.h" //nuestro generador de números secretos - MISMO que el cifrador

//configuracion
#define BUFFER_SIZE 4096 //cuántos bytes leemos de una vez (4KB) - MISMO que cifrador

//carga clave desde archivo key.txt
//Sin esta clave no podemos descifrar nada - es como la llave de una cerradura
bool loadKey(const char* filename, uint8_t* key) {
    //Primero verificamos que el archivo de clave existe
    if (!SPIFFS.exists(filename)) {
        Serial.printf("error: %s no existe\n", filename);
        return false;
    }
    
    //Abrimos el archivo de clave para leerlo
    File file = SPIFFS.open(filename, FILE_READ);
    if (!file) {
        Serial.printf("error: no se pudo abrir %s\n", filename);
        return false;
    }
    
    //La clave DEBE tener exactamente 27 bytes (3 LFSRs × 9 bytes cada uno)
    if (file.size() != 27) {
        Serial.printf("error: %s debe tener 27 bytes (tiene %d)\n", filename, file.size());
        file.close();
        return false;
    }
    
    //leemos los 27 bytes de la clave
    file.read(key, 27);
    file.close();
    Serial.printf("clave cargada desde: %s\n", filename);
    return true;
}
//descifrra archivo (idéntico , XOR es simétrico)
bool decryptFile(const char* inputFile, const char* outputFile, const uint8_t* key) {
    //verifica que existe el archivo cifrado
    if (!SPIFFS.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    //abre archivo de entrada
    File inFile = SPIFFS.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    //obtiene tamaño del archivo cifrado
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    //crea archivo de salida (el archivo descifrado .dec)
    File outFile = SPIFFS.open(outputFile, FILE_WRITE);
    if (!outFile) {
        Serial.printf("error: no se pudo crear %s\n", outputFile);
        inFile.close();
        return false;
    }
    
    //crea generador geffe con la MISMA clave que usamos para cifrar
    Geffe geffe(key);
    
    //reserva memoria temporal para trabajar
    uint8_t* buffer = (uint8_t*)malloc(BUFFER_SIZE);
    if (!buffer) {
        Serial.println("error: no hay memoria suficiente");
        inFile.close();
        outFile.close();
        return false;
    }
    
    Serial.println("\ndescifrando...");
    
    //lee el archivo cifrado por partes hasta terminar
    while (inFile.available()) {
        //lee un pedazo del archivo (hasta BUFFER_SIZE bytes)
        size_t bytesRead = inFile.read(buffer, BUFFER_SIZE);
        
        if (bytesRead > 0) {
            //¡AQUÍ OCURRE LA MAGIA DEL DESCIFRADO!
            //Aplica EXACTAMENTE la misma operación que el cifrador
            //cifrado XOR misma_secuencia = texto_original
            geffe.processBuffer(buffer, bytesRead);
            
            //escribe el resultado descifrado en el nuevo archivo
            outFile.write(buffer, bytesRead);
        }
    }
    
    //limpia todo lo que usamos
    free(buffer);      //libera la memoria temporal
    inFile.close();    //cierra archivo cifrado
    outFile.close();   //cierra archivo descifrado
    
    //muestra resultados
    Serial.println("\ndescifrado completado");
    Serial.printf("archivo descifrado: %s\n", outputFile);
    
    return true;
}
//funcion que se ejecuta una vez al iniciar el ESP32
void setup() {
    Serial.begin(115200); //inicia comunicacion con la computadora
    delay(2000);          //espera 2 segundos para estabilizar
    
    Serial.println("\n=== desencriptador geffe esp32 ===\n");
    
    //monta el sistema de ficheros SPIFFS (contenido viene de /data)
    Serial.println("montando SPIFFS (/data)...");
    if (!SPIFFS.begin(true)) {
        Serial.println("error: no se pudo montar SPIFFS");
        return; //si falla, no podemos continuar
    }
    Serial.println("SPIFFS montado\n");
    
    //carga la clave desde key.txt - ¡DEBE ser la misma que uso el cifrador!
    Serial.println("\ncargando clave...");
    uint8_t key[27]; //aquí guardaremos la clave de 27 bytes
    if (!loadKey("/key.txt", key)) {
        Serial.println("\nerror: no se pudo cargar la clave");
        Serial.println("asegurate de que key.txt existe y tiene 27 bytes");
        return;
    }
    
    const char* inputFile = "/archivo.txt.enc";   //archivo cifrado
    const char* outputFile = "/archivoOG.txt";  //archivo descifrado (DEBE empezar con /)
    
    //verifica que el archivo cifrado existe
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
    delay(10000); //10 s
}
