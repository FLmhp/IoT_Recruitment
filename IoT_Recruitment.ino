#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define I2C_SDA 18
#define I2C_SCL 17

Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!bme.begin(0x76, &Wire)) {
    Serial.println("BME280 init failed");
    while (1);
  }
  Serial.println("BME280 OK");
}

void loop() {
  Serial.print("T: "); Serial.print(bme.readTemperature());  Serial.println(" C");
  Serial.print("H: "); Serial.print(bme.readHumidity());     Serial.println(" %");
  Serial.print("P: "); Serial.print(bme.readPressure() / 100.0F); Serial.println(" hPa");
  Serial.println("-----");
  delay(1000);
}