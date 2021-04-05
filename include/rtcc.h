#ifndef RTCC_H_
#define RTCC_H_
#include <stdint.h>
#include <time.h>

#define EEPROM_PAGESPERCHIP 512
#define EEPROM_PAGESIZE 64
class RTCCmem {
   public:
    RTCCmem();
    ~RTCCmem();
    bool setFromBuf(uint8_t* buf);
    void copyToBuf(uint8_t* buf);

    uint32_t nextId;
    uint32_t lastSentId;
    uint32_t lastUsedWifi;
    time_t lastNTPcheck;
    uint32_t reserved[11];
    uint32_t crc;
};

class RTCC {
   public:
    RTCC();
    ~RTCC();
    void begin(void);
    void setTime(time_t newTime);
    time_t getTime(void);
    bool loadStore(void);
    void saveStore(void);

    bool running;
    time_t powerfail;
    time_t powerreturn;
    RTCCmem store;

   private:
};

extern RTCC Clock;
#endif
