#include "settings.h"

#include <CRC32.h>
#include <errno.h>

#include "eepromstore.h"
#include "tools.h"

Settings::Settings() {}
Settings::~Settings() {}

bool Settings::setFromBuf(uint8_t* buf) {
    if (store.setFromBuf(buf)) {
        if (!validateSerialNo(store.serialno)) store.serialno = 0;  // Make sure the serial# is 0 if not valid.
        return true;
    }
    return false;
}

void Settings::copyToBuf(uint8_t* buf) {
    store.genCrc();
    store.copyToBuf(buf);
}

/*
  Settings tree
    w - write settings to eeprom and continue boot
    l - list settings
    s - set
        s <serialno> (in base10 format, can only be set ONCE)
        h <0,1> Set if humidity sensor is installed
        b <0,1> Set if barometer is installed
        e <1-5> Set number of EEPROMS installed
        w <0-9> Setup WIFI credentials
            s ssid
            p psk
            c clear
        u Set url (not including /api/) (Max 63 characters)
        t Set registration token secret (Max 63 characters)
*/

// TODO: Add some kind of response after processing each command.
bool Settings::configure(void) {
    bool setupDone = false;
    bool settingsChanged = false;
    uint8_t serialBuffer[129];
    uint8_t eeprombufA[65];
    uint8_t eeprombufB[65];
    size_t read = 0;
    uint8_t newValue = 0;
    uint8_t wifiNum = 0;
    size_t len = 0;
    uint32_t intermediate_u32 = 0;

    while (!setupDone) {
        Serial.setTimeout(30000);
        read = Serial.readBytesUntil('\n', serialBuffer, 128);
        if (read > 0) {
            memset(eeprombufA, 0, sizeof(eeprombufA));
            memset(eeprombufB, 0, sizeof(eeprombufB));
            serialBuffer[read] = 0x00;                                        // Make sure string is asciiz (Will remove linefeed if string was short enough)
            if (serialBuffer[read - 1] == 0x0d) serialBuffer[--read] = 0x00;  // Trim any CR

            switch ((char)serialBuffer[0]) {
                case 'w':
                    setupDone = true;
                    break;
                case 'l':
                    Serial.println("Current settings:      ");
                    Serial.print("Serial#:                 ");
                    Serial.println(store.serialno);
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

                    eepromStore.readPage(eeprombufA, EEPROM_URL_PAGE);
                    if (!(eeprombufA[0] == 0 || eeprombufA[0] == 0xff)) {
                        eeprombufA[64] = 0x00;
                        Serial.print("Server URL:              ");
                        Serial.println((char*)eeprombufA);
                    }
                    eepromStore.readPage(eeprombufA, EEPROM_REGISTER_SECRET_PAGE);
                    if (!(eeprombufA[0] == 0 || eeprombufA[0] == 0xff)) {
                        eeprombufA[64] = 0x00;
                        Serial.print("Register Secret Token:   ");
                        Serial.println((char*)eeprombufA);
                    }

                    Serial.print("WIFI Credentials:        ");
                    Serial.println(store.numwificreds);
                    for (int c = 0; c < store.numwificreds; c++) {
                        eepromStore.readPage(eeprombufA, EEPROM_FIRST_WIFIPAGE + 2 * c);
                        if (eeprombufA[0] == 0xff) continue;  // Invalid setting.
                        eepromStore.readPage(eeprombufB, EEPROM_FIRST_WIFIPAGE + 2 * c + 1);
                        if (eeprombufB[0] == 0xff) continue;  // Invalid setting.
                        Serial.print("    ");
                        if (store.numwificreds < 10) Serial.print(" ");
                        Serial.print(store.numwificreds);
                        Serial.print(" SSID '");
                        Serial.print((char*)eeprombufA);
                        Serial.println("'");
                        Serial.print("       PSK  '");
                        Serial.print((char*)eeprombufB);
                        Serial.println("'");
                    }

                    eepromStore.readPage(eeprombufA, EEPROM_DEVICE_TOKEN_PAGE);
                    if (!(eeprombufA[0] == 0 || eeprombufA[0] == 0xff)) {
                        Serial.print("Device Registration:     ");
                        Serial.println((char*)eeprombufA);
                    }
                    break;
                case 's':
                    if (read > 2) {
                        switch ((char)serialBuffer[1]) {
                            case 's':  // Serialno
                                //TODO, leave if serial is already set.
                                errno = 0;
                                intermediate_u32 = strtoul((char*)&serialBuffer[2], NULL, 10);

                                if (errno == 0 && validateSerialNo(intermediate_u32)) {
                                    store.serialno = intermediate_u32;
                                    settingsChanged = true;
                                }
                                break;
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
                                        memcpy(eeprombufA, &serialBuffer[4], len);
                                        eepromStore.writePage(eeprombufA, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum);
                                        break;
                                    case 'p':
                                        memcpy(eeprombufA, &serialBuffer[4], len);
                                        eepromStore.writePage(eeprombufA, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum + 1);
                                        break;
                                    case 'c':
                                        eeprombufA[0] = 0xFF;
                                        eepromStore.writePage(eeprombufA, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum);
                                        eepromStore.writePage(eeprombufA, EEPROM_FIRST_WIFIPAGE + 2 * wifiNum + 1);
                                        if (wifiNum == (store.numwificreds - 1) && store.numwificreds > 0) {
                                            store.numwificreds--;
                                            settingsChanged = true;
                                        }
                                        break;
                                    default:
                                        Serial.println("Unknown wifi option");
                                }
                                break;
                            case 'u':  // Url
                                len = read - 2;
                                memcpy(eeprombufA, &serialBuffer[2], len);
                                eepromStore.writePage(eeprombufA, EEPROM_URL_PAGE);
                                break;
                            case 't':  // registration token secret
                                len = read - 2;
                                memcpy(eeprombufA, &serialBuffer[2], len);
                                eepromStore.writePage(eeprombufA, EEPROM_REGISTER_SECRET_PAGE);
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
            // TODO: Should leave settings after a few read timeouts to make sure we boot even if we accidently entered config.
            Serial.println("Didn't read anything.");
        }

        Serial.println("READY");
        delay(100);
    }

    return settingsChanged;
}

bool Settings::validateSerialNo(uint32_t serial) {
    // A valid serial has odd number of bits in first and last byte, and even in the middle two.
    int count = 0;
    int expectedMod = -1;

    for (int i = 0; i < 4; i++) {
        expectedMod = 0;
        if (i == 0 || i == 3) expectedMod = 1;
        count = 0;
        for (int j = 0; j < 8; j++) {
            count += serial & 0x00000001;
            serial = serial >> 1;
        }
        if (count % 2 != expectedMod) {
            return false;
        }
    }
    return true;
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
