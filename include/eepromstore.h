#ifndef EEPROM_H_
#define EEPROM_H_
#include <stdint.h>

#define EEPROM_PAGESPERCHIP 512
#define EEPROM_PAGESIZE 64

#define EEPROM_SETTINGS_PAGE             0      // Settings structure
#define EEPROM_TEMPSENS_PAGE             1      // 8xTempsens IDs
#define EEPROM_DEVICE_TOKEN_PAGE         2      // Device Token
#define EEPROM_REGISTER_SECRET_PAGE      3      // Secret token used for registering
#define EEPROM_URL_PAGE                  4      // URL for uploading data.

#define EEPROM_FIRST_WIFIPAGE           16      // Wifi SSID/PSK first page (2 pages per Pair)
#define EEPROM_LAST_WIFIPAGE            35      // Last page for Wifi credentials
#define EEPROM_FIRST_SENSORPAGE        128      // First page used for storing measurements not sent to server

class EEPromStore {
   public:
    EEPromStore();
    ~EEPromStore();
    bool readPage(uint8_t* buf, uint32_t pageNo);
    bool writePage(uint8_t* buf, uint32_t pageNo);
    void updateMaxPages(uint32_t maxPages);

   private:
    uint8_t getChipPin(uint32_t pageNo);
    uint16_t getPageStart(uint32_t pageNo);
    void waitForIdle(uint8_t chipPin);
    uint8_t readStatus(uint8_t chipPin);
    void setWrite(uint8_t chipPin);
    void clearWrite(uint8_t chipPin);

    uint32_t maxPages;
};

extern EEPromStore eepromStore;
#endif
