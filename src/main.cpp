#include <Adafruit_BMP280.h>
#include <Adafruit_MCP23017.h>
#include <Arduino.h>
#include <CRC32.h>
#include <OneWire.h>
#include <SPI.h>
#include <Wire.h>

#include "barometric.h"
#include "communication.h"
#include "eepromstore.h"
#include "measurement.h"
#include "pinout.h"
#include "rtcc.h"
#include "settings.h"
#include "tools.h"

OneWire oneWire(GPIO_1WIRE);
Adafruit_MCP23017 ioexpander;
Barometric barometric;

// Todo: replace with own main, there will be no loop, only startup->init->measure->xmit->deep sleep.
void setup() {
    bool clockWasRunning = false;
    bool powerUp = false;

    Serial.begin(115200);
    Wire.begin();  // I2C
    SPI.begin();

    delay(1000);
    Serial.print("INIT");

    // 1. init clock, note if running and if there was a powerfail
    Clock.begin();  // RTCC
    clockWasRunning = Clock.running;
    powerUp = Clock.powerfail!=0;

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
    eepromStore.readPage(buf, EEPROM_SETTINGS_PAGE);  
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
    if (Clock.running) Serial.println("Clock is running");

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
    if (Clock.powerfail) {
        Serial.print("Power failed:   ");
        Serial.println(Clock.powerfail);
        Serial.print("Power returned: ");
        Serial.println(Clock.powerreturn);
    }

    // 3.5 Now is also a great time to check if nextId == 0 -> we need to scan EEPROM storage to find last used id.

    // 4. If clock is not running, start radio to run NTP to set it before doing any measurements. If NTP fails, sleep for a few minutes and try again.
    Comms.begin();
    time_t ntpnow = Comms.getNtpTime();

    if (!Clock.running) {
        Clock.setTime(ntpnow);
    } else {
        time_t rtcNow = Clock.getTime();
        int delta = ntpnow - rtcNow;
        Serial.print("RTC NTP Delta: ");
        Serial.println(delta);
        Serial.print("RTC TIME: ");
        Serial.println(rtcNow);
        if (delta > 3600 || delta < -3600) Clock.setTime(ntpnow);
    }

    // 5. Startup sensors - If clock was running and no powerfail all sensors should have sane settings already.
    if (settings.store.bmpavail) {
        barometric.setup();
    }
    if (settings.store.dhtavail) {
        // TODO: Init DHT
    }

    // Setup done.
    delay(5000);
    Serial.println("Booting");
}

void scanAndPrintOneWire(void);
void runRTCC(void);

void loop() {
    Serial.print("#");
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
    delay(15000);
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
