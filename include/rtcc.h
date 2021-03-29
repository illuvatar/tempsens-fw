#ifndef RTCC_H_
#define RTCC_H_
#include <stdint.h>

#define EEPROM_PAGESPERCHIP 512
#define EEPROM_PAGESIZE 64
class RTCCmem {
   public:
    RTCCmem();
    ~RTCCmem();
    bool setFromBuf(uint8_t* buf);
    void copyToBuf(uint8_t* buf);

    uint32_t nextId;
    uint32_t reserved[14];
    uint32_t crc;
};

class RTCC {
   public:
    RTCC();
    ~RTCC();
    void setup(void);
    void setTime(uint32_t newTime);
    uint32_t getTime(void);
    bool loadStore(void);
    void saveStore(void);

    bool running;
    uint32_t powerfail;
    uint32_t powerreturn;
    RTCCmem store;

   private:
};
#endif
