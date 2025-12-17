//desencriptador de archivos usando generador geffe
//procesa archivos cifrados en tarjeta sd
//Este programa toma un archivo cifrado y lo vuelve a su forma original
//Usa la misma clave que se usó para cifrar - ¡sin la clave no funciona!

#include <Arduino.h>      //biblioteca básica para ESP32
#include <SD.h>          //para leer/escribir en tarjeta SD
#include <SPI.h>         //comunicación con el módulo SD
#include "geffe_generator.h" //nuestro generador de números secretos - MISMO que el cifrador

//configuración
#define SD_CS_PIN 5      //pin donde conectamos el cable "CS" de la SD - MISMO que cifrador
#define BUFFER_SIZE 4096 //cuántos bytes leemos de una vez (4KB) - MISMO que cifrador

//carga clave desde archivo key.txt
//Sin esta clave no podemos descifrar nada - es como la llave de una cerradura
bool loadKey(const char* filename, uint8_t* key) {
    //Primero verificamos que el archivo de clave existe
    if (!SD.exists(filename)) {
        Serial.printf("error: %s no existe\n", filename);
        return false;
    }
    
    //Abrimos el archivo de clave para leerlo
    File file = SD.open(filename, FILE_READ);
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
    
    //Leemos los 27 bytes de la clave
    file.read(key, 27);
    file.close();
    Serial.printf("clave cargada desde: %s\n", filename);
    return true;
}

//descifra archivo (idéntico a cifrar, XOR es simétrico)
//¡Esta función es casi IDÉNTICA a encryptFile! La magia del XOR
bool decryptFile(const char* inputFile, const char* outputFile, const uint8_t* key) {
    //verifica que existe el archivo cifrado
    if (!SD.exists(inputFile)) {
        Serial.printf("error: %s no existe\n", inputFile);
        return false;
    }
    
    //abre archivo de entrada (el archivo cifrado .enc)
    File inFile = SD.open(inputFile, FILE_READ);
    if (!inFile) {
        Serial.printf("error: no se pudo abrir %s\n", inputFile);
        return false;
    }
    
    //obtiene tamaño del archivo cifrado
    size_t fileSize = inFile.size();
    Serial.printf("\narchivo: %s\n", inputFile);
    Serial.printf("tamaño: %.2f mb (%d bytes)\n", fileSize / 1048576.0, fileSize);
    
    //crea archivo de salida (el archivo descifrado .dec)
    File outFile = SD.open(outputFile, FILE_WRITE);
    if (!outFile) {
        Serial.printf("error: no se pudo crear %s\n", outputFile);
        inFile.close();
        return false;
    }
    
    //crea generador geffe con la MISMA clave que usamos para cifrar
    //Si la clave es diferente, el resultado será basura
    Geffe geffe(key);
    
    //reserva memoria temporal para trabajar (buffer)
    //Procesamos el archivo por partes para no gastar toda la memoria
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
    inFile.close();    //cierra archivo cifrado
    outFile.close();   //cierra archivo descifrado
    
    //muestra resultados
    Serial.println("\ndescifrado completado");
    Serial.printf("archivo descifrado: %s\n", outputFile);
    Serial.printf("bytes procesados: %d\n", totalBytes);
    Serial.printf("tiempo: %.2f segundos\n", elapsedTime / 1000.0);
    Serial.printf("velocidad: %.2f kb/s\n", speed);
    
    return true;
}

//muestra todos los archivos que hay en la tarjeta SD
//útil para ver qué archivos tenemos para descifrar
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
    
    Serial.println("\n=== desencriptador geffe esp32 ===\n");
    
    //inicializa la tarjeta SD - MISMO proceso que el cifrador
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
    
    //carga la clave desde key.txt - ¡DEBE ser la misma que usó el cifrador!
    Serial.println("\ncargando clave...");
    uint8_t key[27]; //aquí guardaremos la clave de 27 bytes
    if (!loadKey("/key.txt", key)) {
        Serial.println("\nerror: no se pudo cargar la clave");
        Serial.println("asegurate de que key.txt existe y tiene 27 bytes");
        return;
    }
    
    //muestra qué archivos hay en la SD
    listFiles();
    
    //nombres de archivos a usar (puedes cambiar estos nombres)
    const char* inputFile = "/archivo.txt.enc";   //archivo cifrado
    const char* outputFile = "/archivo.txt.dec";  //archivo descifrado
    
    //verifica que el archivo cifrado existe
    if (!SD.exists(inputFile)) {
        Serial.printf("\narchivo '%s' no encontrado\n", inputFile);
        Serial.println("\ninstrucciones:");
        Serial.println("1. verifica que el archivo cifrado este en la sd");
        Serial.println("2. cambia 'inputFile' en el codigo");
        Serial.println("3. reinicia el esp32");
    } else {
        //si existe, procede a descifrar
        Serial.println("\niniciando descifrado...");
        delay(1000); //pequeña pausa
        
        if (decryptFile(inputFile, outputFile, key)) {
            Serial.println("\noperacion exitosa");
            listFiles(); //muestra archivos otra vez (ahora con el descifrado)
            Serial.println("\ncompara el archivo original con el descifrado");
            Serial.println("deben ser identicos");
        } else {
            Serial.println("\nerror durante el descifrado");
        }
    }
}

//función que se repite una y otra vez
//en este programa no hacemos nada en el loop
void loop() {
    delay(10000); //espera 10 segundos
}

/*
 * DESENCRIPTADOR GEFFE
 * ====================
 * 
 * REQUISITOS:
 * -----------
 * - key.txt (27 bytes) en la SD - MISMO archivo que generó el cifrador
 * - archivo.enc (archivo cifrado) en la SD - generado por el cifrador
 * - mismo geffe_generator.h usado para cifrar - ¡EXACTAMENTE el mismo!
 * 
 * USO:
 * ----
 * 1. Primero usa el programa cifrador para crear:
 *    - archivo.enc (cifrado)
 *    - key.txt (clave)
 * 2. Copia ambos a la tarjeta SD
 * 3. Conecta módulo sd al ESP32
 * 4. Cambia 'inputFile' con el nombre del .enc
 * 5. Compila y sube este código
 * 6. Abre serial monitor (115200)
 * 
 * RESULTADO:
 * ----------
 * - archivo.dec (descifrado, idéntico al original)
 * 
 * NOTA:
 * -----
 * XOR es simétrico: cifrar = descifrar
 * Por eso el código es casi idéntico al cifrador
 * Misma clave + misma secuencia + misma operación = texto original
 */