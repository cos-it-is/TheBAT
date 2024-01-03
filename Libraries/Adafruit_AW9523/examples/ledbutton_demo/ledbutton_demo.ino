#include <Adafruit_AW9523.h>

Adafruit_AW9523 aw;

uint8_t LedPin = 0;
uint8_t ButtonPin = 1;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);  // wait for serial port to open
  
  Serial.println("Adafruit AW9523 Button + LED test!");

  if (! aw.begin(0x58)) {
    Serial.println("AW9523 not found? Check wiring!");
    while (1) delay(10);  // halt forever
  }

  Serial.println("AW9523 found!");
  aw.pinMode(LedPin, OUTPUT);
  aw.pinMode(ButtonPin, INPUT);
  aw.enableInterrupt(ButtonPin, true);
}


void loop() {
  aw.digitalWrite(LedPin, aw.digitalRead(ButtonPin));

  delay(10);
}