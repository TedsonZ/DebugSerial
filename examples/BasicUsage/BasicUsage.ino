#include <DebugSerial.h>

void setup() {
    Serial.begin(115200);
    initializeDebugSerial();
}

void loop() {
    dlog("Mensagem de teste");
    dlog(123);
    dlog(3.14f);
    delay(1000);
}
