/* Plantilla proyectos arduino*/
#include "SPIFFS.h"
void listAllFiles();

void setup(){
    Serial.begin(115200);
    pinMode(43, OUTPUT);
    digitalWrite(43, HIGH);
    if(!SPIFFS.begin(true)){
        Serial.println("Fail mounting FFat");
        return;
    }
}

void loop(){
    Serial.printf("Hola mundo\n");
    digitalWrite(43, LOW);
    delay(2000);
    digitalWrite(43, HIGH);
    delay(2000);
    listAllFiles();
}

void listAllFiles(){
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  char s[500]="";
  while(file){
      snprintf(s,500,"%sFILE %s: %d\n", s, file.name(), file.size());
      file = root.openNextFile();
  }
  Serial.printf("%s",s);
}
