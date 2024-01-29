/**************************************************************************/
/*! 
    @file     readntag203.pde
    @author   KTOWN (Adafruit Industries)
    @license  BSD (see license.txt)

    This example will wait for any NTAG424 card or tag,
    and will attempt to read the NDEF file from it.

*/
/**************************************************************************/
#define NTAG424DEBUG

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532_NTAG424.h>

// If using the breakout with SPI, define the pins for SPI communication.
/*
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)
*/
// NFC Pins

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK (17)
#define PN532_MOSI (13)
#define PN532_SS (15)
#define PN532_MISO (12)

// This is optional to powerup/down the PN532-board.
// For RSTPD_N to work i had to desolder a 10k resistor between RSTPD_N and VCC
#define PN532_RSTPD_N (2)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
//#define PN532_IRQ   (14)
//#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);


void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10); // for Leonardo/Micro/Zero

  Serial.println("Hello!");
#ifdef PN532_RSTPD_N
  pinMode(PN532_RSTPD_N, OUTPUT);
  digitalWrite(PN532_RSTPD_N, HIGH);
#endif
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN53x"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A tag");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (((uidLength == 7) || (uidLength == 4)) && (nfc.ntag424_isNTAG424()))
    {
      uint8_t data[256];
      uint8_t bytesread = nfc.ntag424_ISOReadFile(data, sizeof(data));
      
      // Display the current page number
      Serial.print("Response ");

      // Display the results, depending on 'success'
      if (bytesread)
      {
        //0-terminate the buffer
        if (bytesread < sizeof(data)){
          data[bytesread] = 0;
        }
        else{
          data[sizeof(data)-1] = 0;
        }
        String lnurl = (char*)data;
        // Dump the page data
       Serial.println(lnurl);
        //nfc.PrintHexChar(data, bytesread);
      }
      else
      {
        Serial.println("This doesn't seem to be an NTAG424 tag.");
      }
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG424 tag. (UUID length != 7 bytes and UUID length != 4)!");
    }
    // Wait a bit before trying again
    delay(2000);
  }
}
