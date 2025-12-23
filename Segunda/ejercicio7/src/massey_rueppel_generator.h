// generador de Massey-Rueppel para cifrado
// fórmula: Zn = x0 ⊕ x1 ⊕ x2

#ifndef MASSEY_RUEPPEL_GENERATOR_H
#define MASSEY_RUEPPEL_GENERATOR_H

#include <Arduino.h>

//==================== LFSR ====================
class LFSR {
private:
    uint32_t state;
    uint32_t feedback;
    uint8_t size;
    uint32_t mask;
    
public:
    LFSR() : state(0), feedback(0), size(0), mask(0) {}
    
    void init(uint32_t initialState, uint32_t feedbackBits, uint8_t registerSize) {
        size = registerSize;
        mask = (1 << size) - 1;
        state = initialState & mask;
        feedback = feedbackBits;
    }
    
    bool next() {
        bool outputBit = state & 1;
        
        uint32_t feedbackBit = 0;
        uint32_t temp = state & feedback;
        
        while (temp) {
            feedbackBit ^= (temp & 1);
            temp >>= 1;
        }
        
        state = (state >> 1) | (feedbackBit << (size - 1));
        state &= mask;
        
        return outputBit;
    }
};

//==================== MASSEY-RUEPPEL ====================
//generador Massey-Rueppel: Zn = x0 ⊕ x1 ⊕ x2
class MasseyRueppel {
private:
    LFSR lfsr0, lfsr1, lfsr2;
    
    void decodeLFSRKey(const uint8_t* key, uint8_t& size, uint32_t& state, uint32_t& feedback) {
        size = key[0] & 0x1F;
        
        state = ((uint32_t)key[1]) |
                ((uint32_t)key[2] << 8) |
                ((uint32_t)key[3] << 16) |
                ((uint32_t)key[4] << 24);
        
        feedback = ((uint32_t)key[5]) |
                   ((uint32_t)key[6] << 8) |
                   ((uint32_t)key[7] << 16) |
                   ((uint32_t)key[8] << 24);
    }

public:
    //Constructor con clave de 27 bytes
    MasseyRueppel(const uint8_t* key) {
        uint8_t size0, size1, size2;
        uint32_t state0, state1, state2;
        uint32_t feedback0, feedback1, feedback2;
        
        decodeLFSRKey(&key[0], size0, state0, feedback0);
        decodeLFSRKey(&key[9], size1, state1, feedback1);
        decodeLFSRKey(&key[18], size2, state2, feedback2);
        
        lfsr0.init(state0, feedback0, size0);
        lfsr1.init(state1, feedback1, size1);
        lfsr2.init(state2, feedback2, size2);
    }
    
    //siguiente bit: x0 ⊕ x1 ⊕ x2
    bool next() {
        bool x0 = lfsr0.next();
        bool x1 = lfsr1.next();
        bool x2 = lfsr2.next();
        
        // fórmula de Massey-Rueppel
        return x0 ^ x1 ^ x2;
    }
    
    uint8_t nextByte() {
        uint8_t result = 0;
        for (int i = 0; i < 8; i++) {
            result = (result << 1) | (next() ? 1 : 0);
        }
        return result;
    }
    
    void processBuffer(uint8_t* buffer, size_t length) {
        for (size_t i = 0; i < length; i++) {
            buffer[i] ^= nextByte();
        }
    }
};

#endif