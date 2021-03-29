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

   private:
    uint8_t reserved8[3];
    uint32_t reserved32[13];
    uint32_t crc;
};

class Settings {
   public:
    Settings();
    ~Settings();
    bool setFromBuf(uint8_t* buf);
    void copyToBuf(uint8_t* buf);
    bool configure(void);

    SettingsStorage store;

   private:
};

#endif
