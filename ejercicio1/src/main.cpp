#include <cstdio>   // Para usar printf (salida de texto)
#include <cstdint>  // Para usar enteros de tamaño fijo como uint64_t

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

int main() {
    // Creamos un generador con semilla 12345
    MiRandom generador(12345);

    printf("Primera tanda de numeros:\n");
    // Generamos 5 números aleatorios entre 0 y 1
    for (int i = 0; i < 5; i++) {
        double numero = generador.rand(0.0, 1.0);
        printf("Numero aleatorio %d: %f\n", i + 1, numero);
    }

    // Reiniciamos el generador con otra semilla
    generador.seed(9999);

    printf("\nSegunda tanda de numeros (nueva semilla):\n");
    // Generamos otros 5 números con la nueva semilla
    for (int i = 0; i < 5; i++) {
        double numero = generador.rand(0.0, 1.0);
        printf("Numero aleatorio %d: %f\n", i + 1, numero);
    }

    return 0; // Fin del programa
}
