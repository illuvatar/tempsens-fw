#ifndef COMMUNICATION_h
#define COMMUNICATION_h

#include <Arduino.h>

class Communication {
   public:
    Communication();
    ~Communication();
    void begin(void);
    time_t getNtpTime();

   private:
    bool begun;
    char ssid[65];
    char psk[65];
};

extern Communication Comms;

#endif
