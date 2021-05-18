#include "rtcc.h"

#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#include "pinout.h"
#include "tools.h"
#include "eepromstore.h"


#define RTCCADDR 0x6f

RTCC::RTCC() {
    static_assert(sizeof(RTCCmem) == EEPROM_PAGESIZE, "RTCCmem has wrong size.");
    powerfail = 0;
    powerreturn = 0;
}

RTCC::~RTCC() {}

void RTCC::begin(void) {
    // Check if OSCILLATOR running and read SRAM.

    uint8_t clockbuf[8];
    int received = 0;
    Wire.beginTransmission(RTCCADDR);
    Wire.write((uint8_t)0x03);
    Wire.endTransmission();
    Wire.requestFrom(RTCCADDR, 1);
    received = 0;
    if (Wire.available()) received = Wire.readBytes((&clockbuf[received]), 1);
    if (clockbuf[0] & 0x20) {
        running = 1;
    }
    if (running && (clockbuf[0] & 0x10)) {
        uint8_t existingWday = clockbuf[0] & 0x2f;
        // We had an powerfail with clock running, read registers and clear flag
        time_t now = getTime();
        tm* timeinfo = gmtime(&now);
        timeinfo->tm_sec = 0;
        int nowMonth = timeinfo->tm_mon;
        int nowYear = timeinfo->tm_year;

        Wire.beginTransmission(RTCCADDR);
        Wire.write((uint8_t)0x18);
        Wire.endTransmission();
        Wire.requestFrom(RTCCADDR, 8);
        received = 0;
        while (Wire.available()) {
            received += Wire.readBytes(&(clockbuf[received]), (sizeof(clockbuf) - received));
        }
        // Power down
        timeinfo->tm_min = (clockbuf[0] & 0x0f) + 10 * ((clockbuf[0] & 0x70) >> 4);
        timeinfo->tm_hour = (clockbuf[1] & 0x0f) + 10 * ((clockbuf[1] & 0x30) >> 4);
        timeinfo->tm_mday = (clockbuf[2] & 0x0f) + 10 * ((clockbuf[2] & 0x30) >> 4);
        timeinfo->tm_mon = ((clockbuf[3] & 0x0f) + 10 * ((clockbuf[3] & 0x10) >> 4)) - 1;
        if (nowMonth < timeinfo->tm_mon) timeinfo->tm_year--;
        powerfail = mktime(timeinfo);

        // Power up
        timeinfo->tm_year = nowYear;
        timeinfo->tm_min = (clockbuf[4] & 0x0f) + 10 * ((clockbuf[4] & 0x70) >> 4);
        timeinfo->tm_hour = (clockbuf[5] & 0x0f) + 10 * ((clockbuf[5] & 0x30) >> 4);
        timeinfo->tm_mday = (clockbuf[6] & 0x0f) + 10 * ((clockbuf[6] & 0x30) >> 4);
        timeinfo->tm_mon = ((clockbuf[7] & 0x0f) + 10 * ((clockbuf[7] & 0x10) >> 4)) - 1;
        if (nowMonth < timeinfo->tm_mon) timeinfo->tm_year--;
        powerreturn = mktime(timeinfo);

        // Clear powerfail flag
        Wire.beginTransmission(RTCCADDR);
        Wire.write(0x03);
        Wire.write(existingWday);
        Wire.endTransmission();
    }

    if (!loadStore()) {
        saveStore();
    }
}

void RTCC::setTime(time_t newTime) {
    tm* timeinfo = gmtime(&newTime);
    if(timeinfo->tm_year<100){
        Serial.println("Refusing to update clock with bad time.");
        return;
    }
    Serial.print("Asked to set time to: ");
    Serial.print(timeinfo->tm_year);
    Serial.print("-");
    Serial.print(timeinfo->tm_mon);
    Serial.print("-");
    Serial.print(timeinfo->tm_mday);
    Serial.print(" ");

    Serial.print(timeinfo->tm_hour);
    Serial.print(":");
    Serial.print(timeinfo->tm_min);
    Serial.print(":");
    Serial.print(timeinfo->tm_sec);
    Serial.println();

    uint8_t clockbuf[9];
    clockbuf[0] = ((timeinfo->tm_sec / 10) << 4) + (timeinfo->tm_sec % 10);
    clockbuf[1] = ((timeinfo->tm_min / 10) << 4) + (timeinfo->tm_min % 10);
    clockbuf[2] = ((timeinfo->tm_hour / 10) << 4) + (timeinfo->tm_hour % 10);
    clockbuf[3] = 0x08 + timeinfo->tm_wday + 1;
    clockbuf[4] = ((timeinfo->tm_mday / 10) << 4) + (timeinfo->tm_mday % 10);
    clockbuf[5] = (((timeinfo->tm_mon + 1) / 10) << 4) + ((timeinfo->tm_mon + 1) % 10);
    clockbuf[6] = (((timeinfo->tm_year % 100) / 10) << 4) + (timeinfo->tm_year % 10);
    clockbuf[7] = 0x00;
    clockbuf[8] = 0x00;

    // Stop clock
    running = 0;
    Wire.beginTransmission(RTCCADDR);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();

    // Set time
    Wire.beginTransmission(RTCCADDR);
    Wire.write(0x00);
    for (uint32_t i = 0; i < sizeof(clockbuf); i++) {
        Wire.write(clockbuf[i]);
    }
    Wire.endTransmission();

    // Start clock
    Wire.beginTransmission(RTCCADDR);
    Wire.write(0x00);
    Wire.write(0x80 + clockbuf[0]);
    Wire.endTransmission();

    // Start checking for OSCRUN before returning.
    int tries = 100;
    uint8_t buf[1];
    int received = 0;
    while (!running && tries-- > 0) {
        Wire.beginTransmission(RTCCADDR);
        Wire.write((uint8_t)0x03);
        Wire.endTransmission();
        Wire.requestFrom(RTCCADDR, 1);
        received = 0;
        if (Wire.available()) received = Wire.readBytes((&buf[received]), 1);
        if (buf[0] & 0x20) {
            running = 1;
        } else {
            delay(100);
            Serial.println("Waiting on oscillator");
        }
    }
}

time_t RTCC::getTime(void) {
    struct tm timeinfo;
    uint8_t clockbuf[9];

    // Get time
    Wire.beginTransmission(RTCCADDR);
    Wire.write(uint8_t(0x00));
    Wire.endTransmission();
    Wire.requestFrom(RTCCADDR, sizeof(clockbuf));
    int received = 0;
    while (Wire.available()) {
        received += Wire.readBytes(&(clockbuf[received]), (sizeof(clockbuf) - received));
    }

    // Build timeinfo
    timeinfo.tm_sec = (clockbuf[0] & 0x0f) + 10 * ((clockbuf[0] & 0x70) >> 4);
    timeinfo.tm_min = (clockbuf[1] & 0x0f) + 10 * ((clockbuf[1] & 0x70) >> 4);
    timeinfo.tm_hour = (clockbuf[2] & 0x0f) + 10 * ((clockbuf[2] & 0x30) >> 4);
    timeinfo.tm_wday = (clockbuf[3] & 0x07) - 1;
    timeinfo.tm_mday = (clockbuf[4] & 0x0f) + 10 * ((clockbuf[4] & 0x30) >> 4);
    timeinfo.tm_mon = ((clockbuf[5] & 0x0f) + 10 * ((clockbuf[5] & 0x10) >> 4)) - 1;
    timeinfo.tm_year = (clockbuf[6] & 0x0f) + 10 * ((clockbuf[6] & 0xf0) >> 4) + 100;

    return mktime(&timeinfo);
}

bool RTCC::loadStore(void) {
    uint8_t buf[64];
    size_t received;
    Wire.beginTransmission(RTCCADDR);
    Wire.write(uint8_t(0x20));
    Wire.endTransmission();
    Wire.requestFrom(RTCCADDR, sizeof(buf));
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

RTCC Clock;
