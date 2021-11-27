#include "Adafruit_BMP280.h"
#include "Adafruit_SHT31.h"
#include "lcd12864b_s.h"

const int portNumber = 42;
const int meterInPin = A0;
const unsigned int delayLength = 2;

Adafruit_BMP280 bmp; // I2C
Adafruit_SHT31 sht31;
LCD12864B_S lcd(A2, A1, A0);

/* Drive a QYF-068 beeper.
    @param pin Output pin of the beeper.
    @param freq The frequency to create.
    @param dur Duration in seconds.
*/
void driveBeeper(int pin, unsigned long freq, float dur) {
  /* Beeper data (old):
      delay/ms | frequency/Hz
      1        | 4432.0
      2        | 4221.2
      3        | 4155.3
      4        | 4115.8
      5        | 4089.9
      6        | 4074.6
      7        | 4058.1
      (1+2)/2  | 4287.0
      (1+3)/2  | 4208.0
      (1+4)/2  | 4168.5
      (1+5)/2  | 4142.2

      Beeper curve from regression: v=429.872/d+4001.3, d=429.872/(v-4001.3)

      Beeper data (new):
      delay/us freq+-20/Hz residual1/Hz residual2/Hz residual3/Hz
      60       7920        5.8389248    -99.540106   -25.030691
      70       6813        -10.510046   -60.89152    -12.593434
      80       6000        2.9445392    -14.65508    13.984508
      90       5350        0.8291011    3.6399292    16.989575
      100      4842        14.375606    30.275936    31.393628
      110      4394        -4.7448454   19.705397    10.815126
      120      4017        -22.850089   7.2299469    -10.000292
      130      3747        11.897882    45.673797    21.386662
      140      3461        -12.106698   24.05424     -6.2816634
      150      3251        5.5429995    43.183958    7.605788
      160      3030        -15.814758   22.67246     -17.492692
      170      2878        8.6890942    47.57408     3.3615902
      180      2715        2.8568016    41.819965    -5.9901588
      190      2569        -2.2992067   36.513651    -14.515408
      200      2453        8.6386377    47.137968    -6.7881322
      210      2341        11.632968    49.702827    -6.8444065

      Beeper curve from regression *1 (y~k(x+a)^-1): v=495139/(d+2.56364), d=495139/v-2.56364
      Beeper curve from regression 2 (y~kx^-1):      v=481172/d, d=470164/v
      Beeper curve from regression 3 (y~kx^-1+b):    v=470164/d+108.97, d=470164/(v-108.97)
  */
  unsigned long itersSoFar, finalIter;
  double avgDelay;

  avgDelay = 495139 / freq - 2.56364;
  if (avgDelay >= 3000.0) {
    avgDelay /= 1000;
    finalIter = (dur * 1000) / (avgDelay * 2);
    for (itersSoFar = 0; itersSoFar < finalIter; ++itersSoFar) {
      digitalWrite(pin, HIGH);
      delay((unsigned long)avgDelay);
      digitalWrite(pin, LOW);
      delay((unsigned long)avgDelay + 1);
    }
  }
  else {
    finalIter = (dur * 1000000) / (avgDelay * 2);
    for (itersSoFar = 0; itersSoFar < finalIter; ++itersSoFar) {
      digitalWrite(pin, HIGH);
      delayMicroseconds((unsigned int)avgDelay);
      digitalWrite(pin, LOW);
      delayMicroseconds((unsigned int)avgDelay + 1);
    }
  }
}

void setup() {
  Serial.println("Home LCD Console Initializng");

  if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  Serial.println("BMP280 Initialized");

  sht31 = Adafruit_SHT31();
  if (! sht31.begin(0x44)) {
    Serial.println("Could not find a valid SHT31 sensor, check wiring!");
    while (1);
  }
  Serial.println("SHT31 Initialized");

  lcd.init();
  Serial.println("LCD Initialized");
  lcd.printxy(0, 1, "Welcome to home");
  lcd.printxy(0, 2, "LCD Console");
  const unsigned long notes[] = {523, 523, 523, 392, 659, 659, 659, 523, 523, 659, 784, 784, 698, 659, 587, 587, 659, 698, 698, 659, 587, 659, 523,  523, 659, 587, 392, 494, 587, 523};
  const float lengths[] = {0.5, 0.5, 1, 1, 0.5, 0.5, 1, 1, 0.5, 0.5, 1, 1, 0.5, 0.5, 2, 0.5, 0.5, 1, 1, 0.5, 0.5, 1, 1, 0.5, 0.5, 1, 1, 0.5, 0.5, 3};
  pinMode(portNumber, OUTPUT);
  for (int i = 0; i < sizeof(notes) / sizeof(unsigned long); ++i)
    driveBeeper(portNumber, notes[i], lengths[i]);
}

void loop() {
  char buffer[9];
  float t1 = sht31.readTemperature();
  float h = sht31.readHumidity();
  float t2 = bmp.readTemperature();
  float p = bmp.readPressure();
  float el = bmp.readAltitude(1013.25); /* Adjusted to local forecast! */
  
  Serial.print("Temperature = ");
  Serial.print(t2);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(p);
  Serial.println(" Pa");

  Serial.print("Approx altitude = ");
  Serial.print(el);
  Serial.println(" m");

  if (! isnan(t1)) {
    Serial.print("Temperature = ");
    Serial.print(t1);
    Serial.println(" *C");
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {
    Serial.print("Humidity = ");
    Serial.print(h);
    Serial.println(" %");
  } else { 
    Serial.println("Failed to read humidity");
  }
  Serial.println();
  
  lcd.cls();
  dtostrf(t1, 2, 3, buffer);
  lcd.printfxy(0, 0, "temp = %s *C", buffer);
  dtostrf(h, 2, 4, buffer);
  lcd.printfxy(0, 1, "hum. = %s %%", buffer);
  dtostrf(p / 1000, 2, 2, buffer);
  lcd.printfxy(0, 2, "prs. = %s kPa", buffer);
  dtostrf(el, 4, 2, buffer);
  lcd.printfxy(0, 3, "alt. = %s m", buffer);
  delay(2000);
}
