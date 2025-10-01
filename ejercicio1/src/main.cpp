/* Ejercicio 1: Generador de números pseudoaleatorios con ESP32 */
#include "SPIFFS.h"
#include <cstdint>  // Para usar enteros de tamaño fijo como uint64_t

void listAllFiles();

//generador congruente  ---  xᵢ =(a*xᵢ₋₁ +c)  --  mod m
class MiRandom {
private:
    uint64_t multiplicador;   //a en la formula del genrador
    uint64_t incremento;      //c en la formula del genrador
    uint64_t modulo;          //m en la formula del genrador
    uint64_t estado;          //semilla

public:
    //se llama al crear un objeto MiRandom
    //recibe una semilla, si no hay una, es 1
    //c y m deben ser coprimos
    //a - 1 debe ser divisible entre todos los factores primos de m
    //a - 1 debe ser múltiplo de 2
    MiRandom(uint64_t semilla = 1) {
        multiplicador = 9;              // valor típico usado en LCG
        incremento = 1013904223;              // valor típico usado en LCG
        modulo = (1ULL << 32);                //2^32, desplazamos un bit 32 veces
        estado = semilla;                     // guardamos la semilla inicial
    }

    // Método para cambiar la semilla manualmente en cualquier momento
    void seed(uint64_t nuevaSemilla) {
        estado = nuevaSemilla;
    }

    // Genera un número pseudoaleatorio en el rango [min, max]
    double rand(double min, double max) {
        // Fórmula del generador congruencial lineal:
        // Xn+1 = (a * Xn + c) mod m
        estado = (multiplicador * estado + incremento) % modulo;

        // Normalizamos el resultado a un número entre 0 y 1
        double normalizado = (double)estado / (double)modulo;

        // Escalamos el valor al rango [min, max]
        return min + normalizado * (max - min);
    }
};

// Creamos un generador global con semilla 12345
MiRandom generador(12345);
int contadorNumeros = 0;
bool primeraRonda = true;

void setup(){
    Serial.begin(115200);
    pinMode(43, OUTPUT);
    digitalWrite(43, HIGH);
    
    if(!SPIFFS.begin(true)){
        Serial.println("Fail mounting FFat");
        return;
    }
    
    Serial.println("=== Generador de numeros pseudoaleatorios ===");
    Serial.println("Primera tanda de numeros:");
}

void loop(){
    // Generación de números aleatorios
    if (primeraRonda && contadorNumeros < 5) {
        double numero = generador.rand(0.0, 1.0);
        Serial.printf("Numero aleatorio %d: %f\n", contadorNumeros + 1, numero);
        contadorNumeros++;
    } else if (primeraRonda && contadorNumeros == 5) {
        // Reiniciamos el generador con otra semilla
        generador.seed(9999);
        Serial.println("\nSegunda tanda de numeros (nueva semilla):");
        primeraRonda = false;
        contadorNumeros = 0;
    } else if (!primeraRonda && contadorNumeros < 5) {
        double numero = generador.rand(0.0, 1.0);
        Serial.printf("Numero aleatorio %d: %f\n", contadorNumeros + 1, numero);
        contadorNumeros++;
        
        if (contadorNumeros == 5) {
            Serial.println("\n=== Generacion completa ===");
            listAllFiles();
        }
    }
}
