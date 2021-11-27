// VISHAY NTC 2381-640-55103
// Datasheet: http://connect.iobridge.com/wp-content/uploads/2012/02/thermistor_2381-640-55103.pdf
const int sensorPin = A1;
// Analog ref
const float VAref = 5.0; // Volts
// NTC base resistance
const float R0 = 1e4; // Ohm \pm 1%
// Temperature corresponding to R0
const float T0 = 25.0 + 273.15; // Kelvin
// divider resistance
const float R = 1e4; // Ohm \pm 1%
const float BETA = 3977; // Kelvin \pm 0.75%

// For pin out like this:
// Aref - NTC - R - GND
//            | sensorPin

void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(sensorPin); // \pm 0.5/sensorValue
  float VR = (VAref / 1024.00) * sensorValue;
  float NTC = R * (VAref - VR) / VR;
  // R = R0 * exp(beta/T - beta/T0)
  // ln(R/R0) + beta/T0 = beta/T
  float T = BETA / (log(NTC/R0) + BETA/T0);
  Serial.print(F("t = "));
  Serial.println(T - 273.15);
  delay(1000);
}
