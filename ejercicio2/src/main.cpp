/* Plantilla proyectos arduino*/
#include "SPIFFS.h"
void listAllFiles();

// Clase LFSR (Linear Feedback Shift Register)
class Lfsr {
private:
    uint8_t size;           // Tamaño del registro
    uint32_t state;         // Estado actual del registro
    uint32_t feedback;      // Máscara de bits de realimentación (ci)
    uint32_t mask;          // Máscara para limitar el tamaño del registro

public:
    // Constructor: size = tamaño, init = configuración inicial, fb = bits de realimentación
    Lfsr(uint8_t size, uint32_t init, uint32_t fb) {
        this->size = size;
        this->state = init;
        this->feedback = fb;
        // Crear máscara para el tamaño del registro (ej: para size=4, mask=0xF)
        this->mask = (1 << size) - 1;
        // Asegurar que el estado inicial está dentro del rango
        this->state &= this->mask;
    }

    // Método next(): calcula el bit de salida y actualiza el estado
    bool next() {
        // El bit de salida es el bit menos significativo (s1)
        bool output = state & 1;
        
        // Calcular el nuevo bit con XOR de los bits marcados en feedback
        bool newBit = 0;
        uint32_t temp = state & feedback;
        
        // Realizar XOR de todos los bits en temp
        while (temp) {
            newBit ^= (temp & 1);
            temp >>= 1;
        }
        
        // Desplazar el registro a la derecha e insertar el nuevo bit a la izquierda
        state >>= 1;
        if (newBit) {
            state |= (1 << (size - 1));
        }
        
        // Aplicar máscara para mantener el tamaño
        state &= mask;
        
        return output;
    }

    // Método auxiliar para obtener el estado actual
    uint32_t getState() {
        return state;
    }

    // Método auxiliar para imprimir el estado en binario
    void printState() {
        Serial.print("Estado: ");
        for (int i = size - 1; i >= 0; i--) {
            Serial.print((state >> i) & 1);
        }
        Serial.println();
    }

    // Método para generar una secuencia de n bits y devolverla como String
    String generateSequence(int length) {
        String sequence = "";
        for (int i = 0; i < length; i++) {
            sequence += next() ? "1" : "0";
        }
        return sequence;
    }
};

void setup(){
    Serial.begin(115200);
    pinMode(43, OUTPUT);
    digitalWrite(43, HIGH);
    
    // Esperar a que el Serial esté listo
    delay(1000);
    
    Serial.println("\n=== Prueba de LFSR ===\n");
    
    // Prueba 1: LFSR(4, 0xA, 0x03)
    Serial.println("Prueba 1: Lfsr(4, 0xA, 0x03)");
    Serial.println("Configuración inicial: 0xA = 1010");
    Serial.println("Feedback: 0x03 = 0011");
    Lfsr lfsr1(4, 0xA, 0x03);
    String seq1 = lfsr1.generateSequence(32);
    Serial.print("Secuencia generada:  ");
    Serial.println(seq1);
    Serial.println("Secuencia esperada:  01011110001001101011110001001101");
    Serial.println(seq1 == "01011110001001101011110001001101" ? "✓ CORRECTO" : "✗ INCORRECTO");
    
    Serial.println("\n" + String('-', 50) + "\n");
    
    // Prueba 2: LFSR(7, 0x1C, 0x09)
    Serial.println("Prueba 2: Lfsr(7, 0x1C, 0x09)");
    Serial.println("Configuración inicial: 0x1C = 0011100");
    Serial.println("Feedback: 0x09 = 0001001");
    Lfsr lfsr2(7, 0x1C, 0x09);
    String seq2 = lfsr2.generateSequence(127);
    Serial.print("Secuencia generada (primeros 70):  ");
    Serial.println(seq2.substring(0, 70));
    Serial.println("Secuencia esperada (primeros 70):  00111001111011010000101010111110100101000110111000111111100001110111100");
    String expectedSeq2 = "00111001111011010000101010111110100101000110111000111111100001110111100101100100100000010001001100010111010110110000011001101010";
    Serial.println(seq2 == expectedSeq2 ? "✓ CORRECTO" : "✗ INCORRECTO");
    
    Serial.println("\n=== Fin de las pruebas ===\n");
    
    if(!SPIFFS.begin(true)){
        Serial.println("Fail mounting FFat");
        return;
    }
}

void loop(){
}
