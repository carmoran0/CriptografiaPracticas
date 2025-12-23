//Programa principal
//Permite elegir entre cifrar o descifrar archivos

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "massey_rueppel_generator.h"

// Declarar funciones del cifrador
void cifrador_run();

// Declarar funciones del descifrador
void descifrador_run();

// Variable para controlar cual modo ejecutar
// 1 = cifrador, 2 = descifrador
int modo = 1; 

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== Massey-Rueppel ESP32 ===\n");
    
    Serial.println("montando SPIFFS (/data)...");
    if (!SPIFFS.begin(true)) {
        Serial.println("error: no se pudo montar SPIFFS");
        return;
    }
    Serial.println("SPIFFS montado\n");
    
    // Ejecutar el modo seleccionado
    if (modo == 1) {
        Serial.println("Modo: CIFRADOR\n");
        cifrador_run();
    } else if (modo == 2) {
        Serial.println("Modo: DESCIFRADOR\n");
        descifrador_run();
    } else {
        Serial.println("error: modo invalido. Usar 1=cifrador, 2=descifrador");
    }
}

void loop() {
    delay(10000);
}
