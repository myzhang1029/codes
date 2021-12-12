#include <Wire.h>
#define I2C_SLAVE_ADDR 0x48

const int analogInPin = A0;

void setup() {
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onRequest(requestEvents);
}

void loop() {}

void requestEvents() {
  int16_t sensorValue = analogRead(analogInPin);
  Wire.write((const uint8_t *) &sensorValue, sizeof(int16_t));
}
