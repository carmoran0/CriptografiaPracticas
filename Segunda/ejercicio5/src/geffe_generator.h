//archivo de cabecera para el generador Geffe
//Este archivo contiene las "recetas" para crear números aleatorios
//Es EXACTAMENTE el mismo que usa el programa cifrador

#include <Arduino.h>  //biblioteca básica

#ifndef GEFFE_GENERATOR_H  //Esto evita que el archivo se incluya dos veces
#define GEFFE_GENERATOR_H  //Si no está definido, lo define ahora

// ==================== LFSR ====================
//LFSR = Linear Feedback Shift Register
//Es como una máquina pequeña que genera números "casi aleatorios"
//Funciona tomando bits de un número y mezclándolos matemáticamente
class LFSR {
private:
    uint32_t state;          //número actual dentro de la máquina
    uint32_t feedback;       //instrucciones de cómo mezclar bits
    uint8_t size;            //cuántos bits puede guardar (ej: 8, 10, 11)
    uint32_t mask;           //límite para no pasarse del tamaño

public:
    //Constructor por defecto - pone todo en cero al principio
    LFSR() : state(0), feedback(0), size(0), mask(0) {}
    
    //Configura la máquina con valores específicos
    //Como darle instrucciones a la máquina para que funcione
    void init(uint32_t initialState, uint32_t feedbackBits, uint8_t registerSize) {
        size = registerSize;  //Guardamos el tamaño (ej: 8 bits)
        //Aplica máscara para mantener solo bits válidos
        //Si size=8, (1UL << 8) = 256, -1 = 255 (11111111 en binario)
        state = initialState & ((1UL << size) - 1);
        feedback = feedbackBits;  //Guardamos cómo se mezclan los bits
        mask = (1UL << size) - 1; //Creamos la máscara para este tamaño
    }
    
    //Genera el siguiente bit (0 o 1) y actualiza el estado
    //Es como pedirle a la máquina que produzca un nuevo número
    bool next() {
        bool outputBit = state & 1; //Toma el bit más a la derecha
        
        //Calcula bit de realimentación usando XOR
        //XOR es como "es diferente": 0⊕0=0, 0⊕1=1, 1⊕0=1, 1⊕1=0
        uint32_t feedbackBit = 0;
        uint32_t temp = state & feedback; //Selecciona bits marcados
        
        //XOR de todos los bits seleccionados, uno por uno
        while (temp) {
            feedbackBit ^= (temp & 1); //Hace XOR con el bit actual
            temp >>= 1; //Pasa al siguiente bit
        }
        
        //Desplazar y añadir bit de realimentación
        //Mueve todos los bits a la derecha y mete el nuevo al principio
        state = (state >> 1) | (feedbackBit << (size - 1));
        state &= mask; //Asegura que no tenga más bits de los permitidos
        
        return outputBit; //Devuelve el bit que sacamos al principio
    }
};

// ==================== GEFFE ====================
//Generador de Geffe - combina 3 LFSRs para ser más seguro
//Es como tener 3 máquinas pequeñas trabajando juntas
//La fórmula hace que sea difícil adivinar el resultado
class Geffe {
private:
    LFSR lfsr0, lfsr1, lfsr2; //Tres LFSRs independientes
    
    //Convierte 9 bytes en configuración para una máquina LFSR
    //Lee la "receta" guardada en bytes
    void decodeLFSRKey(const uint8_t* key, uint8_t& size, uint32_t& state, uint32_t& feedback) {
        //Primer byte: tamaño (5 bits menos significativos)
        //0x1F = 00011111 en binario - toma solo los primeros 5 bits
        size = key[0] & 0x1F;
        
        //Siguiente 4 bytes: estado inicial (little-endian)
        //Little-endian significa: byte más pequeño primero
        state = ((uint32_t)key[1]) |
                ((uint32_t)key[2] << 8) |  //Mueve 8 bits a la izquierda
                ((uint32_t)key[3] << 16) | //Mueve 16 bits a la izquierda
                ((uint32_t)key[4] << 24);  //Mueve 24 bits a la izquierda
        
        //Últimos 4 bytes: bits de realimentación (little-endian)
        //Mismo proceso para la máscara de realimentación
        feedback = ((uint32_t)key[5]) |
                   ((uint32_t)key[6] << 8) |
                   ((uint32_t)key[7] << 16) |
                   ((uint32_t)key[8] << 24);
    }

public:
    //Constructor: recibe clave de 27 bytes
    //27 bytes = 3 máquinas × 9 bytes cada una
    Geffe(const uint8_t* key) {
        uint8_t size0, size1, size2;        //Tamaños de las 3 máquinas
        uint32_t state0, state1, state2;    //Estados iniciales
        uint32_t feedback0, feedback1, feedback2; //Fórmulas de mezcla
        
        //Decodifica los tres grupos de 9 bytes
        //Cada grupo configura una máquina diferente
        decodeLFSRKey(&key[0], size0, state0, feedback0);  //Máquina 1
        decodeLFSRKey(&key[9], size1, state1, feedback1);  //Máquina 2
        decodeLFSRKey(&key[18], size2, state2, feedback2); //Máquina 3
        
        //Inicializa los tres LFSRs con sus configuraciones
        //Ahora las 3 máquinas están listas para trabajar
        lfsr0.init(state0, feedback0, size0);
        lfsr1.init(state1, feedback1, size1);
        lfsr2.init(state2, feedback2, size2);
    }
    
    //Genera el siguiente bit: zn = (x0 ∧ x1) ⊕ (¬x0 ∧ x2)
    //Fórmula del generador de Geffe que mezcla los 3 bits
    //∧ = AND (y), ¬ = NOT (no), ⊕ = XOR (o exclusivo)
    bool next() {
        bool x0 = lfsr0.next(); //Pide bit a máquina 1
        bool x1 = lfsr1.next(); //Pide bit a máquina 2
        bool x2 = lfsr2.next(); //Pide bit a máquina 3
        return (x0 & x1) ^ (~x0 & x2); //Fórmula de Geffe
    }
    
    //Genera un byte completo (8 bits)
    //Un byte = 8 bits juntos, como una letra o número pequeño
    uint8_t nextByte() {
        uint8_t result = 0; //Empezamos con 00000000 (todo ceros)
        for (int i = 0; i < 8; i++) {
            //Construye byte bit a bit
            //Mueve todo a la izquierda y mete el nuevo bit al final
            result = (result << 1) | (next() ? 1 : 0);
        }
        return result; //Devuelve el byte completo
    }
    
    //Cifra/descifra buffer aplicando XOR con secuencia Geffe
    //¡Esta es la función más importante! Hace la magia del cifrado/descifrado
    //XOR tiene una propiedad mágica: si lo aplicas dos veces, vuelves al original
    //texto XOR secreto = cifrado, cifrado XOR mismo secreto = texto
    void processBuffer(uint8_t* buffer, size_t length) {
        for (size_t i = 0; i < length; i++) {
            //XOR entre byte del buffer y byte de la secuencia Geffe
            //Cada byte se mezcla con un byte "aleatorio"
            buffer[i] ^= nextByte();
        }
    }
};

#endif //GEFFE_GENERATOR_H - fin del archivo