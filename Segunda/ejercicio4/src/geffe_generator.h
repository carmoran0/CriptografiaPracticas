//cifrador de archivos usando generador geffe
//procesa archivos de 1mb en tarjeta sd

#ifndef GEFFE_GENERATOR_H
#define GEFFE_GENERATOR_H

#include <Arduino.h>

//==================== LFSR ====================
class LFSR {
private:
    uint32_t state;          //estado actual del registro
    uint32_t feedback;       //máscara de bits de realimentación
    uint8_t size;            //tamaño del registro en bits
    uint32_t mask;           //máscara para limitar tamaño

public:
    //constructor por defecto
    LFSR() : state(0), feedback(0), size(0), mask(0) {}
    
    //inicializa el LFSR con parámetros específicos
    void init(uint32_t initialState, uint32_t feedbackBits, uint8_t registerSize) {
        size = registerSize;
        //aplica máscara para mantener solo bits válidos
        state = initialState & ((1UL << size) - 1);
        feedback = feedbackBits;
        mask = (1UL << size) - 1;
    }
    
    //genera el siguiente bit y actualiza el estado
    bool next() {
        //bit de salida es el menos significativo
        bool outputBit = state & 1;
        
        //calcula bit de realimentación usando XOR
        uint32_t feedbackBit = 0;
        uint32_t temp = state & feedback; //selecciona bits de realimentación
        
        //XOR de todos los bits seleccionados
        while (temp) {
            feedbackBit ^= (temp & 1);
            temp >>= 1;
        }
        
        //desplazar y añadir bit de realimentación al inicio
        state = (state >> 1) | (feedbackBit << (size - 1));
        state &= mask; //aplica máscara para mantener tamaño
        
        return outputBit;
    }
};

//==================== GEFFE ====================
class Geffe {
private:
    LFSR lfsr0, lfsr1, lfsr2; //tres LFSRs independientes
    
    //decodifica 9 bytes en parámetros del LFSR
    void decodeLFSRKey(const uint8_t* key, uint8_t& size, uint32_t& state, uint32_t& feedback) {
        //primer byte: tamaño en bits (5 bits menos significativos)
        size = key[0] & 0x1F;
        
        //siguiente 4 bytes: estado inicial (formato little-endian)
        state = ((uint32_t)key[1]) |
                ((uint32_t)key[2] << 8) |
                ((uint32_t)key[3] << 16) |
                ((uint32_t)key[4] << 24);
        
        //últimos 4 bytes: bits de realimentación
        feedback = ((uint32_t)key[5]) |
                   ((uint32_t)key[6] << 8) |
                   ((uint32_t)key[7] << 16) |
                   ((uint32_t)key[8] << 24);
    }

public:
    //constructor: recibe clave de 27 bytes (9 bytes por LFSR)
    Geffe(const uint8_t* key) {
        uint8_t size0, size1, size2;
        uint32_t state0, state1, state2;
        uint32_t feedback0, feedback1, feedback2;
        
        //decodifica los tres grupos de 9 bytes
        decodeLFSRKey(&key[0], size0, state0, feedback0);  //LFSR0
        decodeLFSRKey(&key[9], size1, state1, feedback1);  //LFSR1
        decodeLFSRKey(&key[18], size2, state2, feedback2); //LFSR2
        
        //inicializa los tres LFSRs
        lfsr0.init(state0, feedback0, size0);
        lfsr1.init(state1, feedback1, size1);
        lfsr2.init(state2, feedback2, size2);
    }
    
    //genera el siguiente bit: Zn = (X0 ∧ X1) ⊕ (¬X0 ∧ X2)
    bool next() {
        bool x0 = lfsr0.next();
        bool x1 = lfsr1.next();
        bool x2 = lfsr2.next();
        //fórmula del generador de Geffe
        return (x0 & x1) ^ (~x0 & x2);
    }
    
    //genera un byte completo (8 bits)
    uint8_t nextByte() {
        uint8_t result = 0;
        for (int i = 0; i < 8; i++) {
            //construye byte bit a bit
            result = (result << 1) | (next() ? 1 : 0);
        }
        return result;
    }
    
    //cifra/descifra buffer aplicando XOR con secuencia Geffe
    void processBuffer(uint8_t* buffer, size_t length) {
        for (size_t i = 0; i < length; i++) {
            //XOR entre byte del buffer y byte de la secuencia
            buffer[i] ^= nextByte();
        }
    }
};

#endif //GEFFE_GENERATOR_H