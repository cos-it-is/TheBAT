#include "Adafruit_AW9523.h"
#include "Wire.h"

#define I2C_SDA 48
#define I2C_SCL 47
#define AW9523_ADDRESS 0x5B

Adafruit_AW9523 aw9523;

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (!aw9523.begin(AW9523_ADDRESS)) {
    Serial.println("Failed to initialize AW9523, check wiring!");
    while (1);
  } else {
    Serial.println("AW9523 Initialized successfully");
  }

  // Initialize all pins to digital output with LED mode
  for (uint8_t pin=0; pin<16; pin++) {
    aw9523.pinMode(pin, AW9523_LED_MODE);
  }   
}

void loop() {
  // Toggle state of all GPIOs 
  static bool state = LOW;

  for (uint8_t pin=0; pin<16; pin++) {
    aw9523.digitalWrite(pin, state);
  }  

  state = !state;
  delay(1000);
}