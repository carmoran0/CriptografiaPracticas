//cifrador de archivos usando generador geffe
//procesa archivos de 1mb en tarjeta sd
//este programa toma un archivo y lo hace secreto usando matemáticas
//luego puede volver a ser leído usando la misma clave secreta

#include <Arduino.h>      //biblioteca básica para ESP32
#include <FS.h>           //interfaz de sistema de ficheros
#include <SPIFFS.h>       //SPIFFS (usa /data del proyecto)
#include "geffe_generator.h" //nuestro generador de números secretos

//configuración
#define BUFFER_SIZE 4096 //cuántos bytes leemos de una vez (4KB)

//estructura para guardar la configuración de cada generador pequeño 
//pensemos en esto como una receta para crear números aleatorios
struct LFSRKey {
    uint8_t size;        //cuántos bits usa este generador (ej: 8 bits)
    uint32_t state;      //número inicial para empezar a generar
    uint32_t feedback;   //cómo se mezclan los bits para crear nuevos
};

//escribe un número 4 bytes en formato little-endian
void write32le(uint8_t* dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFF);         //primer byte
    dst[1] = (uint8_t)((v >> 8) & 0xFF);  //segundo byte
    dst[2] = (uint8_t)((v >> 16) & 0xFF); //tercer byte
    dst[3] = (uint8_t)((v >> 24) & 0xFF); //cuarto byte
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
    File file = SPIFFS.open(filename, FILE_WRITE); //abre archivo para escribir
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
    if (!SPIFFS.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    
    //abre el archivo original para leerlo
    File inFile = SPIFFS.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    //obtiene tamaño del archivo
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    //crea el archivo cifrado (salida)
    File outFile = SPIFFS.open(outputFile, FILE_WRITE);
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
        }
    }
    
    //limpia todo lo que usamos
    free(buffer);      //libera la memoria temporal
    inFile.close();    //cierra archivo original
    outFile.close();   //cierra archivo cifrado
    
    //muestra resultados
    Serial.println("\ncifrado completado");
    Serial.printf("archivo cifrado: %s\n", outputFile);
    
    return true;
}


void setup() {
    Serial.begin(115200); //inicia comunicación con la computadora
    delay(2000);          //espera 2 segundos para estabilizar
    
    Serial.println("\n=== cifrador geffe esp32 ===\n");
    
    //monta el sistema de ficheros SPIFFS (contenido viene de /data)
    Serial.println("montando SPIFFS (/data)");
    if (!SPIFFS.begin(true)) { //true: formatea si fuera necesario
        Serial.println("error: no se pudo montar SPIFFS");
        return; //si falla, no podemos continuar
    }
    Serial.println("SPIFFS montado\n");
    

    //manejo de la clave secreta
    uint8_t key[27]; //aquí guardaremos la clave de 27 bytes
    
    if (SPIFFS.exists("/key.txt")) {
        //si ya existe una clave guardada, la cargamos
        Serial.println("\nclave existente encontrada, cargando...");
        File keyFile = SPIFFS.open("/key.txt", FILE_READ);
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
   
    
    const char* inputFile = "/archivoOG.txt";   //archivo original
    const char* outputFile = "/archivo.txt.enc"; //archivo cifrado
    
    //verifica que el archivo a cifrar existe
    if (!SPIFFS.exists(inputFile)) {
        Serial.printf("\narchivo '%s' no encontrado\n", inputFile);
        Serial.println("\nDeberias:");
        Serial.println("1. colocar tu archivo en la carpeta /data del proyecto");
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
    delay(10000); //espera 10 segundos
}