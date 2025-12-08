#include <Arduino.h>
#include <SPIFFS.h>

#include <string>

#include "MD5.h"

namespace {
constexpr const char* kSampleFile = "/md5_sample.txt";
constexpr const char* kExpectedTexto = "5df9f63916ebf8528697b629022993e8";
constexpr const char* kExpectedVacio = "d41d8cd98f00b204e9800998ecf8427e";

bool ensureFileWithContent(const char* path, const std::string& content) {
    if (SPIFFS.exists(path)) {
        return true;
    }

    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const uint8_t*>(content.data()), content.size());
    file.close();
    return true;
}

void logDigest(const char* label, const std::string& digest, const char* expected = nullptr) {
    if (expected == nullptr) {
        Serial.printf("%s = %s\n", label, digest.c_str());
        return;
    }

    const bool ok = digest == expected;
    Serial.printf("%s = %s [%s]\n", label, digest.c_str(), ok ? "OK" : "ERROR");
}
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
        delay(10);
    }

    if (!SPIFFS.begin(true)) {
        Serial.println("Error al montar SPIFFS");
        return;
    }

    const std::string texto = "Generando un MD5 de un texto";
    const std::string digestTexto = MD5::hash(texto);
    logDigest("MD5(\"Generando un MD5 de un texto\")", digestTexto, kExpectedTexto);

    const std::string digestVacio = MD5::hash(std::string());
    logDigest("MD5(\"\")", digestVacio, kExpectedVacio);

    if (!ensureFileWithContent(kSampleFile, texto)) {
        Serial.printf("No se pudo preparar el fichero %s\n", kSampleFile);
        return;
    }

    const std::string digestFichero = MD5::hashFile(SPIFFS, kSampleFile);
    logDigest("MD5 fichero de ejemplo", digestFichero, kExpectedTexto);
}

void loop() {
    delay(1000);
}