#ifndef COMMUNICATION_h
#define COMMUNICATION_h

#include <Arduino.h>

class Communication {
   public:
    Communication();
    ~Communication();
    void begin(void);
    time_t getNtpTime();
    bool registerDevice(void);

   private:
    String jsonQuery(String service, String query);
    bool begun;
    char ssid[65];
    char psk[65];
    char registrationBase64[89];
    String baseUrl;
    String server;
    uint16_t port;
    bool ssl;
};

extern Communication Comms;

#endif
