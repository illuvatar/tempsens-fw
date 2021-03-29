#include "rtcc.h"

#include <Arduino.h>
#include <Wire.h>

#include "pinout.h"
#include "tools.h"

#define RTCCADDR 0x6f

RTCC::RTCC() {}
RTCC::~RTCC() {}

void RTCC::setup(void) {
    // Check if OSCILLATOR running and read SRAM.

    uint8_t buf[1];
    int received = 0;
    Wire.beginTransmission(RTCCADDR);
    Wire.write((uint8_t)0x03);
    Wire.endTransmission();
    Wire.requestFrom(0x6f, 1);
    received = 0;
    if (Wire.available()) received = Wire.readBytes((&buf[received]), 1);
    if (buf[0] & 0x20) {
        running = 1;
    }

    if (!loadStore()) {
        saveStore();
    }
}

#if 0
#define RTCBYTES 0x09
void runRTCC(void) {
    Wire.beginTransmission(RTCCADDR);
    Wire.write(uint8_t(0x03));

    Wire.endTransmission();

    // Try reading a byte
    Wire.requestFrom(0x6f, (uint8_t)RTCBYTES);
    int received = 0;
    while (Wire.available()) {
        received += Wire.readBytes(&(clockBuf[received]), (RTCBYTES - received));
        Serial.print(".");
    }
    Serial.print(" Total ");
    Serial.print(received, HEX);
    if (received == RTCBYTES) {
        Serial.print(" - Data: ");
        for (int i = 0; i < RTCBYTES; i++) {
            Serial.print(clockBuf[i], HEX);
            Serial.print(" ");
        }
        if (!(clockBuf[0] & 0x80)) {
            Wire.beginTransmission(0x6f);
            Wire.write(0x00);
            Wire.write(0x81);
            Wire.endTransmission();
            Wire.beginTransmission(0x6f);
            Wire.write(0x01);
            Wire.write(0x02);
            Wire.endTransmission();
            Wire.beginTransmission(0x6f);
            Wire.write(0x02);
            Wire.write(0x03);
            Wire.endTransmission();
            Wire.beginTransmission(0x6f);
            Wire.write(0x03);
            Wire.write(0x0c);
            Wire.endTransmission();
            Wire.beginTransmission(0x6f);
            Wire.write(0x04);
            Wire.write(0x22);
            Wire.endTransmission();
            Wire.beginTransmission(0x6f);
            Wire.write(0x05);
            Wire.write(0x03);
            Wire.endTransmission();
            Wire.beginTransmission(0x6f);
            Wire.write(0x06);
            Wire.write(0x21);
            Wire.endTransmission();
            Wire.beginTransmission(0x6f);
            Wire.write(0x07);
            Wire.write(0xc0);
            Wire.endTransmission();
            Serial.print("  - Enabled Oscillator");
        } else {
            /*
            Wire.beginTransmission(0x6f);
            Wire.write(0x00);
            Wire.write(0x02);
            Wire.endTransmission();
            */
        }
        if (clockBuf[3] & 0x10) {
            Wire.beginTransmission(0x6f);
            Wire.write(0x03);
            Wire.write(clockBuf[3] & ~0x10);
            Wire.endTransmission();
            Serial.print("   - Reset powerfail");
        }
        if (clockBuf[3] & 0x20) {
            Serial.print("   - Clock running");
            digitalWrite(GPIO_DHT, LOW);
        }
    }
    Serial.println();
}
#endif

void RTCC::setTime(uint32_t newTime) {}

uint32_t RTCC::getTime(void) { return 0x00000000; }

bool RTCC::loadStore(void) {
    uint8_t buf[64];
    size_t received;
    Wire.beginTransmission(RTCCADDR);
    Wire.write(uint8_t(0x20));
    Wire.endTransmission();
    Wire.requestFrom(0x6f, sizeof(buf));
    received = 0;
    while (Wire.available()) received += Wire.readBytes((&buf[received]), sizeof(buf) - received);
    return store.setFromBuf(buf);
}

void RTCC::saveStore(void) {
    uint8_t buf[64];
    store.copyToBuf(buf);
    updateCrcBuf(buf, sizeof(buf));
    Wire.beginTransmission(RTCCADDR);
    Wire.write(uint8_t(0x20));
    Wire.write(buf, sizeof(buf));
    Wire.endTransmission();
}

RTCCmem::RTCCmem() {}

RTCCmem::~RTCCmem() {}

bool RTCCmem::setFromBuf(uint8_t* buf) {
    if (!checkCrcBuf(buf, sizeof(RTCCmem))) return false;
    memcpy((uint8_t*)this, buf, sizeof(RTCCmem));
    return true;
}

void RTCCmem::copyToBuf(uint8_t* buf) {
    memcpy(buf, (uint8_t*)this, sizeof(RTCCmem));
}
