/* Generador de Geffe con tres LFSRs */
#include "SPIFFS.h"
#include <Arduino.h>
#include <stdint.h>

//Zn​=(x0​∧x1​)⊕(¬x0​∧x2​)
//X0,X1,X2 bits de salida de los tres LFSR


class LFSR {
private:
    uint32_t state;           //Estado actual del registro
    uint32_t feedback;        //Bits de realimentación
    uint8_t size;             //Tamaño del registro (número de bits)
    uint32_t mask;            //Máscara para el tamaño del registro

    
public:
    LFSR() : state(0), feedback(0), size(0), mask(0) {} //se inician estos parametros a 0
    /*Es como escribir:
    LFSR() {
    state = 0;
    feedback = 0;
    size = 0;
    mask = 0;
    }*/ 
    
    void init(uint32_t initialState, uint32_t feedbackBits, uint8_t registerSize) {
        size = registerSize;
        state = initialState & ((1UL << size) - 1);  // Aplicar máscara según tamaño
        feedback = feedbackBits;
        mask = (1UL << size) - 1; //asegurarse de que el estado inicial tiene solo los bits necesarios ((1UL << size) - 1
    }
    
    // Genera el siguiente bit y actualiza el estado
    bool next() {
        // Calcular bit de salida (bit menos significativo)
        bool outputBit = state & 1;
        
        // Calcular bit de realimentación usando XOR de los bits marcados
        uint32_t feedbackBit = 0;
        uint32_t temp = state & feedback; //seleccion de los bits que se usan
        
        // Contar paridad (XOR de todos los bits activos)
        while (temp) {
            feedbackBit ^= (temp & 1);
            temp >>= 1; //desplazamiento hacia la derecha
        }
        
        // Desplazar y añadir bit de realimentación
        state = (state >> 1) | (feedbackBit << (size - 1));
        state &= mask;
        
        return outputBit;
    }
    
    uint32_t getState() const { return state; }//devuelve el valor actual de estado interno
};

// Clase del generador de Geffe
class Geffe {
private:
    LFSR lfsr0, lfsr1, lfsr2;
    
    // Decodificar 9 bytes en parametros del LFSR
    void decodeLFSRKey(const uint8_t* key, uint8_t& size, uint32_t& state, uint32_t& feedback) {
        // Primer byte: bits 4-0 contienen el tamaño (bits 7-5 se ignoran)
        size = key[0] & 0x1F;  //5 bits menos significativos se quedan como 1, el resto se quedan en 0
        
        // Siguiente 4 bytes: estado inicial (little-endian)
        //se hace esto para combinar los bytes y asignarselos a una variable de 32 bits
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
        decodeLFSRKey(&key[0], size0, state0, feedback0);   //LFSR0
        decodeLFSRKey(&key[9], size1, state1, feedback1);   //LFSR1, se le da la llave empezando desde la 9º cosa, por lo que se omiten los primeros 8 elementos
        decodeLFSRKey(&key[18], size2, state2, feedback2);  //LFSR2, ahora se omiten los 16 primeros
        
        // Inicializar los tres LFSRs
        lfsr0.init(state0, feedback0, size0);
        lfsr1.init(state1, feedback1, size1);
        lfsr2.init(state2, feedback2, size2);
    }
    
    //Genera el siguiente bit de la secuencia Geffe
    bool next() {
        bool x0 = lfsr0.next();
        bool x1 = lfsr1.next();
        bool x2 = lfsr2.next();
        
        //(x0 AND x1) XOR ((NOT x0) AND x2)
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


struct LFSRKey {
    uint8_t size;
    uint32_t state;
    uint32_t feedback;
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Generador de Geffe ===\n");
    
    
    //creamos los LFSRKeys que necesitamos
    LFSRKey k0 = { 8,  0x12345678, 0x0000001D };
    LFSRKey k1 = { 10, 0xABCDEF01, 0x00000205 };
    LFSRKey k2 = { 11, 0x98765432, 0x00000403 };

    //para colocar todo en bytes usamos la funcion memcpy(destino, origen, cantidad_de_bytes)
    uint8_t key[27];

    auto write32le = [](uint8_t* dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFF);
    dst[1] = (uint8_t)((v >> 8) & 0xFF);
    dst[2] = (uint8_t)((v >> 16) & 0xFF);
    dst[3] = (uint8_t)((v >> 24) & 0xFF);
    };

    // k0 en offset 0
    key[0] = k0.size;
    write32le(&key[1], k0.state); // bytes 1..4
    write32le(&key[5], k0.feedback); // bytes 5..8

    // k1 en offset 9
    key[9] = k1.size;
    write32le(&key[10], k1.state); // bytes 10..13
    write32le(&key[14], k1.feedback); // bytes 14..17

    // k2 en offset 18
    key[18] = k2.size;
    write32le(&key[19], k2.state); // bytes 19..22
    write32le(&key[23], k2.feedback); // bytes 23..26


    Geffe geffe(key);
    

    //generar y mostrar primeros 32 bits
    Serial.println("Primeros 32 bits generados:");

    for (int i = 0; i < 32; i++) {
        Serial.print(geffe->next() ? "1" : "0");
        if ((i + 1) % 8 == 0) Serial.print(" ");
    }
    Serial.println("\n");
    
    //Generar y mostrar 16 bytes
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
