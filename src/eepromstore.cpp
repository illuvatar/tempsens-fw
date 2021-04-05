#include "eepromstore.h"

#include <Adafruit_MCP23017.h>
#include <SPI.h>

#include "pinout.h"

#define EEPROM_CMD_READ 0b00000011
#define EEPROM_CMD_WRITE 0b00000010
#define EEPROM_CMD_WRDI 0b00000100
#define EEPROM_CMD_WREN 0b00000110
#define EEPROM_CMD_RDSR 0b00000101
#define EEPROM_CMD_WRSR 0b00000001

extern Adafruit_MCP23017 ioexpander;

EEPromStore::EEPromStore() {
    
    this->maxPages = EEPROM_PAGESPERCHIP; // Before settings are loaded assume that we have ONE chip
}

EEPromStore::~EEPromStore() {
}

bool EEPromStore::readPage(uint8_t* buf, uint32_t pageNo) {
    if (pageNo >= this->maxPages) return false;
    uint8_t chipPin = getChipPin(pageNo);

    SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
    waitForIdle(chipPin);

    ioexpander.digitalWrite(chipPin, LOW);
    SPI.transfer(EEPROM_CMD_READ);
    SPI.transfer16(getPageStart(pageNo));
    for (int c = 0; c < EEPROM_PAGESIZE; c++) {
        buf[c] = SPI.transfer(0x00);
    }
    ioexpander.digitalWrite(chipPin, HIGH);

    SPI.endTransaction();
    return true;
}

bool EEPromStore::writePage(uint8_t* buf, uint32_t pageNo) {
    if (pageNo >= this->maxPages) return false;
    uint8_t chipPin = getChipPin(pageNo);

    SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
    waitForIdle(chipPin);
    setWrite(chipPin);
    ioexpander.digitalWrite(chipPin, LOW);
    SPI.transfer(EEPROM_CMD_WRITE);
    SPI.transfer16(getPageStart(pageNo));
    for (int c = 0; c < EEPROM_PAGESIZE; c++) {
        SPI.transfer(buf[c]);
    }
    ioexpander.digitalWrite(chipPin, HIGH);

    waitForIdle(chipPin);
    SPI.endTransaction();
    return true;
}

void EEPromStore::updateMaxPages(uint32_t maxPages) {
    this->maxPages = maxPages;
}

uint8_t EEPromStore::getChipPin(uint32_t pageNo) {
    int basePin = pageNo / EEPROM_PAGESPERCHIP;
    switch (basePin) {
        case 0:
            return IOEXP_EEPROM0;
        case 1:
            return IOEXP_EEPROM1;
        case 2:
            return IOEXP_EEPROM2;
        case 3:
            return IOEXP_EEPROM3;
        case 4:
            return IOEXP_EEPROM4;

        default:
            return 0;
    }
}

uint16_t EEPromStore::getPageStart(uint32_t pageNo) {
    return (pageNo % EEPROM_PAGESPERCHIP) * EEPROM_PAGESIZE;
}

void EEPromStore::waitForIdle(uint8_t chipPin) {
    uint8_t status = readStatus(chipPin);
    while (status & 0x01) {
        // Chip is currently writing.
        delay(1);
        status = readStatus(chipPin);
    }
    if (status & 0x02) {
        clearWrite(chipPin);
    }
}

uint8_t EEPromStore::readStatus(uint8_t chipPin) {
    uint8_t status = 0xff;
    ioexpander.digitalWrite(chipPin, LOW);
    SPI.transfer(EEPROM_CMD_RDSR);
    status = SPI.transfer(0x00);
    ioexpander.digitalWrite(chipPin, HIGH);

    return status;
}
void EEPromStore::setWrite(uint8_t chipPin) {
    ioexpander.digitalWrite(chipPin, LOW);
    SPI.transfer(EEPROM_CMD_WREN);
    ioexpander.digitalWrite(chipPin, HIGH);
    uint8_t status = readStatus(chipPin);
    while (!status & 0x01) {
        delay(10);
        ioexpander.digitalWrite(chipPin, LOW);
        SPI.transfer(EEPROM_CMD_WREN);
        ioexpander.digitalWrite(chipPin, HIGH);
        status = readStatus(chipPin);
    }
}

void EEPromStore::clearWrite(uint8_t chipPin) {
    ioexpander.digitalWrite(chipPin, LOW);
    SPI.transfer(EEPROM_CMD_WRDI);
    ioexpander.digitalWrite(chipPin, HIGH);
}

EEPromStore eepromStore;
