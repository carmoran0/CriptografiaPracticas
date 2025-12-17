/* Ejercicio 1: Generador de números pseudoaleatorios con ESP32 */
#include "SPIFFS.h"
#include <cstdint>

class MiRandom {
private:
    uint64_t multiplicador;
    uint64_t incremento;
    uint64_t modulo;
    uint64_t estado;

public:
    MiRandom(uint64_t semilla = 1) {
        multiplicador = 1103515245;
        incremento = 12345;  // Cambiado a valor estándar
        modulo = (1ULL << 31);
        estado = semilla;
    }

    void seed(uint64_t nuevaSemilla) {
        estado = nuevaSemilla;
    }

    double rand(double min, double max) {
        estado = (multiplicador * estado + incremento) % modulo;
        double normalizado = (double)estado / (double)modulo;
        return min + normalizado * (max - min);
    }
};

//Creamos un generador global
MiRandom generador(903699);
int contadorNumeros = 0;
bool primeraRonda = true;
bool terminado = false;  // Nueva variable para control

void setup(){
    Serial.begin(115200);
    pinMode(43, OUTPUT);
    digitalWrite(43, HIGH);
    
    if(!SPIFFS.begin(true)){
        Serial.println("Error montando SPIFFS");
        return;
    }
    
    delay(2000);  // Espera para que el puerto serie se estabilice
    
    Serial.println("=== Generador de numeros pseudoaleatorios ===");
    Serial.println("Primera tanda de numeros (semilla 903699):");
}

void loop(){
    if (terminado) {
        return;  // No hacer nada si ya terminó
    }
    
    if (primeraRonda && contadorNumeros < 5) {
        double numero = generador.rand(0.0, 1.0);
        Serial.printf("Numero aleatorio %d: %.6f\n", contadorNumeros + 1, numero);
        contadorNumeros++;
        delay(1000);  // Espera 1 segundo entre números
    } 
    else if (primeraRonda && contadorNumeros == 5) {
        generador.seed(3102025);
        Serial.println("\nSegunda tanda de numeros (con semilla 3102025):");
        primeraRonda = false;
        contadorNumeros = 0;
        delay(2000);  // Espera antes de la segunda tanda
    } 
    else if (!primeraRonda && contadorNumeros < 5) {
        double numero = generador.rand(0.0, 1.0);
        Serial.printf("Numero aleatorio %d: %.6f\n", contadorNumeros + 1, numero);
        contadorNumeros++;
        delay(1000);  // Espera 1 segundo entre números
        
        if (contadorNumeros == 5) {
            Serial.println("\n=== Generacion completa ===");
            terminado = true;  // Marca como terminado
        }
    }
}