// este h es para el CIFRADOR DE UN ARCHIVO LLAMADO "archivoOG.txt"
//este h sirve para generar el numero del geffe
//Se puede usar en otros programas incluyendo este archivo

#ifndef GEFFE_GENERATOR_H  //para evitar que el archivo se duplique
#define GEFFE_GENERATOR_H  //por si no estaba definido

#include <Arduino.h>  

//==================== LFSR ====================
//toma bits de un numero y los entremezcla y tal
class LFSR {
private:
    uint32_t state;          //Estado actual del registro
    uint32_t feedback;       //Mascara de bits de realimentación - qué bits se mezclan
    uint8_t size;            //Tamaño del registro en bits
    uint32_t mask;           //Máscara para no pasarse de la raya con los bits
public:
    //poner todo en cero al principio
    LFSR() : state(0), feedback(0), size(0), mask(0) {}
    
void init(uint32_t initialState, uint32_t feedbackBits, uint8_t registerSize) {
    size = registerSize;  //cuántos bits usamos
    
    //mascara era el número más grande que cabe en 'size' bits
    mask = (1 << size) - 1;  //(2^size) - 1 , se desplaza size veces hacia la derecha, y se le resta 1, ej si size 3 entonces seria 1000-1=111 el cual tiene 3 bits
    
    //estado inicial con el tamaño decidido
    state = initialState & mask;
    feedback = feedbackBits;
}
    //Genera el siguiente bit (0 o 1) y actualiza el estado de la máquina
    //Es como pedirle a la máquina que produzca un nuevo número
    bool next() {
        //Bit de salida es el menos significativo (el de más a la derecha)
        bool outputBit = state & 1;
        
        //Calcula bit de realimentación usando XOR
        //XOR es como "es diferente": 0⊕0=0, 0⊕1=1, 1⊕0=1, 1⊕1=0
        uint32_t feedbackBit = 0;
        uint32_t temp = state & feedback; //Selecciona solo los bits marcados para realimentación
        
        //XOR de todos los bits seleccionados, uno por uno
        //Es como sumar los bits pero solo si son diferentes
        while (temp) {
            feedbackBit ^= (temp & 1); //Hace XOR con el bit actual
            temp >>= 1; //Pasa al siguiente bit moviendo a la derecha
        }
        
        //Desplazar y añadir bit de realimentación al inicio
        //Mueve todos los bits a la derecha y mete el nuevo al principio
        state = (state >> 1) | (feedbackBit << (size - 1));
        state &= mask; //Aplica máscara para mantener tamaño - corta bits sobrantes
        
        return outputBit; //Devuelve el bit que sacamos al principio
    }
};

//==================== GEFFE ====================
//Generador de Geffe - combina 3 LFSRs para ser más seguro
//Es como tener 3 máquinas pequeñas trabajando juntas
//La fórmula hace que sea difícil adivinar el resultado
class Geffe {
private:
    LFSR lfsr0, lfsr1, lfsr2; //Tres LFSRs independientes - las 3 máquinas
    
    //Decodifica 9 bytes en parámetros del LFSR
    //Convierte bytes guardados en configuración para una máquina
    void decodeLFSRKey(const uint8_t* key, uint8_t& size, uint32_t& state, uint32_t& feedback) {
        //Primer byte: tamaño en bits (5 bits menos significativos)
        //0x1F = 00011111 en binario - toma solo los primeros 5 bits
        size = key[0] & 0x1F;
        
        //Siguiente 4 bytes: estado inicial (formato little-endian)
        //Little-endian significa: byte más pequeño primero
        //Tomamos 4 bytes y los juntamos en un número de 32 bits
        state = ((uint32_t)key[1]) |
                ((uint32_t)key[2] << 8) |  //Mueve 8 bits a la izquierda
                ((uint32_t)key[3] << 16) | //Mueve 16 bits a la izquierda
                ((uint32_t)key[4] << 24);  //Mueve 24 bits a la izquierda
        
        //Últimos 4 bytes: bits de realimentación
        //Mismo proceso para la máscara de realimentación
        feedback = ((uint32_t)key[5]) |
                   ((uint32_t)key[6] << 8) |
                   ((uint32_t)key[7] << 16) |
                   ((uint32_t)key[8] << 24);
    }

public:
    //Constructor: recibe clave de 27 bytes (9 bytes por LFSR)
    //27 bytes = 3 máquinas × 9 bytes cada una
    Geffe(const uint8_t* key) {
        uint8_t size0, size1, size2;        //Tamaños de las 3 máquinas
        uint32_t state0, state1, state2;    //Estados iniciales
        uint32_t feedback0, feedback1, feedback2; //Fórmulas de mezcla
        
        //Decodifica los tres grupos de 9 bytes
        //Cada grupo de 9 bytes configura una máquina diferente
        decodeLFSRKey(&key[0], size0, state0, feedback0);  //LFSR0 - primera máquina
        decodeLFSRKey(&key[9], size1, state1, feedback1);  //LFSR1 - segunda máquina
        decodeLFSRKey(&key[18], size2, state2, feedback2); //LFSR2 - tercera máquina
        
        //Inicializa los tres LFSRs con sus configuraciones
        //Ahora las 3 máquinas están listas para trabajar
        lfsr0.init(state0, feedback0, size0);
        lfsr1.init(state1, feedback1, size1);
        lfsr2.init(state2, feedback2, size2);
    }
    
    //Genera el siguiente bit: Zn = (X0 ∧ X1) ⊕ (¬X0 ∧ X2)
    //Fórmula del generador de Geffe que mezcla los 3 bits
    //∧ = AND (y), ¬ = NOT (no), ⊕ = XOR (o exclusivo)
    bool next() {
        bool x0 = lfsr0.next(); //Pide un bit a la máquina 1
        bool x1 = lfsr1.next(); //Pide un bit a la máquina 2
        bool x2 = lfsr2.next(); //Pide un bit a la máquina 3
        //Fórmula del generador de Geffe - mezcla inteligente
        return (x0 & x1) ^ (~x0 & x2);
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
    //Esta es la función más importante - ¡hace la magia del cifrado!
    //XOR tiene una propiedad mágica: si lo aplicas dos veces, vuelves al original
    //texto XOR secreto = cifrado, cifrado XOR mismo secreto = texto
    void processBuffer(uint8_t* buffer, size_t length) {
        for (size_t i = 0; i < length; i++) {
            //XOR entre byte del buffer y byte de la secuencia Geffe
            //Cada byte del archivo se mezcla con un byte "aleatorio"
            buffer[i] ^= nextByte();
        }
    }
};

#endif //GEFFE_GENERATOR_H - fin del archivo