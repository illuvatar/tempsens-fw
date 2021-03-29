#include "tools.h"

#include <Arduino.h>
#include <CRC32.h>

void serialDumpPage(uint8_t* buf) {
    Serial.println("PAGEDUMP");
    for (int r = 0; r < 8; r++) {
        Serial.print("r");
        Serial.print(r);
        Serial.print(": ");
        for (int c = 0; c < 8; c++) {
            if (buf[(r * 8) + c] < 0x10) Serial.print("0");
            Serial.print(buf[(r * 8) + c], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}

bool checkCrcBuf(uint8_t* buf, size_t bufSize) {
    uint32_t existingCrc = 0;
    memcpy((uint8_t*)&existingCrc, &buf[bufSize - sizeof(existingCrc)], sizeof(existingCrc));

    uint32_t crcCheck = CRC32::calculate(buf, bufSize - sizeof(existingCrc));
    return existingCrc == crcCheck;
}

bool updateCrcBuf(uint8_t* buf, size_t bufSize) {
    uint32_t newCrc = 0;
    newCrc = CRC32::calculate(buf, bufSize - sizeof(newCrc));
    memcpy(&buf[bufSize - sizeof(newCrc)], (uint8_t*)&newCrc, sizeof(newCrc));
}
