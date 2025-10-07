/* Generador de Geffe con tres LFSRs */
#include <Arduino.h>
#include <stdint.h>

// Clase que implementa un LFSR simple
class LFSR {
private:
    uint32_t state;           // Estado actual del registro
    uint32_t feedback;        // Bits de realimentación
    uint8_t size;             // Tamaño del registro (número de bits)
    uint32_t mask;            // Máscara para el tamaño del registro

public:
    LFSR() : state(0), feedback(0), size(0), mask(0) {}
    
    void init(uint32_t initialState, uint32_t feedbackBits, uint8_t registerSize) {
        size = registerSize;
        state = initialState & ((1UL << size) - 1);  // Aplicar máscara según tamaño
        feedback = feedbackBits;
        mask = (1UL << size) - 1;
    }
    
    // Genera el siguiente bit
    bool next() {
        // Calcular bit de salida (bit menos significativo)
        bool outputBit = state & 1;
        
        // Calcular bit de realimentación usando XOR de los bits marcados
        uint32_t feedbackBit = 0;
        uint32_t temp = state & feedback;
        
        // Contar paridad (XOR de todos los bits activos)
        while (temp) {
            feedbackBit ^= (temp & 1);
            temp >>= 1;
        }
        
        // Desplazar y añadir bit de realimentación
        state = (state >> 1) | (feedbackBit << (size - 1));
        state &= mask;
        
        return outputBit;
    }
    
    uint32_t getState() const { return state; }
};

// Clase del generador de Geffe
class Geffe {
private:
    LFSR lfsr0, lfsr1, lfsr2;
    
    // Decodificar 9 bytes en parámetros del LFSR
    void decodeLFSRKey(const uint8_t* key, uint8_t& size, uint32_t& state, uint32_t& feedback) {
        // Primer byte: bits 4-0 contienen el tamaño (bits 7-5 se ignoran)
        size = key[0] & 0x1F;  // 5 bits menos significativos
        
        // Siguiente 4 bytes: estado inicial (little-endian)
        state = ((uint32_t)key[1]) |
                ((uint32_t)key[2] << 8) |
                ((uint32_t)key[3] << 16) |
                ((uint32_t)key[4] << 24);
        
        // Últimos 4 bytes: bits de realimentación (little-endian)
        feedback = ((uint32_t)key[5]) |
                   ((uint32_t)key[6] << 8) |
                   ((uint32_t)key[7] << 16) |
                   ((uint32_t)key[8] << 24);
    }

public:
    // Constructor: recibe clave de 27 bytes
    Geffe(const uint8_t* key) {
        uint8_t size0, size1, size2;
        uint32_t state0, state1, state2;
        uint32_t feedback0, feedback1, feedback2;
        
        // Decodificar los tres grupos de 9 bytes
        decodeLFSRKey(&key[0], size0, state0, feedback0);   // LFSR0
        decodeLFSRKey(&key[9], size1, state1, feedback1);   // LFSR1
        decodeLFSRKey(&key[18], size2, state2, feedback2);  // LFSR2
        
        // Inicializar los tres LFSRs
        lfsr0.init(state0, feedback0, size0);
        lfsr1.init(state1, feedback1, size1);
        lfsr2.init(state2, feedback2, size2);
    }
    
    // Genera el siguiente bit de la secuencia Geffe
    bool next() {
        bool x0 = lfsr0.next();
        bool x1 = lfsr1.next();
        bool x2 = lfsr2.next();
        
        // Función de Geffe: (x0 AND x1) XOR ((NOT x0) AND x2)
        return (x0 & x1) ^ (~x0 & x2);
    }
    
    // Genera un byte completo
    uint8_t nextByte() {
        uint8_t result = 0;
        for (int i = 0; i < 8; i++) {
            result = (result << 1) | (next() ? 1 : 0);
        }
        return result;
    }
};

// Ejemplo de uso
Geffe* geffe = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Generador de Geffe ===\n");
    
    // Crear una clave de ejemplo de 27 bytes
    uint8_t key[27];
    
    // LFSR0: tamaño 8, estado inicial 0x12345678, feedback 0x0000001D
    key[0] = 8;                    // Tamaño: 8 bits 00001000
    key[1] = 0x78; key[2] = 0x56;  // Estado inicial
    key[3] = 0x34; key[4] = 0x12;
    key[5] = 0x1D; key[6] = 0x00;  // Feedback
    key[7] = 0x00; key[8] = 0x00;
    
    // LFSR1: tamaño 10, estado inicial 0xABCDEF01, feedback 0x00000205
    key[9] = 10;                   // Tamaño: 10 bits
    key[10] = 0x01; key[11] = 0xEF;
    key[12] = 0xCD; key[13] = 0xAB;
    key[14] = 0x05; key[15] = 0x02;
    key[16] = 0x00; key[17] = 0x00;
    
    // LFSR2: tamaño 11, estado inicial 0x98765432, feedback 0x00000403
    key[18] = 11;                  // Tamaño: 11 bits
    key[19] = 0x32; key[20] = 0x54;
    key[21] = 0x76; key[22] = 0x98;
    key[23] = 0x03; key[24] = 0x04;
    key[25] = 0x00; key[26] = 0x00;
    
    // Crear el generador de Geffe
    geffe = new Geffe(key);
    
    // Generar y mostrar primeros 32 bits
    Serial.println("Primeros 32 bits generados:");
    for (int i = 0; i < 32; i++) {
        Serial.print(geffe->next() ? "1" : "0");
        if ((i + 1) % 8 == 0) Serial.print(" ");
    }
    Serial.println("\n");
    
    // Generar y mostrar 16 bytes
    Serial.println("Siguientes 16 bytes en hexadecimal:");
    for (int i = 0; i < 16; i++) {
        uint8_t byte = geffe->nextByte();
        Serial.printf("%02X ", byte);
        if ((i + 1) % 8 == 0) Serial.println();
    }
    Serial.println("\n=== Fin de la demostración ===");
}

void loop() {
    // Nada en el loop
    delay(1000);
}
