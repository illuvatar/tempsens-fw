#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

class SettingsStorage {
   public:
    SettingsStorage();
    ~SettingsStorage();

    void genCrc(void);
    bool checkCrc(void);
    bool setFromBuf(uint8_t* buf);
    void copyToBuf(uint8_t* buf);

    // Total size should be 64bytes
    uint8_t bmpavail;
    uint8_t dhtavail;
    uint8_t numeeprom;
    uint8_t numtempsens;
    uint8_t numwificreds;
    uint8_t reserved8[3];
    uint32_t serialno;
    uint32_t reserved32[12];
    uint32_t crc;
};

class Settings {
   public:
    Settings();
    ~Settings();
    bool configure(void);
    bool setFromBuf(uint8_t* buf);
    void copyToBuf(uint8_t* buf);

    SettingsStorage store;
    bool urlSet;
    bool registrationTokenSet;
    bool registered;

   private:
   bool validateSerialNo(uint32_t serial);
};

extern Settings settings;
#endif
