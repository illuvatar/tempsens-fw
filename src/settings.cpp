#include "settings.h"

#include <CRC32.h>

// We should try to load settings from clock SRAM (64 bytes in total available, just enough for the settings struct)
Settings::Settings() {}
Settings::~Settings() {}

bool Settings::setFromBuf(uint8_t* buf) {
    return store.setFromBuf(buf);
}

void Settings::copyToBuf(uint8_t* buf) {
    store.genCrc();
    store.copyToBuf(buf);
}

bool Settings::configure(void) {
    bool setupDone = false;
    bool settingsChanged = false;
    uint8_t serialBuffer[128];
    size_t read = 0;
    uint8_t newValue;

    while (!setupDone) {
        while (!Serial.available()) {
            delay(10);
        }
        Serial.setTimeout(30000);
        read = Serial.readBytesUntil('\n', serialBuffer, 128);
        if (read > 0) {
            serialBuffer[read] = 0x00;
            if (serialBuffer[read - 1] == 0x0d) serialBuffer[--read] = 0x00;  // Trim any CR
            switch ((char)serialBuffer[0]) {
                case 'w':
                    setupDone = true;
                    break;
                case 'l':
                    Serial.println("Current settings:      ");
                    Serial.print("Barometer:               ");
                    if (store.bmpavail)
                        Serial.println("Available");
                    else
                        Serial.println("Not available");

                    Serial.print("Humidity:                ");
                    if (store.dhtavail)
                        Serial.println("Available");
                    else
                        Serial.println("Not available");

                    Serial.print("EEProms available:       ");
                    Serial.println(store.numeeprom);

                    Serial.print("Tempsensors detected:    ");
                    Serial.println(store.numtempsens);

                    Serial.print("WIFI Credentials:        ");
                    Serial.println(store.numtempsens);

                    break;
                case 's':
                    if (read > 2) {
                        switch ((char)serialBuffer[1]) {
                            case 'h':
                                newValue = ((char)serialBuffer[2] == '1');
                                if (newValue != store.dhtavail) {
                                    store.dhtavail = newValue;
                                    settingsChanged = true;
                                }
                                break;
                            case 'b':
                                newValue = ((char)serialBuffer[2] == '1');
                                if (newValue != store.bmpavail) {
                                    store.bmpavail = newValue;
                                    settingsChanged = true;
                                }
                                break;
                            case 'e':
                                newValue = store.numeeprom;
                                switch ((char)serialBuffer[2]) {
                                    case '1':
                                        newValue = 1;
                                        break;
                                    case '2':
                                        newValue = 2;
                                        break;
                                    case '3':
                                        newValue = 3;
                                        break;
                                    case '4':
                                        newValue = 4;
                                        break;
                                    case '5':
                                        newValue = 5;
                                        break;
                                    default:
                                        Serial.println("Invalid number of eeproms.");
                                }
                                if (newValue != store.numeeprom) {
                                    store.numeeprom = newValue;
                                    settingsChanged = true;
                                }
                                break;

                            default:
                                Serial.println("Unknown setting.");
                        }
                    }
                    break;
                default:
                    Serial.println("Unknown option.");
            }
        } else {
            Serial.println("Didn't read anything.");
        }

        delay(100);
    }

    return settingsChanged;
}

SettingsStorage::SettingsStorage() {
    numeeprom = 1;
}

SettingsStorage::~SettingsStorage() {}

void SettingsStorage::genCrc(void) {
    uint32_t newCrc = CRC32::calculate((uint8_t*)this, sizeof(SettingsStorage) - sizeof(crc));
    crc = newCrc;
    Serial.print("CRC Generated: ");
    Serial.println(crc, HEX);
}

bool checkCrcBuf(uint8_t* buf, uint32_t& crc) {
    uint32_t crcCheck = CRC32::calculate(buf, sizeof(SettingsStorage) - sizeof(crc));
    Serial.print("CRC Check: ");
    Serial.println(crcCheck, HEX);
    return crc == crcCheck;
}

bool SettingsStorage::checkCrc(void) {
    return checkCrcBuf((uint8_t*)this, crc);
}

bool SettingsStorage::setFromBuf(uint8_t* buf) {
    uint32_t existingCrc = 0;
    memcpy((uint8_t*)&existingCrc, &buf[sizeof(SettingsStorage) - sizeof(existingCrc)], sizeof(existingCrc));
    if (!checkCrcBuf(buf, existingCrc)) return false;
    memcpy((uint8_t*)this, buf, sizeof(SettingsStorage));
    return true;
}

void SettingsStorage::copyToBuf(uint8_t* buf) {
    memcpy(buf, (uint8_t*)this, sizeof(SettingsStorage));
}
