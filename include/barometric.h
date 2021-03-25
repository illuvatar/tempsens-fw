#ifndef BAROMETRIC_H_
#define BAROMETRIC_H_
#include <Adafruit_BMP280.h>

#include "measurement.h"

class Barometric {
   public:
    Barometric();
    ~Barometric();
    // Initializes Pressure sensor
    void setup(void);
    // Takes measurement and updates the storage object m
    void measure(Measurement& m);

   private:
    Adafruit_BMP280 bmp;  // use I2C interface
    boolean haveBmp;
};
#endif
