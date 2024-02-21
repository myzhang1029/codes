/// CW Keyer for ATTiny85

#define OUT 5
#define OUT2 6
// This needs to be analog input capable
#define SPEED A1
#define DIT 2
#define DAH 3

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
