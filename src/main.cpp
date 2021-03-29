#include <Adafruit_BMP280.h>
#include <Adafruit_MCP23017.h>
#include <Arduino.h>
#include <CRC32.h>
#include <OneWire.h>
#include <SPI.h>
#include <Wire.h>

#include "barometric.h"
#include "eepromstore.h"
#include "measurement.h"
#include "pinout.h"
#include "settings.h"

Settings settings;
OneWire oneWire(GPIO_1WIRE);
Barometric barometric;
Adafruit_MCP23017 ioexpander;
EEPromStore eepromStore(EEPROM_PAGESPERCHIP /* Before settings are loaded assume that we have ONE chip */, ioexpander);

// Todo: replace with own main, there will be no loop, only startup->init->measure->xmit->deep sleep.
void setup() {
    Serial.begin(115200);
    Wire.begin();  // I2C
    SPI.begin();

    delay(1000);
    Serial.print("INIT");

    // 1. init clock, note if running and if there was a powerfail

    // 2. Next step is to initialize memory and read first block (settings) from EEPROM 0.
    // If that fails we need to go into a pure "setup-me" mode.

    ioexpander.begin();  // TODO: Might be possible to ignore this if we were running.
    ioexpander.digitalWrite(IOEXP_EEPROM0, HIGH);
    ioexpander.digitalWrite(IOEXP_EEPROM1, HIGH);
    ioexpander.digitalWrite(IOEXP_EEPROM2, HIGH);
    ioexpander.digitalWrite(IOEXP_EEPROM3, HIGH);
    ioexpander.digitalWrite(IOEXP_EEPROM4, HIGH);
    ioexpander.pinMode(IOEXP_EEPROM0, OUTPUT);
    ioexpander.pinMode(IOEXP_EEPROM1, OUTPUT);
    ioexpander.pinMode(IOEXP_EEPROM2, OUTPUT);
    ioexpander.pinMode(IOEXP_EEPROM3, OUTPUT);
    ioexpander.pinMode(IOEXP_EEPROM4, OUTPUT);

    uint8_t buf[EEPROM_PAGESIZE];
    eepromStore.readPage(buf, 0);  // Page 0 is settings
    if (!settings.setFromBuf(buf)) {
        Serial.println();
        Serial.println("Failed to load settings from EEPROM, using default settings.");
        settings.copyToBuf(buf);
        eepromStore.writePage(buf, 0);
    }

    // Delay startup by 5 seconds to allow user to start sending text via serial (TODO: Should only be done on powerup, but then wait much longer)
    for (int d = 0; d < 5; d++) {
        if (Serial.available()) break;
        delay(1000);
        Serial.print(".");
    }
    Serial.println();

    // 2.9 Allow settings to be changed.
    if (Serial.available()) {
        if (settings.configure()) {
            Serial.println("Writing settings to EEPROM");
            settings.copyToBuf(buf);
            eepromStore.writePage(buf, 0);
        }
    }

    eepromStore.updateMaxPages(settings.store.numeeprom * EEPROM_PAGESPERCHIP);

    // 3. Once we have settings loaded: IF clock is running but there was a powerfail, log that to eeprom

    //note, part of settings SRAM must be reserved for next measurement id / position in EEPROM (should be the same, but perhaps MODULUS total pagecount)
    // but this value should always be zeroed before storing to EEPROM (there needs to be an exhaustive way of scanning all blocks to find the last ID stored)
    // on the server-side it has to account for this when receiving IDs

    // 4. If clock is not running, start radio to run NTP to set it before doing any measurements. If NTP fails, sleep for a few minutes and try again.

    // 5. Startup sensors - If clock was running and no powerfail all sensors should have sane settings already.
    if (settings.store.bmpavail) {
        barometric.setup();
    }
    if(settings.store.dhtavail){
        // TODO: Init DHT
    }
    

    // Setup done.
    delay(5000);
    Serial.println("Booting");
}

void scanAndPrintOneWire(void);
void runRTCC(void);

void loop() {
    Serial.println("Trying to communicate with eeprom");
    uint8_t buf[EEPROM_PAGESIZE];
    eepromStore.readPage(buf, 0);
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

    Serial.println("#################################");
#if 0
    Measurement m;
    barometric.measure(m);
    Serial.print("Baro: ");
    Serial.println(m.baropress);
    m.genCrc();
    if (m.checkCrc()) {
        Serial.println("CRC Ok");
    }
#endif

#if 0
    // Check one-wire
    scanAndPrintOneWire();   
    runRTCC();
#endif
    delay(5000);
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
