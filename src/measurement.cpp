#include "measurement.h"
#include <CRC32.h>

Measurement::Measurement() {
    type = TYPE_SENSORREAD;
    id = 0xDEAD;  // Should call settings for next id.
    bits = 0;
    timestamp = 0;
    batteryvoltage = baropress = barotemp = humidity = humidtemp =
        tempsens0 = tempsens1 = tempsens2 = tempsens3 = tempsens4 = tempsens5 = tempsens6 = tempsens7 = NaN();
    crc = 0;
}

Measurement::~Measurement() {
}

float Measurement::NaN() {
    float res;
    uint32_t* fi = (uint32_t*)&res;
    *fi = 0xffffffff;
    return res;
}

void Measurement::genCrc() {
    uint32_t newCrc = CRC32::calculate((uint8_t*)this,sizeof(Measurement)-sizeof(crc));
    crc=newCrc;
    Serial.print("CRC Generated: ");
    Serial.println(crc, HEX);
}

bool Measurement::checkCrc() {
    uint32_t crcCheck = CRC32::calculate((uint8_t*)this,sizeof(Measurement)-sizeof(crc));
    Serial.print("CRC Check: ");
    Serial.println(crcCheck, HEX);
    return crc==crcCheck;
}
