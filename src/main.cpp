#include <Adafruit_BMP280.h>
#include <Arduino.h>
#include <CRC32.h>
#include <OneWire.h>
#include <SPI.h>
#include <Wire.h>

#include "measurement.h"
#include "pinout.h"
#include "barometric.h"

OneWire oneWire(GPIO_1WIRE);
Barometric barometric;

void setup() {
    Serial.begin(115200);
    Wire.begin();  // I2C
    barometric.setup();
}

void scanAndPrintOneWire(void);
void runRTCC(void);

void loop() {
    Measurement m;
    barometric.measure(m);
    Serial.print("Baro: "); 
    Serial.println(m.baropress);
    m.genCrc();
    if(m.checkCrc()){
        Serial.println("CRC Ok");
    }

#if 0
    // Check one-wire
    scanAndPrintOneWire();   
    runRTCC();
#endif
    delay(2000);
}

void scanAndPrintOneWire(void) {
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius;

    if (!oneWire.search(addr)) {
        oneWire.reset_search();
        delay(250);
        return;
    }

    Serial.print("1Wire :");
    for (i = 0; i < 8; i++) {
        Serial.write(' ');
        Serial.print(addr[i], HEX);
    }

    if (OneWire::crc8(addr, 7) != addr[7]) {
        Serial.println("CRC is not valid!");
        return;
    }
    // the first ROM byte indicates which chip
    switch (addr[0]) {
        case 0x10:
            Serial.print("  DS18S20");  // or old DS1820
            type_s = 1;
            break;
        case 0x28:
            Serial.print("  DS18B20");
            type_s = 0;
            break;
        case 0x22:
            Serial.print("  DS1822 ");
            type_s = 0;
            break;
        default:
            Serial.println(" <- Device is not a DS18x20 family device.");
            return;
    }

    oneWire.reset();
    oneWire.select(addr);
    oneWire.write(0x44, 1);  // start conversion, with parasite power on at the end

    delay(1000);  // maybe 750ms is enough, maybe not
    // we might do a oneWire.depower() here, but the reset will take care of it.

    present = oneWire.reset();
    oneWire.select(addr);
    oneWire.write(0xBE);  // Read Scratchpad
    // TODO: if sensor is of b type (address starts with 2x) then make sure it is set for high res
    Serial.print(" D(");
    if (present) {
        for (i = 0; i < 9; i++) {  // we need 9 bytes
            data[i] = oneWire.read();
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.print(") C(");
        Serial.print(OneWire::crc8(data, 8), HEX);
        Serial.print(")");

        // Convert the data to actual temperature
        // because the result is a 16 bit signed integer, it should
        // be stored to an "int16_t" type, which is always 16 bits
        // even when compiled on a 32 bit processor.
        int16_t raw = (data[1] << 8) | data[0];
        if (type_s) {
            raw = raw << 3;  // 9 bit resolution default
            if (data[7] == 0x10) {
                // "count remain" gives full 12 bit resolution
                raw = (raw & 0xFFF0) + 12 - data[6];
            }
        } else {
            byte cfg = (data[4] & 0x60);
            // at lower res, the low bits are undefined, so let's zero them
            if (cfg == 0x00)
                raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20)
                raw = raw & ~3;  // 10 bit res, 187.5 ms
            else if (cfg == 0x40)
                raw = raw & ~1;  // 11 bit res, 375 ms
                                 //// default is 12 bit resolution, 750 ms conversion time
        }
        celsius = (float)raw / 16.0;
        Serial.print("  T = ");
        Serial.print(celsius);
        Serial.println(" C");
    } else {
        Serial.println(" Not present.");
    }
}

const char *days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void printDayOfWeek(Stream &s, uint8_t wday) {
    if (wday >= 0 && wday < 7) {
        s.print(days[wday]);
    } else
        s.print("ERROR");
}

#define RTCBYTES 0x09
void runRTCC(void) {
    uint8_t clockBuf[RTCBYTES];
    Wire.beginTransmission(0x6f);
    Wire.write(uint8_t(0));
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
