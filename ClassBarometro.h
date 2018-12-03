#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bme; // I2C

class ClassBarometro{
  private:
    boolean estado = false;
  public:
    void setup(){
      estado = bme.begin();
    }

    float temperatura(){
      return estado? bme.readTemperature() : 0; //C
    }

    float presion(){
      return estado? bme.readPressure()/100 : 0; // hPa
    }

    float altitud(){
      return estado? bme.readAltitude() : 0; // metros
    }
};
ClassBarometro Barometro;
