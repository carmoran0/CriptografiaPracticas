
// Implementación de un cifrador OFB personalizado para bloques de 8 bits
#include <Arduino.h>
#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct PermEntry {
    uint8_t destino;
    std::array<int8_t, 4> vi;
    uint8_t totalVI;
    uint8_t keyIndex;
};

constexpr std::array<PermEntry, 16> kPermutacion = {{
    {15, {3, 6, -1, -1}, 2, 4},
    {14, {13, 10, -1, -1}, 2, 2},
    {13, {9, 1, -1, -1}, 2, 13},
    {12, {10, 14, -1, -1}, 2, 7},
    {11, {12, 5, -1, -1}, 2, 15},
    {10, {0, 2, -1, -1}, 2, 1},
    {9, {3, 6, -1, -1}, 2, 14},
    {8, {0, 13, -1, -1}, 2, 6},
    {7, {8, 5, -1, -1}, 2, 12},
    {6, {3, 10, -1, -1}, 2, 0},
    {5, {2, 9, -1, -1}, 2, 3},
    {4, {3, 11, -1, -1}, 2, 11},
    {3, {1, 14, -1, -1}, 2, 5},
    {2, {7, 15, -1, -1}, 2, 10},
    {1, {2, 5, -1, -1}, 2, 8},
    {0, {0, 4, 10, -1}, 3, 9},
}};

uint8_t hexToNibble(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<uint8_t>(10 + c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<uint8_t>(10 + c - 'A');
    }
    return 0;
}

std::array<uint8_t, 16> hexStringToArray(const char *hex) {
    std::array<uint8_t, 16> resultado{};
    size_t longitud = strlen(hex);
    if (longitud < 32) {
        return resultado;
    }
    for (size_t i = 0; i < resultado.size(); ++i) {
        resultado[i] = static_cast<uint8_t>((hexToNibble(hex[2 * i]) << 4) |
                                            hexToNibble(hex[2 * i + 1]));
    }
    return resultado;
}

void imprimirBuffer(const char *etiqueta, const std::vector<uint8_t> &datos) {
    Serial.printf("%s", etiqueta);
    for (uint8_t valor : datos) {
        Serial.printf("%02X ", valor);
    }
    Serial.println();
}

} // namespace

class cifradorOFB {
public:
    cifradorOFB(const std::array<uint8_t, 16> &clave,
                const std::array<uint8_t, 16> &vi)
        : clave_(clave), estadoVI_(vi) {
        cursor_ = keystream_.size();
    }

    void reiniciarVI(const std::array<uint8_t, 16> &vi) {
        estadoVI_ = vi;
        cursor_ = keystream_.size();
    }

    uint8_t procesarByte(uint8_t dato) {
        if (cursor_ >= keystream_.size()) {
            generarSiguienteBloque();
        }
        uint8_t salida = dato ^ keystream_[cursor_];
        ++cursor_;
        return salida;
    }

    std::vector<uint8_t> procesar(const std::vector<uint8_t> &datos) {
        std::vector<uint8_t> resultado;
        resultado.reserve(datos.size());
        for (uint8_t byte : datos) {
            resultado.push_back(procesarByte(byte));
        }
        return resultado;
    }

private:
    std::array<uint8_t, 16> clave_{};
    std::array<uint8_t, 16> estadoVI_{};
    std::array<uint8_t, 16> keystream_{};
    size_t cursor_ = 0;

    void generarSiguienteBloque() {
        std::array<uint8_t, 16> nuevo{};
        for (const auto &entrada : kPermutacion) {
            uint8_t acumulado = 0;
            for (uint8_t i = 0; i < entrada.totalVI; ++i) {
                int8_t indice = entrada.vi[i];
                acumulado ^= estadoVI_[indice];
            }
            acumulado ^= clave_[entrada.keyIndex];
            nuevo[entrada.destino] = static_cast<uint8_t>(~acumulado);
        }
        estadoVI_ = nuevo;
        keystream_ = nuevo;
        cursor_ = 0;
    }
};

void setup() {
    Serial.begin(115200);
    const unsigned long esperaMaxima = millis() + 2000;
    while (!Serial && millis() < esperaMaxima) {
        delay(10);
    }
    pinMode(43, OUTPUT);
    digitalWrite(43, HIGH);

    const auto clave =
        hexStringToArray("00112233445566778899AABBCCDDEEFF");
    const auto vi = hexStringToArray("0F1E2D3C4B5A69788796A5B4C3D2E1F0");

    cifradorOFB ofb(clave, vi);

    const char *mensaje = "Criptografia OFB";
    std::vector<uint8_t> plano(mensaje, mensaje + strlen(mensaje));

    auto cifrado = ofb.procesar(plano);

    ofb.reiniciarVI(vi);
    auto descifrado = ofb.procesar(cifrado);
    std::string textoDescifrado(descifrado.begin(), descifrado.end());

    Serial.println("=== OFB_Cripto ===");
    Serial.printf("Mensaje original: %s\n", mensaje);
    imprimirBuffer("Cifrado (hex): ", cifrado);
    Serial.printf("Descifrado: %s\n", textoDescifrado.c_str());
}

void loop() {
    delay(1000);
}