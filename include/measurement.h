#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>

class Measurement {
   public:
    static float NaN();

    Measurement();
    ~Measurement();

    void genCrc();
    bool checkCrc();

    enum TYPE : uint8_t { TYPE_SENSORREAD = 0x01,
                          TYPE_PWRFAIL = 0x20,
                          TYPE_UNKNOWN = 0xff };

    // Total size should be 64bytes
    uint16_t id;
    TYPE type;
    uint8_t bits;  // b0 = ext power present
    uint32_t timestamp;
    union {
        float batteryvoltage;
        uint32_t powerfail;
    };
    union {
        float baropress;
        uint32_t powerback;
    };
    float barotemp;
    float humidity;
    float humidtemp;
    float tempsens0;
    float tempsens1;
    float tempsens2;
    float tempsens3;
    float tempsens4;
    float tempsens5;
    float tempsens6;
    float tempsens7;
   private:
    uint32_t crc;
};

#endif
