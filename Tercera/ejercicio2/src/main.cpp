
#include "CBCMAC.h"
#include <Arduino.h>
/* Example usage: compute and save CBC-MAC (AES-CBC-MAC) of a file
 * Reads file from project data folder (relative path), computes MAC,
 * prints to serial and saves to a file.
 */

void setup(){
    Serial.begin(115200);
    // initialize pin 43 for blinking
    pinMode(43, OUTPUT);
    delay(1000);

    // 16-byte AES key (example)
    const uint8_t key[16] = {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10
    };

    CBCMAC cbc(key);
    // file path relative to project root (during development)
    const char* inpath = "data/Preguntas_Moodle_20210123.txt";
    const char* outpath = "mac_Preguntas_20210123.txt";

    auto mac = cbc.computeMAC(inpath);

    // Print MAC as hex to Serial
    char hex[33];
    for (int i = 0; i < 16; ++i) sprintf(hex + i*2, "%02x", mac[i]);
    hex[32] = '\0';
    Serial.printf("MAC(%s) = %s\n", inpath, hex);

    // Save MAC to outpath
    if (cbc.saveMACHex(mac, outpath)) {
        Serial.printf("Saved MAC to %s\n", outpath);
    } else {
        Serial.printf("Failed to save MAC to %s\n", outpath);
    }
}

void loop(){
    digitalWrite(43, LOW);
    delay(2000);
    digitalWrite(43, HIGH);
    delay(2000);
}