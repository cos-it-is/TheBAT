/* CRC32.ino
 * 
 * This sketch demonstrates how to use the Arduino_CRC32
 * library to calculate a CRC-32 checksum.
 * 
 * Alexander Entinger
 */

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <Arduino_CRC32.h>

/**************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************/

Arduino_CRC32 crc32;

/**************************************************************************************
 * SETUP/LOOP
 **************************************************************************************/

void setup()
{
  Serial.begin(9600);

  unsigned long const start = millis();
  for(unsigned long now = millis(); !Serial && ((now - start) < 5000); now = millis()) { };

  char const str[] = "Hello CRC32 ;)";

  uint32_t const crc32_res = crc32.calc((uint8_t const *)str, strlen(str));

  Serial.print("CRC32(\"");
  Serial.print(str);
  Serial.print("\") = 0x");
  Serial.println(crc32_res, HEX);
}

void loop()
{
  
}
