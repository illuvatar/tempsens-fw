#ifndef EEPROM_H_
#define EEPROM_H_
#include <stdint.h>

#define EEPROM_PAGESPERCHIP 512
#define EEPROM_PAGESIZE 64

#define EEPROM_FIRST_SENSORPAGE 128
#define EEPROM_FIRST_WIFIPAGE 16

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
