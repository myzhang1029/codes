/// CW Keyer for ATTiny85

// For use with damellis/attiny core
//                    +-\/-+
// RESET/A0/PB5      1|    |8  Vcc
// DIT (PULL UP) PB3 2|    |7  PB2/A1 SPEED POT
// DAH (PULL UP) PB4 3|    |6  PB1 OUT2 high when active
//               GND 4|    |5  PB0 OUT  high when active
//                    +----+
// Two outputs are identical, for maybe keying and an LED

#define OUT 0
#define OUT2 1
// This needs to be analog input capable
#define SPEED A1
#define DIT 3
#define DAH 4

void setup() {
    pinMode(OUT, OUTPUT);
    pinMode(OUT2, OUTPUT);
    pinMode(SPEED, INPUT);
    pinMode(DIT, INPUT);
    pinMode(DAH, INPUT);
    digitalWrite(OUT, LOW);
}

static void make(int duration) {
    digitalWrite(OUT, HIGH);
    digitalWrite(OUT2, HIGH);
    delay(duration);
    digitalWrite(OUT, LOW);
    digitalWrite(OUT2, LOW);
}

void loop() {
    // `delay` is in ms and this gives max 1023
    // mark length = 60/(50 "wpm") seconds = 1200/"wpm" ms
    // A sensible range is 5-inf wpm, or 0-240 ms, so let's scale to 256
    int mark = analogRead(SPEED) >> 2;
    if (!digitalRead(DIT)) {
        make(mark);
        delay(mark);
    }
    if (!digitalRead(DAH)) {
        make(3 * mark);
        delay(mark);
    }
}
