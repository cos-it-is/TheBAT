/**************************************************************************/
/*!
    @file     ntag424_examples.ino
    @author   Thilo
    @license  BSD (see license.txt)

    This example will wait for any NTAG424 card or tag,
    and will try to authenticate using the default key (16*0x0) and read the uid
   an d the tamperstatus. The last examples can write to the tag an are
   therefore commented to prevent unwanted dataloss.


*/
/**************************************************************************/

#include <Adafruit_PN532_NTAG424.h>
#include <SPI.h>
#include <Wire.h>
//#include <ntag424.h>

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
// Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
// Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // for Leonardo/Micro/Zero

  Serial.println("Hello!");
#ifdef PN532_RSTPD_N
  pinMode(PN532_RSTPD_N, OUTPUT);
  digitalWrite(PN532_RSTPD_N, HIGH);
#endif
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1)
      ; // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN53x");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                     // card type)

  // Wait for an NTAG242 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found a tag");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    //&& (nfc.ntag424_isNTAG424())
    if (((uidLength == 7) || (uidLength == 4)) && (nfc.ntag424_isNTAG424())) {
      Serial.println("Found an NTAG424-tag");
      // select application
      uint8_t filename[7] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};
      nfc.ntag424_ISOSelectFileByDFN(filename);

      // Authenticate with default_key. Will only work if no keys are set.
      uint8_t key[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      uint8_t keyno = 0;
      uint8_t success = nfc.ntag424_Authenticate(key, keyno, 0x71);

      // Display the current page number
      Serial.print("Response ");
      // Display the results, depending on 'success'
      if (success == 1) {
        Serial.println("Authentication successful.");
        uint8_t carduid[16];
        uint8_t bytesread = nfc.ntag424_GetCardUID(carduid);
        Serial.println("CardUId:");
        nfc.PrintHexChar(carduid, bytesread);

        uint8_t ttstatus[16];
        bytesread = nfc.ntag424_GetTTStatus(ttstatus);
        Serial.println("TamperTag Status:");
        nfc.PrintHexChar(ttstatus, bytesread);

        // the next few methods write to the picc and may ruin your tag so you
        // have to uncomment them on your on risk :-)
        uint8_t oldkey[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t newkey[16] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

        // change key 1 - uncomment next line
        // nfc.ntag424_ChangeKey(oldkey, newkey, 1);
        // change key 1 back to default-key
        // nfc.ntag424_ChangeKey(newkey, oldkey, 1);

        // change filesettings (No Mirroring, No SDM)
        // uint8_t fileSettings[] = {0x00, 0xE0, 0xEE};
        // nfc.ntag424_ChangeFileSettings((uint8_t) 2, fileSettings, (uint8_t)
        // sizeof(fileSettings), (uint8_t) NTAG424_COMM_MODE_FULL);

        // format NDEF record, ATTENTION: this will wipe any existing
        // ndef-record record on the card. nfc.ntag424_FormatNDEF();
      } else {
        Serial.println("Authentication failed.");
      }
    } else {
      Serial.println("This doesn't seem to be an NTAG424 tag. (UUID length != "
                     "7 bytes and UUID length != 4)!");
    }
    // Wait a bit before trying again
    delay(3000);
  }
}
