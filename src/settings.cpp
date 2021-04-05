#include "settings.h"

#include <CRC32.h>

#include "eepromstore.h"
#include "tools.h"

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
    uint8_t serialBuffer[129];
    uint8_t ssidBuf[65];
    uint8_t pskBuf[65];
    size_t read = 0;
    uint8_t newValue = 0;
    uint8_t wifiNum = 0;
    size_t len = 0;

    while (!setupDone) {
        while (!Serial.available()) {
            delay(10);
        }
        Serial.setTimeout(30000);
        read = Serial.readBytesUntil('\n', serialBuffer, 128);
        if (read > 0) {
            memset(ssidBuf, 0, sizeof(ssidBuf));
            memset(pskBuf, 0, sizeof(pskBuf));
            serialBuffer[read] = 0x00;
            if (serialBuffer[read - 1] == 0x0d) serialBuffer[--read] = 0x00;  // Trim any CR
            Serial.print("Read: ");
            Serial.println(read);

            switch ((char)serialBuffer[0]) {
                case 'w':
                    setupDone = true;
                    break;
                case 'l':
                    Serial.println("Current settings:      ");
                    Serial.print("UUID:                    ");
                    Serial.println(store.uuid);
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
                    Serial.println(store.numwificreds);
                    for (int c = 0; c < store.numwificreds; c++) {
                        eepromStore.readPage(ssidBuf, EEPROM_FIRST_WIFIPAGE + 2 * c);
                        if (ssidBuf[0] == 0xff) continue;  // Invalid setting.
                        eepromStore.readPage(pskBuf, EEPROM_FIRST_WIFIPAGE + 2 * c + 1);
                        if (pskBuf[0] == 0xff) continue;  // Invalid setting.
                        Serial.print("    ");
                        if (store.numwificreds < 10) Serial.print(" ");
                        Serial.print(store.numwificreds);
                        Serial.print(" SSID '");
                        Serial.print((char*)ssidBuf);
                        Serial.println("'");
                        Serial.print("       PSK  '");
                        Serial.print((char*)pskBuf);
                        Serial.println("'");
                    }

                    break;
                case 's':
                    if (read > 2) {
                        switch ((char)serialBuffer[1]) {
                            case 'h':  // DHT Humidity sensor available
                                newValue = ((char)serialBuffer[2] == '1');
                                if (newValue != store.dhtavail) {
                                    store.dhtavail = newValue;
                                    settingsChanged = true;
                                }
                                break;
                            case 'b':  // BMP Pressure sensor available
                                newValue = ((char)serialBuffer[2] == '1');
                                if (newValue != store.bmpavail) {
                                    store.bmpavail = newValue;
                                    settingsChanged = true;
                                }
                                break;
                            case 'e':  // Number of eeproms
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
                            case 'w':  // WIFI
                                if (read < 4) continue;
                                wifiNum = serialBuffer[2] - '0';
                                Serial.print(" WIFINUM ");
                                Serial.println(wifiNum);

                                if (wifiNum > (store.numwificreds + 1)) {
                                    Serial.println("Store WIFI as next available");
                                    continue;
                                }
                                if (wifiNum == store.numwificreds) {
                                    store.numwificreds++;
                                    settingsChanged = true;
                                    Serial.println("Adding new cred.");
                                }
                                len = read - 4;
                                Serial.print("   TO STORE: ");
                                Serial.println(len);
                                Serial.print("READ: ");
                                Serial.println(read);
                                if (len > 64) continue;
                                switch ((char)serialBuffer[3]) {
                                    case 's':
                                        memcpy(ssidBuf, &serialBuffer[4], len);
                                        eepromStore.writePage(ssidBuf, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum);
                                        break;
                                    case 'p':
                                        memcpy(pskBuf, &serialBuffer[4], len);
                                        eepromStore.writePage(pskBuf, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum + 1);
                                        break;
                                    case 'c':
                                        ssidBuf[0] = 0xFF;
                                        eepromStore.writePage(ssidBuf, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum);
                                        eepromStore.writePage(ssidBuf, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum + 1);
                                        if (wifiNum == (store.numwificreds - 1) && store.numwificreds > 0) {
                                            store.numwificreds--;
                                            settingsChanged = true;
                                        }
                                        break;
                                    default:
                                        Serial.println("Unknown wifi option");
                                }
                                break;
                            default:
                                Serial.println("Unknown setting.");
                                Serial.println((char*)serialBuffer);
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

bool SettingsStorage::checkCrc(void) {
    return checkCrcBuf((uint8_t*)this, sizeof(SettingsStorage));
}

bool SettingsStorage::setFromBuf(uint8_t* buf) {
    if (!checkCrcBuf(buf, sizeof(SettingsStorage))) return false;
    memcpy((uint8_t*)this, buf, sizeof(SettingsStorage));
    return true;
}

void SettingsStorage::copyToBuf(uint8_t* buf) {
    memcpy(buf, (uint8_t*)this, sizeof(SettingsStorage));
}

Settings settings;
