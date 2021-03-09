#include <Arduino.h>

#include "pinout.h"

void setup() {
    Serial.begin(115200);
    pinMode(GPIO_1WIRE, OUTPUT);
    pinMode(GPIO_DHT, OUTPUT);
}

void loop() {
    digitalWrite(GPIO_1WIRE, LOW);
    digitalWrite(GPIO_DHT, HIGH);
    Serial.println("The BLUE led is off");
    delay(1000);
    digitalWrite(GPIO_1WIRE, HIGH);
    digitalWrite(GPIO_DHT, LOW);
    Serial.println("The led is on");
    delay(1000);
}
