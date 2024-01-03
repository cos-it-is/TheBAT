/**************************************************************************/
/*!
    @file Adafruit_PN532.cpp

    @section intro_sec Introduction

    Driver for NXP's PN532 NFC/13.56MHz RFID Transceiver

    This is a library for the Adafruit PN532 NFC/RFID breakout boards
    This library works with the Adafruit NFC breakout
    ----> https://www.adafruit.com/products/364

    Check out the links above for our tutorials and wiring diagrams
    These chips use SPI or I2C to communicate.

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section author Author

    Adafruit Industries

    @section license License

    BSD (see license.txt)

    @section  HISTORY

    v2.2 - Added startPassiveTargetIDDetection() to start card detection and
            readDetectedPassiveTargetID() to read it, useful when using the
            IRQ pin.

    v2.1 - Added NTAG2xx helper functions

    v2.0 - Refactored to add I2C support from Adafruit_NFCShield_I2C library.

    v1.4 - Added setPassiveActivationRetries()

    v1.2 - Added writeGPIO()
         - Added readGPIO()

    v1.1 - Changed readPassiveTargetID() to handle multiple UID sizes
         - Added the following helper functions for text display
             static void PrintHex(const byte * data, const uint32_t numBytes)
             static void PrintHexChar(const byte * pbtData, const uint32_t
   numBytes)
         - Added the following Mifare Classic functions:
             bool mifareclassic_IsFirstBlock (uint32_t uiBlock)
             bool mifareclassic_IsTrailerBlock (uint32_t uiBlock)
             uint8_t mifareclassic_AuthenticateBlock (uint8_t * uid, uint8_t
   uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t * keyData) uint8_t
   mifareclassic_ReadDataBlock (uint8_t blockNumber, uint8_t * data) uint8_t
   mifareclassic_WriteDataBlock (uint8_t blockNumber, uint8_t * data)
         - Added the following Mifare Ultalight functions:
             uint8_t mifareultralight_ReadPage (uint8_t page, uint8_t * buffer)
*/
/**************************************************************************/

#include "Adafruit_PN532_NTAG424.h"

Arduino_CRC32 crc32; ///< Arduino CRC32 Class

byte pn532ack[] = {0x00, 0x00, 0xFF,
                   0x00, 0xFF, 0x00}; ///< ACK message from PN532
byte pn532response_firmwarevers[] = {
    0x00, 0x00, 0xFF,
    0x06, 0xFA, 0xD5}; ///< Expected firmware version message from PN532

// Uncomment these lines to enable debug output for PN532(SPI) and/or MIFARE
// related code

// #define PN532DEBUG
// #define MIFAREDEBUG
#define NTAG424DEBUG

// If using Native Port on Arduino Zero or Due define as SerialUSB
#define PN532DEBUGPRINT Serial ///< Fixed name for debug Serial instance
//#define PN532DEBUGPRINT SerialUSB ///< Fixed name for debug Serial instance

#define PN532_PACKBUFFSIZ 64                ///< Packet buffer size in bytes
byte pn532_packetbuffer[PN532_PACKBUFFSIZ]; ///< Packet buffer used in various
                                            ///< transactions

/**************************************************************************/
/*!
    @brief  Instantiates a new PN532 class using software SPI.

    @param  clk       SPI clock pin (SCK)
    @param  miso      SPI MISO pin
    @param  mosi      SPI MOSI pin
    @param  ss        SPI chip select pin (CS/SSEL)
*/
/**************************************************************************/
Adafruit_PN532::Adafruit_PN532(uint8_t clk, uint8_t miso, uint8_t mosi,
                               uint8_t ss) {
  _cs = ss;
  spi_dev = new Adafruit_SPIDevice(ss, clk, miso, mosi, 100000,
                                   SPI_BITORDER_LSBFIRST, SPI_MODE0);
}

/**************************************************************************/
/*!
    @brief  Instantiates a new PN532 class using I2C.

    @param  irq       Location of the IRQ pin
    @param  reset     Location of the RSTPD_N pin
    @param  theWire   pointer to I2C bus to use
*/
/**************************************************************************/
Adafruit_PN532::Adafruit_PN532(uint8_t irq, uint8_t reset, TwoWire *theWire)
    : _irq(irq), _reset(reset) {
  pinMode(_irq, INPUT);
  pinMode(_reset, OUTPUT);
  i2c_dev = new Adafruit_I2CDevice(PN532_I2C_ADDRESS, theWire);
}

/**************************************************************************/
/*!
    @brief  Instantiates a new PN532 class using hardware SPI.

    @param  ss        SPI chip select pin (CS/SSEL)
    @param  theSPI    pointer to the SPI bus to use
*/
/**************************************************************************/
Adafruit_PN532::Adafruit_PN532(uint8_t ss, SPIClass *theSPI) {
  _cs = ss;
  spi_dev = new Adafruit_SPIDevice(ss, 1000000, SPI_BITORDER_LSBFIRST,
                                   SPI_MODE0, theSPI);
}

/**************************************************************************/
/*!
    @brief  Instantiates a new PN532 class using hardware UART (HSU).

    @param  reset     Location of the RSTPD_N pin
    @param  theSer    pointer to HardWare Serial bus to use
*/
/**************************************************************************/
Adafruit_PN532::Adafruit_PN532(uint8_t reset, HardwareSerial *theSer)
    : _reset(reset) {
  pinMode(_reset, OUTPUT);
  ser_dev = theSer;
}

/**************************************************************************/
/*!
    @brief  Setups the HW

    @returns  true if successful, otherwise false
*/
/**************************************************************************/
bool Adafruit_PN532::begin() {
#ifdef NTAG424DEBUG
    Serial.println("NTAG424DEBUG: On");
    Serial.println("EncBuffer: 52");
#endif
  if (spi_dev) {
    // SPI initialization
    if (!spi_dev->begin()) {
      return false;
    }
  } else if (i2c_dev) {
    // I2C initialization
    // PN532 will fail address check since its asleep, so suppress
    if (!i2c_dev->begin(false)) {
      return false;
    }
  } else if (ser_dev) {
    ser_dev->begin(115200);
    // clear out anything in read buffer
    while (ser_dev->available())
      ser_dev->read();
  } else {
    // no interface specified
    return false;
  }
  reset(); // HW reset - put in known state
  delay(10);
  wakeup(); // hey! wakeup!
  return true;
}

/**************************************************************************/
/*!
    @brief  Perform a hardware reset. Requires reset pin to have been provided.
*/
/**************************************************************************/
void Adafruit_PN532::reset(void) {
  // see Datasheet p.209, Fig.48 for timings
  if (_reset != -1) {
    digitalWrite(_reset, LOW);
    delay(1); // min 20ns
    digitalWrite(_reset, HIGH);
    delay(2); // max 2ms
  }
}

/**************************************************************************/
/*!
    @brief  Wakeup from LowVbat mode into Normal Mode.
*/
/**************************************************************************/
void Adafruit_PN532::wakeup(void) {
  // interface specific wakeups - each one is unique!
  if (spi_dev) {
    // hold CS low for 2ms
    digitalWrite(_cs, LOW);
    delay(2);
  } else if (ser_dev) {
    uint8_t w[3] = {0x55, 0x00, 0x00};
    ser_dev->write(w, 3);
    delay(2);
  }

  // PN532 will clock stretch I2C during SAMConfig as a "wakeup"

  // need to config SAM to stay in Normal Mode
  SAMConfig();
}

/**************************************************************************/
/*!
    @brief  Prints a hexadecimal value in plain characters

    @param  data      Pointer to the byte data
    @param  numBytes  Data length in bytes
*/
/**************************************************************************/
void Adafruit_PN532::PrintHex(const byte *data, const uint32_t numBytes) {
  uint32_t szPos;
  for (szPos = 0; szPos < numBytes; szPos++) {
    PN532DEBUGPRINT.print(F("0x"));
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      PN532DEBUGPRINT.print(F("0"));
    PN532DEBUGPRINT.print(data[szPos] & 0xff, HEX);
    if ((numBytes > 1) && (szPos != numBytes - 1)) {
      PN532DEBUGPRINT.print(F(" "));
    }
  }
  PN532DEBUGPRINT.println();
}

/**************************************************************************/
/*!
    @brief  Prints a hexadecimal value in plain characters, along with
            the char equivalents in the following format

            00 00 00 00 00 00  ......

    @param  data      Pointer to the byte data
    @param  numBytes  Data length in bytes
*/
/**************************************************************************/
void Adafruit_PN532::PrintHexChar(const byte *data, const uint32_t numBytes) {
  uint32_t szPos;
  for (szPos = 0; szPos < numBytes; szPos++) {
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      PN532DEBUGPRINT.print(F("0"));
    PN532DEBUGPRINT.print(data[szPos], HEX);
    if ((numBytes > 1) && (szPos != numBytes - 1)) {
      PN532DEBUGPRINT.print(F(" "));
    }
  }
  PN532DEBUGPRINT.print(F("  "));
  for (szPos = 0; szPos < numBytes; szPos++) {
    if (data[szPos] <= 0x1F)
      PN532DEBUGPRINT.print(F("."));
    else
      PN532DEBUGPRINT.print((char)data[szPos]);
  }
  PN532DEBUGPRINT.println();
}

/**************************************************************************/
/*!
    @brief  Checks the firmware version of the PN5xx chip

    @returns  The chip's firmware version and ID
*/
/**************************************************************************/
uint32_t Adafruit_PN532::getFirmwareVersion(void) {
  uint32_t response;

  pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

  if (!sendCommandCheckAck(pn532_packetbuffer, 1)) {
    return 0;
  }

  // read data packet
  readdata(pn532_packetbuffer, 13);

  // check some basic stuff
  if (0 != memcmp((char *)pn532_packetbuffer,
                  (char *)pn532response_firmwarevers, 6)) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("Firmware doesn't match!"));
#endif
    return 0;
  }

  int offset = 7;
  response = pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];

  return response;
}

/**************************************************************************/
/*!
    @brief  Sends a command and waits a specified period for the ACK

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    The size of the command in bytes
    @param  timeout   timeout before giving up

    @returns  1 if everything is OK, 0 if timeout occured before an
              ACK was recieved
*/
/**************************************************************************/
// default timeout of one second
bool Adafruit_PN532::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen,
                                         uint16_t timeout) {

  // I2C works without using IRQ pin by polling for RDY byte
  // seems to work best with some delays between transactions
  uint8_t SLOWDOWN = 0;
  if (i2c_dev)
    SLOWDOWN = 1;

  // write the command
  writecommand(cmd, cmdlen);

  // I2C TUNING
  delay(SLOWDOWN);

  // Wait for chip to say its ready!
  if (!waitready(timeout)) {
    return false;
  }

#ifdef PN532DEBUG
  if (spi_dev == NULL) {
    PN532DEBUGPRINT.println(F("IRQ received"));
  }
#endif

  // read acknowledgement
  if (!readack()) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("No ACK frame received!"));
#endif
    return false;
  }

  // I2C TUNING
  delay(SLOWDOWN);

  // Wait for chip to say its ready!
  if (!waitready(timeout)) {
    return false;
  }

  return true; // ack'd command
}

/**************************************************************************/
/*!
    @brief   Writes an 8-bit value that sets the state of the PN532's GPIO
             pins.
    @param   pinstate  P3 pins state.

    @warning This function is provided exclusively for board testing and
             is dangerous since it will throw an error if any pin other
             than the ones marked "Can be used as GPIO" are modified!  All
             pins that can not be used as GPIO should ALWAYS be left high
             (value = 1) or the system will become unstable and a HW reset
             will be required to recover the PN532.

             pinState[0]  = P30     Can be used as GPIO
             pinState[1]  = P31     Can be used as GPIO
             pinState[2]  = P32     *** RESERVED (Must be 1!) ***
             pinState[3]  = P33     Can be used as GPIO
             pinState[4]  = P34     *** RESERVED (Must be 1!) ***
             pinState[5]  = P35     Can be used as GPIO

    @return  1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::writeGPIO(uint8_t pinstate) {
  // uint8_t errorbit;

  // Make sure pinstate does not try to toggle P32 or P34
  pinstate |= (1 << PN532_GPIO_P32) | (1 << PN532_GPIO_P34);

  // Fill command buffer
  pn532_packetbuffer[0] = PN532_COMMAND_WRITEGPIO;
  pn532_packetbuffer[1] = PN532_GPIO_VALIDATIONBIT | pinstate; // P3 Pins
  pn532_packetbuffer[2] = 0x00; // P7 GPIO Pins (not used ... taken by SPI)

#ifdef PN532DEBUG
  PN532DEBUGPRINT.print(F("Writing P3 GPIO: "));
  PN532DEBUGPRINT.println(pn532_packetbuffer[1], HEX);
#endif

  // Send the WRITEGPIO command (0x0E)
  if (!sendCommandCheckAck(pn532_packetbuffer, 3))
    return 0x0;

  // Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0F) DATACHECKSUM
  // 00)
  readdata(pn532_packetbuffer, 8);

#ifdef PN532DEBUG
  PN532DEBUGPRINT.print(F("Received: "));
  PrintHex(pn532_packetbuffer, 8);
  PN532DEBUGPRINT.println();
#endif

  int offset = 6;
  return (pn532_packetbuffer[offset] == 0x0F);
}

/**************************************************************************/
/*!
    Reads the state of the PN532's GPIO pins

    @returns An 8-bit value containing the pin state where:

             pinState[0]  = P30
             pinState[1]  = P31
             pinState[2]  = P32
             pinState[3]  = P33
             pinState[4]  = P34
             pinState[5]  = P35
*/
/**************************************************************************/
uint8_t Adafruit_PN532::readGPIO(void) {
  pn532_packetbuffer[0] = PN532_COMMAND_READGPIO;

  // Send the READGPIO command (0x0C)
  if (!sendCommandCheckAck(pn532_packetbuffer, 1))
    return 0x0;

  // Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0D) P3 P7 IO1
  // DATACHECKSUM 00)
  readdata(pn532_packetbuffer, 11);

  /* READGPIO response should be in the following format:

    byte            Description
    -------------   ------------------------------------------
    b0..5           Frame header and preamble (with I2C there is an extra 0x00)
    b6              P3 GPIO Pins
    b7              P7 GPIO Pins (not used ... taken by SPI)
    b8              Interface Mode Pins (not used ... bus select pins)
    b9..10          checksum */

  int p3offset = 7;

#ifdef PN532DEBUG
  PN532DEBUGPRINT.print(F("Received: "));
  PrintHex(pn532_packetbuffer, 11);
  PN532DEBUGPRINT.println();
  PN532DEBUGPRINT.print(F("P3 GPIO: 0x"));
  PN532DEBUGPRINT.println(pn532_packetbuffer[p3offset], HEX);
  PN532DEBUGPRINT.print(F("P7 GPIO: 0x"));
  PN532DEBUGPRINT.println(pn532_packetbuffer[p3offset + 1], HEX);
  PN532DEBUGPRINT.print(F("IO GPIO: 0x"));
  PN532DEBUGPRINT.println(pn532_packetbuffer[p3offset + 2], HEX);
  // Note: You can use the IO GPIO value to detect the serial bus being used
  switch (pn532_packetbuffer[p3offset + 2]) {
  case 0x00: // Using UART
    PN532DEBUGPRINT.println(F("Using UART (IO = 0x00)"));
    break;
  case 0x01: // Using I2C
    PN532DEBUGPRINT.println(F("Using I2C (IO = 0x01)"));
    break;
  case 0x02: // Using SPI
    PN532DEBUGPRINT.println(F("Using SPI (IO = 0x02)"));
    break;
  }
#endif

  return pn532_packetbuffer[p3offset];
}

/**************************************************************************/
/*!
    @brief   Configures the SAM (Secure Access Module)
    @return  true on success, false otherwise.
*/
/**************************************************************************/
bool Adafruit_PN532::SAMConfig(void) {
  pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
  pn532_packetbuffer[1] = 0x01; // normal mode;
  pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
  pn532_packetbuffer[3] = 0x01; // use IRQ pin!

  if (!sendCommandCheckAck(pn532_packetbuffer, 4))
    return false;

  // read data packet
  readdata(pn532_packetbuffer, 9);

  int offset = 6;
  return (pn532_packetbuffer[offset] == 0x15);
}

/**************************************************************************/
/*!
    Sets the MxRtyPassiveActivation byte of the RFConfiguration register

    @param  maxRetries    0xFF to wait forever, 0x00..0xFE to timeout
                          after mxRetries

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::setPassiveActivationRetries(uint8_t maxRetries) {
  pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
  pn532_packetbuffer[1] = 5;    // Config item 5 (MaxRetries)
  pn532_packetbuffer[2] = 0xFF; // MxRtyATR (default = 0xFF)
  pn532_packetbuffer[3] = 0x01; // MxRtyPSL (default = 0x01)
  pn532_packetbuffer[4] = maxRetries;

#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Setting MxRtyPassiveActivation to "));
  PN532DEBUGPRINT.print(maxRetries, DEC);
  PN532DEBUGPRINT.println(F(" "));
#endif

  if (!sendCommandCheckAck(pn532_packetbuffer, 5))
    return 0x0; // no ACK

  return 1;
}

/***** ISO14443A Commands ******/

/**************************************************************************/
/*!
    @brief   Waits for an ISO14443A target to enter the field and reads
             its ID.

    @param   cardbaudrate  Baud rate of the card
    @param   uid           Pointer to the array that will be populated
                           with the card's UID (up to 7 bytes)
    @param   uidLength     Pointer to the variable that will hold the
                           length of the card's UID.
    @param   timeout       Timeout in milliseconds.

    @return  1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid,
                                         uint8_t *uidLength, uint16_t timeout) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
  pn532_packetbuffer[2] = cardbaudrate;

  if (!sendCommandCheckAck(pn532_packetbuffer, 3, timeout)) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("No card(s) read"));
#endif
    return 0x0; // no cards read
  }

  return readDetectedPassiveTargetID(uid, uidLength);
}

/**************************************************************************/
/*!
    @brief   Put the reader in detection mode, non blocking so interrupts
             must be enabled.
    @param   cardbaudrate  Baud rate of the card
    @return  1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::startPassiveTargetIDDetection(uint8_t cardbaudrate) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
  pn532_packetbuffer[2] = cardbaudrate;

  return sendCommandCheckAck(pn532_packetbuffer, 3);
}

/**************************************************************************/
/*!
    Reads the ID of the passive target the reader has deteceted.

    @param  uid           Pointer to the array that will be populated
                          with the card's UID (up to 7 bytes)
    @param  uidLength     Pointer to the variable that will hold the
                          length of the card's UID.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::readDetectedPassiveTargetID(uint8_t *uid,
                                                 uint8_t *uidLength) {
  // read data packet
  readdata(pn532_packetbuffer, 20);
  // check some basic stuff

  /* ISO14443A card response should be in the following format:

    byte            Description
    -------------   ------------------------------------------
    b0..6           Frame header and preamble
    b7              Tags Found
    b8              Tag Number (only one used in this example)
    b9..10          SENS_RES
    b11             SEL_RES
    b12             NFCID Length
    b13..NFCIDLen   NFCID                                      */

#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Found "));
  PN532DEBUGPRINT.print(pn532_packetbuffer[7], DEC);
  PN532DEBUGPRINT.println(F(" tags"));
#endif
  if (pn532_packetbuffer[7] != 1)
    return 0;

  uint16_t sens_res = pn532_packetbuffer[9];
  sens_res <<= 8;
  sens_res |= pn532_packetbuffer[10];
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("ATQA: 0x"));
  PN532DEBUGPRINT.println(sens_res, HEX);
  PN532DEBUGPRINT.print(F("SAK: 0x"));
  PN532DEBUGPRINT.println(pn532_packetbuffer[11], HEX);
#endif

  /* Card appears to be Mifare Classic */
  *uidLength = pn532_packetbuffer[12];
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("UID:"));
#endif
  for (uint8_t i = 0; i < pn532_packetbuffer[12]; i++) {
    uid[i] = pn532_packetbuffer[13 + i];
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.print(F(" 0x"));
    PN532DEBUGPRINT.print(uid[i], HEX);
#endif
  }
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.println();
#endif

  return 1;
}

/**************************************************************************/
/*!
    @brief   Exchanges an APDU with the currently inlisted peer

    @param   send            Pointer to data to send
    @param   sendLength      Length of the data to send
    @param   response        Pointer to response data
    @param   responseLength  Pointer to the response data length
    @return  true on success, false otherwise.
*/
/**************************************************************************/
bool Adafruit_PN532::inDataExchange(uint8_t *send, uint8_t sendLength,
                                    uint8_t *response,
                                    uint8_t *responseLength) {
  if (sendLength > PN532_PACKBUFFSIZ - 2) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("APDU length too long for packet buffer"));
#endif
    return false;
  }
  uint8_t i;

  pn532_packetbuffer[0] = 0x40; // PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = _inListedTag;
  for (i = 0; i < sendLength; ++i) {
    pn532_packetbuffer[i + 2] = send[i];
  }

  if (!sendCommandCheckAck(pn532_packetbuffer, sendLength + 2, 1000)) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("Could not send APDU"));
#endif
    return false;
  }

  if (!waitready(1000)) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("Response never received for APDU..."));
#endif
    return false;
  }

  readdata(pn532_packetbuffer, sizeof(pn532_packetbuffer));

  if (pn532_packetbuffer[0] == 0 && pn532_packetbuffer[1] == 0 &&
      pn532_packetbuffer[2] == 0xff) {
    uint8_t length = pn532_packetbuffer[3];
    if (pn532_packetbuffer[4] != (uint8_t)(~length + 1)) {
#ifdef PN532DEBUG
      PN532DEBUGPRINT.println(F("Length check invalid"));
      PN532DEBUGPRINT.println(length, HEX);
      PN532DEBUGPRINT.println((~length) + 1, HEX);
#endif
      return false;
    }
    if (pn532_packetbuffer[5] == PN532_PN532TOHOST &&
        pn532_packetbuffer[6] == PN532_RESPONSE_INDATAEXCHANGE) {
      if ((pn532_packetbuffer[7] & 0x3f) != 0) {
#ifdef PN532DEBUG
        PN532DEBUGPRINT.println(F("Status code indicates an error"));
#endif
        return false;
      }

      length -= 3;

      if (length > *responseLength) {
        length = *responseLength; // silent truncation...
      }

      for (i = 0; i < length; ++i) {
        response[i] = pn532_packetbuffer[8 + i];
      }
      *responseLength = length;

      return true;
    } else {
      PN532DEBUGPRINT.print(F("Don't know how to handle this command: "));
      PN532DEBUGPRINT.println(pn532_packetbuffer[6], HEX);
      return false;
    }
  } else {
    PN532DEBUGPRINT.println(F("Preamble missing"));
    return false;
  }
}

/**************************************************************************/
/*!
    @brief   'InLists' a passive target. PN532 acting as reader/initiator,
             peer acting as card/responder.
    @return  true on success, false otherwise.
*/
/**************************************************************************/
bool Adafruit_PN532::inListPassiveTarget() {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = 0;

#ifdef PN532DEBUG
  PN532DEBUGPRINT.print(F("About to inList passive target"));
#endif

  if (!sendCommandCheckAck(pn532_packetbuffer, 3, 1000)) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("Could not send inlist message"));
#endif
    return false;
  }

  if (!waitready(30000)) {
    return false;
  }

  readdata(pn532_packetbuffer, sizeof(pn532_packetbuffer));

  if (pn532_packetbuffer[0] == 0 && pn532_packetbuffer[1] == 0 &&
      pn532_packetbuffer[2] == 0xff) {
    uint8_t length = pn532_packetbuffer[3];
    if (pn532_packetbuffer[4] != (uint8_t)(~length + 1)) {
#ifdef PN532DEBUG
      PN532DEBUGPRINT.println(F("Length check invalid"));
      PN532DEBUGPRINT.println(length, HEX);
      PN532DEBUGPRINT.println((~length) + 1, HEX);
#endif
      return false;
    }
    if (pn532_packetbuffer[5] == PN532_PN532TOHOST &&
        pn532_packetbuffer[6] == PN532_RESPONSE_INLISTPASSIVETARGET) {
      if (pn532_packetbuffer[7] != 1) {
#ifdef PN532DEBUG
        PN532DEBUGPRINT.println(F("Unhandled number of targets inlisted"));
#endif
        PN532DEBUGPRINT.println(F("Number of tags inlisted:"));
        PN532DEBUGPRINT.println(pn532_packetbuffer[7]);
        return false;
      }

      _inListedTag = pn532_packetbuffer[8];
      PN532DEBUGPRINT.print(F("Tag number: "));
      PN532DEBUGPRINT.println(_inListedTag);

      return true;
    } else {
#ifdef PN532DEBUG
      PN532DEBUGPRINT.print(F("Unexpected response to inlist passive host"));
#endif
      return false;
    }
  } else {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.println(F("Preamble missing"));
#endif
    return false;
  }

  return true;
}

/***** Mifare Classic Functions ******/

/**************************************************************************/
/*!
    @brief   Indicates whether the specified block number is the first block
             in the sector (block 0 relative to the current sector)
    @param   uiBlock  Block number to test.
    @return  true if first block, false otherwise.
*/
/**************************************************************************/
bool Adafruit_PN532::mifareclassic_IsFirstBlock(uint32_t uiBlock) {
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock) % 4 == 0);
  else
    return ((uiBlock) % 16 == 0);
}

/**************************************************************************/
/*!
    @brief   Indicates whether the specified block number is the sector
             trailer.
    @param   uiBlock  Block number to test.
    @return  true if sector trailer, false otherwise.
*/
/**************************************************************************/
bool Adafruit_PN532::mifareclassic_IsTrailerBlock(uint32_t uiBlock) {
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock + 1) % 4 == 0);
  else
    return ((uiBlock + 1) % 16 == 0);
}

/**************************************************************************/
/*!
    Tries to authenticate a block of memory on a MIFARE card using the
    INDATAEXCHANGE command.  See section 7.3.8 of the PN532 User Manual
    for more information on sending MIFARE and other commands.

    @param  uid           Pointer to a byte array containing the card UID
    @param  uidLen        The length (in bytes) of the card's UID (Should
                          be 4 for MIFARE Classic)
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  keyNumber     Which key type to use during authentication
                          (0 = MIFARE_CMD_AUTH_A, 1 = MIFARE_CMD_AUTH_B)
    @param  keyData       Pointer to a byte array containing the 6 byte
                          key value

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_AuthenticateBlock(uint8_t *uid,
                                                        uint8_t uidLen,
                                                        uint32_t blockNumber,
                                                        uint8_t keyNumber,
                                                        uint8_t *keyData) {
  // uint8_t len;
  uint8_t i;

  // Hang on to the key and uid data
  memcpy(_key, keyData, 6);
  memcpy(_uid, uid, uidLen);
  _uidLen = uidLen;

#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Trying to authenticate card "));
  Adafruit_PN532::PrintHex(_uid, _uidLen);
  PN532DEBUGPRINT.print(F("Using authentication KEY "));
  PN532DEBUGPRINT.print(keyNumber ? 'B' : 'A');
  PN532DEBUGPRINT.print(F(": "));
  Adafruit_PN532::PrintHex(_key, 6);
#endif

  // Prepare the authentication command //
  pn532_packetbuffer[0] =
      PN532_COMMAND_INDATAEXCHANGE; /* Data Exchange Header */
  pn532_packetbuffer[1] = 1;        /* Max card numbers */
  pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
  pn532_packetbuffer[3] =
      blockNumber; /* Block Number (1K = 0..63, 4K = 0..255 */
  memcpy(pn532_packetbuffer + 4, _key, 6);
  for (i = 0; i < _uidLen; i++) {
    pn532_packetbuffer[10 + i] = _uid[i]; /* 4 byte card ID */
  }

  if (!sendCommandCheckAck(pn532_packetbuffer, 10 + _uidLen))
    return 0;

  // Read the response packet
  readdata(pn532_packetbuffer, 12);

  // check if the response is valid and we are authenticated???
  // for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
  // Mifare auth error is technically byte 7: 0x14 but anything other and 0x00
  // is not good
  if (pn532_packetbuffer[7] != 0x00) {
#ifdef PN532DEBUG
    PN532DEBUGPRINT.print(F("Authentification failed: "));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 12);
#endif
    return 0;
  }

  return 1;
}

/**************************************************************************/
/*!
    Tries to read an entire 16-byte data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          Pointer to the byte array that will hold the
                          retrieved data (if any)

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_ReadDataBlock(uint8_t blockNumber,
                                                    uint8_t *data) {
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Trying to read 16 bytes from block "));
  PN532DEBUGPRINT.println(blockNumber);
#endif

  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;               /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
  pn532_packetbuffer[3] =
      blockNumber; /* Block Number (0..63 for 1K, 0..255 for 4K) */

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 4)) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for read command"));
#endif
    return 0;
  }

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);

  /* If byte 8 isn't 0x00 we probably have an error */
  if (pn532_packetbuffer[7] != 0x00) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Unexpected response"));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif
    return 0;
  }

  /* Copy the 16 data bytes to the output buffer        */
  /* Block content starts at byte 9 of a valid response */
  memcpy(data, pn532_packetbuffer + 8, 16);

/* Display data for debug if requested */
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Block "));
  PN532DEBUGPRINT.println(blockNumber);
  Adafruit_PN532::PrintHexChar(data, 16);
#endif

  return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 16-byte data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          The byte array that contains the data to write.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_WriteDataBlock(uint8_t blockNumber,
                                                     uint8_t *data) {
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Trying to write 16 bytes to block "));
  PN532DEBUGPRINT.println(blockNumber);
#endif

  /* Prepare the first command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_WRITE; /* Mifare Write command = 0xA0 */
  pn532_packetbuffer[3] =
      blockNumber; /* Block Number (0..63 for 1K, 0..255 for 4K) */
  memcpy(pn532_packetbuffer + 4, data, 16); /* Data Payload */

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 20)) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  delay(10);

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);

  return 1;
}

/**************************************************************************/
/*!
    Formats a Mifare Classic card to store NDEF Records

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_FormatNDEF(void) {
  uint8_t sectorbuffer1[16] = {0x14, 0x01, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1,
                               0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
  uint8_t sectorbuffer2[16] = {0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1,
                               0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
  uint8_t sectorbuffer3[16] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0x78, 0x77,
                               0x88, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // Note 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 must be used for key A
  // for the MAD sector in NDEF records (sector 0)

  // Write block 1 and 2 to the card
  if (!(mifareclassic_WriteDataBlock(1, sectorbuffer1)))
    return 0;
  if (!(mifareclassic_WriteDataBlock(2, sectorbuffer2)))
    return 0;
  // Write key A and access rights card
  if (!(mifareclassic_WriteDataBlock(3, sectorbuffer3)))
    return 0;

  // Seems that everything was OK (?!)
  return 1;
}

/**************************************************************************/
/*!
    Writes an NDEF URI Record to the specified sector (1..15)

    Note that this function assumes that the Mifare Classic card is
    already formatted to work as an "NFC Forum Tag" and uses a MAD1
    file system.  You can use the NXP TagWriter app on Android to
    properly format cards for this.

    @param  sectorNumber  The sector that the URI record should be written
                          to (can be 1..15 for a 1K card)
    @param  uriIdentifier The uri identifier code (0 = none, 0x01 =
                          "http://www.", etc.)
    @param  url           The uri text to write (max 38 characters).

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_WriteNDEFURI(uint8_t sectorNumber,
                                                   uint8_t uriIdentifier,
                                                   const char *url) {
  // Figure out how long the string is
  uint8_t len = strlen(url);

  // Make sure we're within a 1K limit for the sector number
  if ((sectorNumber < 1) || (sectorNumber > 15))
    return 0;

  // Make sure the URI payload is between 1 and 38 chars
  if ((len < 1) || (len > 38))
    return 0;

  // Note 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 must be used for key A
  // in NDEF records

  // Setup the sector buffer (w/pre-formatted TLV wrapper and NDEF message)
  uint8_t sectorbuffer1[16] = {0x00,
                               0x00,
                               0x03,
                               (uint8_t)(len + 5),
                               0xD1,
                               0x01,
                               (uint8_t)(len + 1),
                               0x55,
                               uriIdentifier,
                               0x00,
                               0x00,
                               0x00,
                               0x00,
                               0x00,
                               0x00,
                               0x00};
  uint8_t sectorbuffer2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t sectorbuffer3[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t sectorbuffer4[16] = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7, 0x7F, 0x07,
                               0x88, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  if (len <= 6) {
    // Unlikely we'll get a url this short, but why not ...
    memcpy(sectorbuffer1 + 9, url, len);
    sectorbuffer1[len + 9] = 0xFE;
  } else if (len == 7) {
    // 0xFE needs to be wrapped around to next block
    memcpy(sectorbuffer1 + 9, url, len);
    sectorbuffer2[0] = 0xFE;
  } else if ((len > 7) && (len <= 22)) {
    // Url fits in two blocks
    memcpy(sectorbuffer1 + 9, url, 7);
    memcpy(sectorbuffer2, url + 7, len - 7);
    sectorbuffer2[len - 7] = 0xFE;
  } else if (len == 23) {
    // 0xFE needs to be wrapped around to final block
    memcpy(sectorbuffer1 + 9, url, 7);
    memcpy(sectorbuffer2, url + 7, len - 7);
    sectorbuffer3[0] = 0xFE;
  } else {
    // Url fits in three blocks
    memcpy(sectorbuffer1 + 9, url, 7);
    memcpy(sectorbuffer2, url + 7, 16);
    memcpy(sectorbuffer3, url + 23, len - 24);
    sectorbuffer3[len - 22] = 0xFE;
  }

  // Now write all three blocks back to the card
  if (!(mifareclassic_WriteDataBlock(sectorNumber * 4, sectorbuffer1)))
    return 0;
  if (!(mifareclassic_WriteDataBlock((sectorNumber * 4) + 1, sectorbuffer2)))
    return 0;
  if (!(mifareclassic_WriteDataBlock((sectorNumber * 4) + 2, sectorbuffer3)))
    return 0;
  if (!(mifareclassic_WriteDataBlock((sectorNumber * 4) + 3, sectorbuffer4)))
    return 0;

  // Seems that everything was OK (?!)
  return 1;
}

/***** Mifare Ultralight Functions ******/

/**************************************************************************/
/*!
    @brief   Tries to read an entire 4-byte page at the specified address.

    @param   page        The page number (0..63 in most cases)
    @param   buffer      Pointer to the byte array that will hold the
                         retrieved data (if any)
    @return  1 on success, 0 on error.
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareultralight_ReadPage(uint8_t page,
                                                  uint8_t *buffer) {
  if (page >= 64) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Page value out of range"));
#endif
    return 0;
  }

#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Reading page "));
  PN532DEBUGPRINT.println(page);
#endif

  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;               /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
  pn532_packetbuffer[3] = page; /* Page Number (0..63 in most cases) */

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 4)) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.println(F("Received: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif

  /* If byte 8 isn't 0x00 we probably have an error */
  if (pn532_packetbuffer[7] == 0x00) {
    /* Copy the 4 data bytes to the output buffer         */
    /* Block content starts at byte 9 of a valid response */
    /* Note that the command actually reads 16 byte or 4  */
    /* pages at a time ... we simply discard the last 12  */
    /* bytes                                              */
    memcpy(buffer, pn532_packetbuffer + 8, 4);
  } else {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Unexpected response reading block: "));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif
    return 0;
  }

/* Display data for debug if requested */
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Page "));
  PN532DEBUGPRINT.print(page);
  PN532DEBUGPRINT.println(F(":"));
  Adafruit_PN532::PrintHexChar(buffer, 4);
#endif

  // Return OK signal
  return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 4-byte page at the specified block
    address.

    @param  page          The page number to write.  (0..63 for most cases)
    @param  data          The byte array that contains the data to write.
                          Should be exactly 4 bytes long.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareultralight_WritePage(uint8_t page,
                                                   uint8_t *data) {

  if (page >= 64) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Page value out of range"));
#endif
    // Return Failed Signal
    return 0;
  }

#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Trying to write 4 byte page"));
  PN532DEBUGPRINT.println(page);
#endif

  /* Prepare the first command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] =
      MIFARE_ULTRALIGHT_CMD_WRITE; /* Mifare Ultralight Write command = 0xA2 */
  pn532_packetbuffer[3] = page;    /* Page Number (0..63 for most cases) */
  memcpy(pn532_packetbuffer + 4, data, 4); /* Data Payload */

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 8)) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif

    // Return Failed Signal
    return 0;
  }
  delay(10);

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);

  // Return OK Signal
  return 1;
}
/***** NTAG424 Functions ******/

/**************************************************************************/
/*!
    @brief   create bytecount random bytes into output.

    @param   output     buffer to generate randomness in
    @param   bytecount  amount of bytes randomness to create in buffer

    @return
*/
/**************************************************************************/
void Adafruit_PN532::ntag424_random(uint8_t *output, uint8_t bytecount) {
  for (int i = 0; i < bytecount; i++) {
    output[i] = random(256);
  }
}

/**************************************************************************/
/*!
    @brief   send an apdu-frame to the picc, and wait for a response.

    @param *cla                   CLA/ISO prefix
    @param *ins                   Instruction or command
    @param *p1                    Parameter 1
    @param *p2                    Parameter 2
    @param *cmd_header            command header
    @param cmd_header_length      length of command_header
    @param *cmd_data              command data
    @param cmd_data_length        length of command data
    @param le                     TODO: check if needed.
    @param comm_mode              Communication mode: NTAG424_COMM_MODE_PLAIN,
   NTAG424_COMM_MODE_MAC or NTAG424_COMM_MODE_FULL
    @param *response              response buffer
    @param response_le            size of response buffer
    @return length of response
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_apdu_send(
    uint8_t *cla, uint8_t *ins, uint8_t *p1, uint8_t *p2, uint8_t *cmd_header,
    uint8_t cmd_header_length, uint8_t *cmd_data, uint8_t cmd_data_length,
    uint8_t le, uint8_t comm_mode, uint8_t *response, uint8_t response_le) {
  Serial.print("cmd_counter: ");
  Serial.println(ntag424_Session.cmd_counter);
  uint8_t apdusize = 8 + (7 + cmd_header_length + cmd_data_length + 2) & 0xff;
  uint8_t apdu[apdusize];
  uint8_t offset = 0;
  apdu[0] = PN532_COMMAND_INDATAEXCHANGE;
  apdu[1] = 0x01;
  apdu[2] = cla[0];
  apdu[3] = ins[0];
  apdu[4] = p1[0];
  apdu[5] = p2[0];
  apdu[6] = cmd_data_length + cmd_header_length;
  uint8_t offset_lc = 6;
  offset = 7;
  // apdu[4] = cmd_data_length + cmd_header_length;
  memcpy(apdu + offset, cmd_header, cmd_header_length);
  offset += cmd_header_length;

  if (comm_mode == NTAG424_COMM_MODE_PLAIN) {
    // we are done
    memcpy(apdu + offset, cmd_data, cmd_data_length);
    offset += cmd_data_length;
  } else if (comm_mode == NTAG424_COMM_MODE_MAC) {
    memcpy(apdu + offset, cmd_data, cmd_data_length);
    offset += cmd_data_length;
    uint8_t cmac_short[8];
    Adafruit_PN532::ntag424_MAC(ins, cmd_header, cmd_header_length, cmd_data,
                                cmd_data_length, cmac_short);
#ifdef NTAG424DEBUG
    Serial.println("CMAC NEW:");
    Adafruit_PN532::PrintHexChar(cmac_short, 8);
#endif
    memcpy(apdu + offset, cmac_short, 8);
    offset += 8;
    apdu[offset_lc] += 8;

  } else if (comm_mode == NTAG424_COMM_MODE_FULL) {
#ifdef NTAG424DEBUG
    Serial.println("ENC NEW:");
    Serial.println("APDU UNENC:");
    Adafruit_PN532::PrintHexChar(apdu, offset);
#endif
    uint8_t cmac_short[8];
    uint8_t padded_payload_length = 16 + cmd_data_length;
    uint8_t payload_padded[padded_payload_length];
    if (cmd_data_length > 0) {
      // Add padding to the cmddata
      memcpy(payload_padded, cmd_data, cmd_data_length);
      padded_payload_length = Adafruit_PN532::ntag424_addpadding(
          cmd_data_length, 16, payload_padded);
#ifdef NTAG424DEBUG
      Serial.print("CMDDATA Length:");
      Serial.println(cmd_data_length);
      Adafruit_PN532::PrintHexChar(payload_padded, cmd_data_length);
      Serial.println("CMDDATA Padded:");
      Serial.println(padded_payload_length);
      Adafruit_PN532::PrintHexChar(payload_padded, padded_payload_length);
#endif
      // assemble iv
      uint8_t iv[32];
      uint8_t ive[16];
      iv[0] = 0xA5;
      iv[1] = 0x5A;
      memcpy(iv + 2, ntag424_authresponse_TI, 4);
      iv[6] = ntag424_Session.cmd_counter & 0xff;
      iv[7] = (ntag424_Session.cmd_counter >> 8) & 0xff;
      memset(iv + 7, 0, 24); // was 25
#ifdef NTAG424DEBUG
      Serial.println("IV-init:");
      Adafruit_PN532::PrintHex(iv, 16);
#endif
      Adafruit_PN532::ntag424_encrypt(ntag424_Session.session_key_enc,
                                      sizeof(iv), iv, ive);
      // encrypt cmd_data using SesAuthENCKey
      // padded_payload_length
      //uint8_t payload_encrypted[32];
      uint8_t payload_encrypted[52];
      Adafruit_PN532::ntag424_encrypt(ntag424_Session.session_key_enc, ive,
                                      padded_payload_length, payload_padded,
                                      payload_encrypted);
      memcpy(apdu + offset, payload_encrypted, padded_payload_length);
#ifdef NTAG424DEBUG
      Serial.println("APDU Payload:");
      Adafruit_PN532::PrintHexChar(apdu, offset);
      Serial.println("CMDDATA ENC:");
      Serial.println(padded_payload_length);
      Serial.println("CMD:");
      Serial.println(ins[0], HEX);
      Adafruit_PN532::PrintHex(payload_encrypted, padded_payload_length);
#endif
      offset += padded_payload_length;
#ifdef NTAG424DEBUG
      Serial.println("APDU PREMAC:");
      Adafruit_PN532::PrintHexChar(apdu, offset);
#endif
      // add CMAC
      Adafruit_PN532::ntag424_MAC(
          ntag424_Session.session_key_mac, ins, cmd_header, cmd_header_length,
          payload_encrypted, padded_payload_length, cmac_short);
      memcpy(apdu + offset, cmac_short, 8);
      offset += 8;
      apdu[offset_lc] = cmd_header_length + padded_payload_length + 8;
#ifdef NTAG424DEBUG
      Serial.println("APDU AFTERMAC:");
      Adafruit_PN532::PrintHexChar(apdu, offset);
#endif
    } else {
      Adafruit_PN532::ntag424_MAC(ntag424_Session.session_key_mac, ins,
                                  cmd_header, cmd_header_length, cmd_data,
                                  cmd_data_length, cmac_short);
      memcpy(apdu + offset, cmac_short, 8);
      offset += 8;
      apdu[offset_lc] += 8;
#ifdef NTAG424DEBUG
      Serial.println("APDU AFTERMAC:");
      Adafruit_PN532::PrintHexChar(apdu, offset);
#endif
    }
#ifdef NTAG424DEBUG
    Serial.println(offset);
    Serial.println("APDU ENC:");
    Adafruit_PN532::PrintHexChar(apdu, offset);
#endif
  }
  if (apdu[3] != NTAG424_CMD_ISOUPDATEBINARY) {
    apdu[offset] = le;
    offset++;
  }
  apdusize = offset;
  //#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("PCD->PICC:"));
  Adafruit_PN532::PrintHexChar(apdu + 2, apdusize - 2);
  //#endif
  if (!sendCommandCheckAck((uint8_t *)apdu, apdusize)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  /* Read the response packet */
  // readdata(pn532_packetbuffer, 41);
  readdata(pn532_packetbuffer, response_le);
  //#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("PCD<-PICC: "));
  // Adafruit_PN532::PrintHexChar(pn532_packetbuffer + 8, 5 +
  // pn532_packetbuffer[3] - 8);
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 5 + pn532_packetbuffer[3]);
  //#endif
  // increase cmd_counter
  ntag424_Session.cmd_counter += 1;

  uint8_t response_length = pn532_packetbuffer[3] - 3;
  memcpy(response, pn532_packetbuffer + 8, response_length);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("RESPONSE: "));
  Adafruit_PN532::PrintHexChar(response, response_length);
#endif
  uint8_t resp_cmac_ok = 0;
  // check the responsemac if there is a MAC
  if ((response_length >= 10) && ((comm_mode == NTAG424_COMM_MODE_FULL) ||
                                  (comm_mode == NTAG424_COMM_MODE_MAC))) {
    uint8_t respcmac[8];
    memcpy(respcmac, response + (response_length - 10), 8);
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.print(F("response cmac:"));
    Adafruit_PN532::PrintHex(respcmac, 8);
#endif

    uint8_t *checkmacin = (uint8_t *)malloc(response_length + 6);
    uint8_t maclength = 0;
    checkmacin[0] = response[response_length - 1];
    checkmacin[1] = ntag424_Session.cmd_counter & 0xff;
    checkmacin[2] = (ntag424_Session.cmd_counter >> 8) & 0xff;
    memcpy(checkmacin + 3, ntag424_authresponse_TI,
           NTAG424_AUTHRESPONSE_TI_SIZE);
    uint8_t padded_respdata_length = 0;
    uint8_t *respdata = (uint8_t *)malloc(response_length - 10);
    if (response_length > 10) {
      memcpy(respdata, response, response_length - 10);
      // padded_respdata_length =
      // Adafruit_PN532::ntag424_addpadding(response_length - 10 ,16, respdata);
      padded_respdata_length = response_length - 10;
      memcpy(checkmacin + 3 + NTAG424_AUTHRESPONSE_TI_SIZE, respdata,
             padded_respdata_length);
    }
    maclength = 3 + NTAG424_AUTHRESPONSE_TI_SIZE + padded_respdata_length;
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.print(F("checkcmac input: "));
    Adafruit_PN532::PrintHex(checkmacin, maclength);
#endif
    uint8_t checkmac[8];

    Adafruit_PN532::ntag424_cmac_short(ntag424_Session.session_key_mac,
                                       checkmacin, maclength, checkmac);
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.print(F("checkcmac:"));
    Adafruit_PN532::PrintHex(checkmac, 8);
#endif
    free(respdata);
    free(checkmacin);
    for (int i = 0; i < 8; i++) {
      if (respcmac[i] != checkmac[i]) {
#ifdef NTAG424DEBUG
        PN532DEBUGPRINT.println(
            F("Response CMAC integrity error! (picc <> pcd)"));
        Adafruit_PN532::PrintHex(respcmac, 8);
        PN532DEBUGPRINT.print(F(" <> "));
        Adafruit_PN532::PrintHex(checkmac, 8);
#endif
        return 0;
      }
    }
    PN532DEBUGPRINT.println(F("Response CMAC ok! (picc == pcd)"));
  }
  // decrypt the response in mode.full
  if ((response_length >= 10) && (comm_mode == NTAG424_COMM_MODE_FULL)) {
    uint8_t ivd[32];
    uint8_t ivde[16];
    ivd[0] = 0x5A;
    ivd[1] = 0xA5;
    memcpy(ivd + 2, ntag424_authresponse_TI, 4);
    ivd[6] = ntag424_Session.cmd_counter & 0xff;
    ivd[7] = (ntag424_Session.cmd_counter >> 8) & 0xff;
    memset(ivd + 7, 0, 25);
    // Serial.println("IV-init:");
    // Adafruit_PN532::PrintHex(iv, 16);
    Adafruit_PN532::ntag424_encrypt(ntag424_Session.session_key_enc,
                                    sizeof(ivd), ivd, ivde);
    uint8_t *respplain = (uint8_t *)malloc(response_length - 10);
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Encrypted Response(pcd < picc)"));
    Adafruit_PN532::PrintHex(response, response_length - 10);
#endif
    Adafruit_PN532::ntag424_decrypt(ntag424_Session.session_key_enc, ivde,
                                    response_length - 10, response, respplain);
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Decrypted Response(pcd < picc)"));
    Adafruit_PN532::PrintHex(respplain, response_length - 10);
#endif
    // 10 byte = cmac+responsecode
    // Serial.println(response_length);
    memcpy(response, respplain, response_length - 10);
    uint8_t resp_no_padding = response_length - 10;
    if (response_length > 10) {
      for (uint8_t i = response_length - 10 - 1; i >= 0; i--) {
        // Serial.println(i);
        if (response[i] == 0x00) {
          resp_no_padding = i;
        } else if (response[i] == 0x80) {
          resp_no_padding = i;
          break;
        } else {
          // nopadding?
          break;
        }
      }
    }
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(resp_no_padding);
#endif
    free(respplain);
    memcpy(response + resp_no_padding, response + response_length - 2, 2);
    resp_no_padding += 2;
    memset(response + resp_no_padding, 0, response_length - resp_no_padding);
    response_length = resp_no_padding;
  }
  return response_length;
}

/**************************************************************************/
/*!
    @brief   add padding to a buffer.

    @param   inputlength
    @param   paddinglength
    @param   buffer input & outputbuffer needs to be big enough for inputlength
   + padding worstcase

    @return  the new length of buffer including the added zeroes
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_addpadding(uint8_t inputlength,
                                           uint8_t paddinglength,
                                           uint8_t *buffer) {
  uint8_t zeroestoadd = paddinglength - (inputlength % paddinglength);
  memset(buffer + inputlength, 0, zeroestoadd);
  if (zeroestoadd > 0) {
    buffer[inputlength] = 0x80;
  }
  return inputlength + zeroestoadd;
}

/**************************************************************************/
/*!
    @brief   wrapper for ntag424_encrypt with standard iv.

    @param   key    encryption key
    @param   length  sizeof input
    @param   input inputbuffer
    @param   output outputbuffer

    @return
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_encrypt(uint8_t *key, uint8_t length,
                                        uint8_t *input, uint8_t *output) {
  uint8_t iv[16];
  memset(iv, 0, sizeof(iv));
  return ntag424_encrypt(key, iv, length, input, output);
}

/**************************************************************************/
/*!
    @brief   encrypt input's lengths bytes with aes 128 cbc into output.

    @param   key    encryption key
    @param   iv     initialization vector
    @param   length sizeof input
    @param   input  inputbuffer
    @param   output outputbuffer

    @return
*/
/**************************************************************************/
// encrypt input's lengths bytes with aes 128 cbc into output
uint8_t Adafruit_PN532::ntag424_encrypt(uint8_t *key, uint8_t *iv,
                                        uint8_t length, uint8_t *input,
                                        uint8_t *output) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  // Set the key for the AES context
  if (mbedtls_aes_setkey_dec(&ctx, key, 128) != 0) {
    // Error setting key
    mbedtls_aes_free(&ctx);
    return 0;
  }
  mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, length, iv, (uint8_t *)input,
                        (uint8_t *)output);
  mbedtls_aes_free(&ctx);
  return 1;
}

/**************************************************************************/
/*!
    @brief   wrapper for ntag424_decrypt with standard iv.

    @param   key    encryption key
    @param   length  sizeof input
    @param   input inputbuffer
    @param   output outputbuffer

    @return
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_decrypt(uint8_t *key, uint8_t length,
                                        uint8_t *input, uint8_t *output) {
  uint8_t iv[16];
  memset(iv, 0, sizeof(iv));
  return ntag424_decrypt(key, iv, length, input, output);
}

/**************************************************************************/
/*!
    @brief   decrypt input's lengths bytes with aes 128 cbc into output.

    @param   key    encryption key
    @param   iv     initialization vector
    @param   length sizeof input
    @param   input  inputbuffer
    @param   output outputbuffer

    @return
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_decrypt(uint8_t *key, uint8_t *iv,
                                        uint8_t length, uint8_t *input,
                                        uint8_t *output) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  // Set the key for the AES context
  if (mbedtls_aes_setkey_dec(&ctx, key, 128) != 0) {
    // Error setting key
    mbedtls_aes_free(&ctx);
    return 0;
  }
  if (mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, length, iv,
                            (uint8_t *)input, (uint8_t *)output) != 0) {
    return 0;
  }
  mbedtls_aes_free(&ctx);
  return 1;
}

/**************************************************************************/
/*!
    @brief   create short cmac by returning the uneven bytes (1,3,5,7,9).

    @param   key    signing key
    @param   input  inputbuffer
    @param   length length of inputbuffer
    @param   cmac   outputbuffer (>=8 bytes)

    @return
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_cmac_short(uint8_t *key, uint8_t *input,
                                           uint8_t length, uint8_t *cmac) {
  uint8_t regularcmac[16];
  Adafruit_PN532::ntag424_cmac(key, input, length, regularcmac);
  uint8_t c = 0;
  for (int i = 1; i < 16; i += 2) {
    cmac[c] = regularcmac[i];
    c++;
  }

#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("INPUT: "));
  Adafruit_PN532::PrintHexChar(input, length);
  PN532DEBUGPRINT.print(F("CMAC: "));
  Adafruit_PN532::PrintHexChar(regularcmac, 16);
  PN532DEBUGPRINT.print(F("CMAC_SHORT: "));
  Adafruit_PN532::PrintHexChar(cmac, 8);
#endif
  return 0;
}

/**************************************************************************/
/*!
    @brief   create aes128-cmac.

    @param   key    signing key
    @param   input  inputbuffer
    @param   length length of inputbuffer
    @param   cmac   outputbuffer (>=16 bytes)

    @return
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_cmac(uint8_t *key, uint8_t *input,
                                     uint8_t length, uint8_t *cmac) {
  int ret = 0;
  const mbedtls_cipher_info_t *cipher_info;
  cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
  mbedtls_cipher_context_t ctx;
  mbedtls_cipher_init(&ctx);
  if ((ret = mbedtls_cipher_setup(&ctx, cipher_info)) != 0) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("could not setup cipher "));
#endif
    goto exit;
  }
  ret = mbedtls_cipher_cmac_starts(&ctx, key, 128);
  if (ret != 0) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("could not start cmac "));
#endif
    goto exit;
  }
  ret = mbedtls_cipher_cmac_update(&ctx, input, length);
  if (ret != 0) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("error while updateing cmac "));
#endif
    goto exit;
  }
  ret = mbedtls_cipher_cmac_finish(&ctx, cmac);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("cmac key: "));
  Adafruit_PN532::PrintHexChar(key, 16);
  PN532DEBUGPRINT.print(F("cmac input: "));
  Adafruit_PN532::PrintHexChar(input, length);
  PN532DEBUGPRINT.print(F("cmac output: "));
  Adafruit_PN532::PrintHexChar(cmac, 16);
#endif
  return 1;
exit:
  mbedtls_cipher_free(&ctx);
  return 0;
}

/**************************************************************************/
/*!
    @brief   sign the supplied data and return the size.

    @param   cmd              apducmd
    @param   cmdheader        buffer containing the commandheader
    @param   cmdheader_length length of commandheader
    @param   cmddata          buffer containing the command data
    @param   cmddata_length   length of commanddata. set to 0 if n/a
    @param   signature        outputbuffer for the signature

    @return
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_MAC(uint8_t *cmd, uint8_t *cmdheader,
                                    uint8_t cmdheader_length, uint8_t *cmddata,
                                    uint8_t cmddata_length,
                                    uint8_t *signature) {
  return ntag424_MAC(ntag424_Session.session_key_mac, cmd, cmdheader,
                     cmdheader_length, cmddata, cmddata_length, signature);
}

/**************************************************************************/
/*!
    @brief   sign the supplied data.

    @param   key              mac-key
    @param   cmd              apducmd
    @param   cmdheader        buffer containing the commandheader
    @param   cmdheader_length length of commandheader
    @param   cmddata          buffer containing the command data
    @param   cmddata_length   length of commanddata. set to 0 if n/a
    @param   signature        outputbuffer for the signature

    @return
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_MAC(uint8_t *key, uint8_t *cmd,
                                    uint8_t *cmdheader,
                                    uint8_t cmdheader_length, uint8_t *cmddata,
                                    uint8_t cmddata_length,
                                    uint8_t *signature) {
  // counter is LSB
  uint8_t counter[2] = {(uint8_t) (ntag424_Session.cmd_counter & 0xff),
                        (uint8_t) ((ntag424_Session.cmd_counter >> 8) & 0xff)};
  uint8_t msglen = 1 + sizeof(counter) + NTAG424_AUTHRESPONSE_TI_SIZE +
                   cmdheader_length + cmddata_length;
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("mesglen: "));
  Serial.println(msglen);
#endif
  uint8_t mesg[msglen];
  uint8_t cmac_short[8];

  mesg[0] = cmd[0];
  memcpy(mesg + 1, counter, sizeof(counter));
  memcpy(mesg + 1 + sizeof(counter), ntag424_authresponse_TI,
         NTAG424_AUTHRESPONSE_TI_SIZE);
  memcpy(mesg + 1 + sizeof(counter) + NTAG424_AUTHRESPONSE_TI_SIZE, cmdheader,
         cmdheader_length);
  if (cmddata_length > 0) {
    memcpy(mesg + 1 + sizeof(counter) + NTAG424_AUTHRESPONSE_TI_SIZE +
               cmdheader_length,
           cmddata, cmddata_length);
  }
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("mesg: padded: "));
  Adafruit_PN532::PrintHexChar(mesg, msglen);
#endif
  Adafruit_PN532::ntag424_cmac_short(key, mesg, msglen, signature);
  return 0;
}

/**************************************************************************/
/*!
    @brief   derive sessionskeys from RndA and RNDB.

    @param   key         mac-key
    @param   RndA        RndA
    @param   RndB        RndB

    @return
*/
/**************************************************************************/
void Adafruit_PN532::ntag424_derive_session_keys(uint8_t *key, uint8_t *RndA,
                                                 uint8_t *RndB) {
  uint8_t f1[2];
  uint8_t f2[6];
  uint8_t f3[6];
  uint8_t f4[10];
  uint8_t f5[8];
  uint8_t f2xor3[6];
  memcpy(&f1, RndA, 2);
  memcpy(&f2, RndA + 2, 6);
  memcpy(&f3, RndB, 6);
  memcpy(&f4, RndB + 6, 10);
  memcpy(&f5, RndA + 8, 8);
  // xor f2 & f3
  for (int i = 0; i < 6; ++i) {
    f2xor3[i] = f2[i] ^ f3[i];
  }
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("DERIVE SESSIONKEYS: "));
  PN532DEBUGPRINT.print(F("RndA: "));
  Adafruit_PN532::PrintHexChar(RndA, 16);
  PN532DEBUGPRINT.print(F("RndB: "));
  Adafruit_PN532::PrintHexChar(RndB, 16);
  PN532DEBUGPRINT.print(F("f1: "));
  Adafruit_PN532::PrintHexChar(f1, 2);
  PN532DEBUGPRINT.print(F("f2: "));
  Adafruit_PN532::PrintHexChar(f2, 6);
  PN532DEBUGPRINT.print(F("f3: "));
  Adafruit_PN532::PrintHexChar(f3, 6);
  PN532DEBUGPRINT.print(F("f4: "));
  Adafruit_PN532::PrintHexChar(f4, 10);
  PN532DEBUGPRINT.print(F("f5: "));
  Adafruit_PN532::PrintHexChar(f5, 8);
  PN532DEBUGPRINT.print(F("f2xorf3: "));
  Adafruit_PN532::PrintHexChar(f2xor3, 6);
#endif
  uint8_t sv1[32] = {
      0xA5,  0x5A,      0x00,      0x01,      0x00,      0x80,      f1[0],
      f1[1], f2xor3[0], f2xor3[1], f2xor3[2], f2xor3[3], f2xor3[4], f2xor3[5],
      f4[0], f4[1],     f4[2],     f4[3],     f4[4],     f4[5],     f4[6],
      f4[7], f4[8],     f4[9],     f5[0],     f5[1],     f5[2],     f5[3],
      f5[4], f5[5],     f5[6],     f5[7]};
  uint8_t sv2[32] = {
      0x5A,  0xA5,      0x00,      0x01,      0x00,      0x80,      f1[0],
      f1[1], f2xor3[0], f2xor3[1], f2xor3[2], f2xor3[3], f2xor3[4], f2xor3[5],
      f4[0], f4[1],     f4[2],     f4[3],     f4[4],     f4[5],     f4[6],
      f4[7], f4[8],     f4[9],     f5[0],     f5[1],     f5[2],     f5[3],
      f5[4], f5[5],     f5[6],     f5[7]};
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("SV1: "));
  Adafruit_PN532::PrintHexChar(sv1, 32);
  PN532DEBUGPRINT.print(F("SV2: "));
  Adafruit_PN532::PrintHexChar(sv2, 32);
#endif

  Adafruit_PN532::ntag424_cmac(key, sv1, 32, ntag424_Session.session_key_enc);
  Adafruit_PN532::ntag424_cmac(key, sv2, 32, ntag424_Session.session_key_mac);

#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("session_key_mac: "));
  Adafruit_PN532::PrintHexChar(ntag424_Session.session_key_mac,
                               NTAG424_SESSION_KEYSIZE);
  PN532DEBUGPRINT.print(F("session_key_enc: "));
  Adafruit_PN532::PrintHexChar(ntag424_Session.session_key_enc,
                               NTAG424_SESSION_KEYSIZE);
#endif
}

/**************************************************************************/
/*!
    @brief   calculate and return the crc32 of data.

    @param   data      databuffer needs to be of size datalength
    @param   datalength

    @return  uint32 crc
*/
/**************************************************************************/

uint32_t Adafruit_PN532::ntag424_crc32(uint8_t *data, uint8_t datalength) {
  Adafruit_PN532::PrintHexChar((uint8_t const *)data, datalength);
  uint32_t const crc32_res = crc32.calc((uint8_t const *)data, datalength);
  Serial.println(crc32_res, HEX);
  return crc32_res;
}

/**************************************************************************/
/*!
    @brief   left-rotate the bufferlen bytes of input by rotation bytes into
   output.

    @param   input      inputbuffer needs to be of size bufferlen
    @param   output     outputbuffer needs to be of size bufferlen
    @param   bufferlen  size of input & output-buffer
    @param   rotation     number of random bytes to rotate

    @return
*/
/**************************************************************************/

uint8_t Adafruit_PN532::ntag424_rotl(uint8_t *input, uint8_t *output,
                                     uint8_t bufferlen, uint8_t rotation) {
  uint8_t overlap[16];
  if ((rotation > 16) || (bufferlen < rotation)) {
    // no overflow
    PN532DEBUGPRINT.print(F("rotation-error: overflow or negative rotation"));
    return 0;
  }

#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("rotation-before: "));
  Adafruit_PN532::PrintHexChar(input, bufferlen);
#endif

  for (int i = 0; i < bufferlen; i++) {
    int z = i - rotation;
    if (z < 0) {
      z = bufferlen + z;
    }
    output[z] = input[i];
  }
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("rotation-after: "));
  Adafruit_PN532::PrintHexChar(output, bufferlen);
#endif
  return 1;
}

/*!
    @brief   Authenticate to start encrypted or signed communication.

    @param   key      encryption key
    @param   keyno   number of key to authenticate against (0-4)
    @param   cmd      0x71 or 0x77

    @return  1 = success; 0 = failed
*/
/**************************************************************************/

uint8_t Adafruit_PN532::ntag424_Authenticate(uint8_t *key, uint8_t keyno,
                                             uint8_t cmd) {

#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("Authenticating with key: "));
  PN532DEBUGPRINT.println((char *)key);
#endif

// 1.) IsoSelectFile
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("1.) ISOSelectFile"));
#endif
  int cmd_len = 15;
  uint8_t cmd_select[cmd_len] = {PN532_COMMAND_INDATAEXCHANGE,
                                 0x01,
                                 0x00,
                                 0xA4,
                                 0x04,
                                 0x00,
                                 0x07,
                                 0xD2,
                                 0x76,
                                 0x00,
                                 0x00,
                                 0x85,
                                 0x01,
                                 0x01,
                                 0x00};
  /* Prepare the command */
  /* Send the command */
  if (!sendCommandCheckAck((uint8_t *)cmd_select, cmd_len)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("CMD: "));
  Adafruit_PN532::PrintHexChar(cmd_select, cmd_len);
  PN532DEBUGPRINT.println(strlen((char *)cmd_select));
  PN532DEBUGPRINT.print(F("Received: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif

  /* If byte 8 isn't 0x00 we probably have an error, also byte 8 & 9 should be
   * 0x9000 */
  if (pn532_packetbuffer[7] != 0x00 || pn532_packetbuffer[8] != 0x90 ||
      pn532_packetbuffer[9] != 0x00) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("ISOSelectFile ResultError"));
#endif
    return 0;
  }

// 2.) AuthenticateFirst part 1
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("2.) AuthenticateFirst part 1"));
#endif
  cmd_len = 13;
  uint8_t cmd_auth1[cmd_len] = {PN532_COMMAND_INDATAEXCHANGE,
                                0x01,
                                0x90,
                                cmd,
                                0x00,
                                0x00,
                                0x05,
                                keyno,
                                0x03,
                                0x00,
                                0x00,
                                0x00,
                                0x00};
  /* Prepare the command */
  /* Send the command */
  if (!sendCommandCheckAck((uint8_t *)cmd_auth1, cmd_len)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("> AUTH 1: "));
  Adafruit_PN532::PrintHexChar(cmd_auth1, cmd_len);
  PN532DEBUGPRINT.print(F("Received: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif

  /* If byte 8 isn't 0x00 we probably have an error, also byte 8 & 9 should be
   * 0x91AF */
  if (pn532_packetbuffer[7] != 0x00 || pn532_packetbuffer[24] != 0x91 ||
      pn532_packetbuffer[25] != 0xAF) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("AuthenticateFirst part 1 ResultError"));
#endif
    return 0;
  }

  /*
   * prepare the answer
   */
  // copy the encrypted RndB
  uint8_t blocklength = 16;
  uint8_t RndA[16];
  uint8_t RndB[16];
  uint8_t RndAEnc[16];
  uint8_t RndBEnc[16];
  uint8_t RndBRotl[16];
  uint8_t answer[32];
  uint8_t answer_enc[32];
  memcpy(&RndBEnc, pn532_packetbuffer + 8, blocklength);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("RndBEnc: "));
  Adafruit_PN532::PrintHexChar(RndBEnc, blocklength);
#endif
  if (!Adafruit_PN532::ntag424_decrypt(key, blocklength, RndBEnc, RndB)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Decryption error"));
    return 0;
#endif
  }
  memset(RndBRotl, 0, sizeof(RndBRotl));
  ntag424_rotl(RndB, RndBRotl, blocklength, 1);

  ntag424_random(RndA, blocklength);

#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("RndA: "));
  Adafruit_PN532::PrintHexChar(RndA, blocklength);
  PN532DEBUGPRINT.print(F("RndBEnc: "));
  Adafruit_PN532::PrintHexChar(RndBEnc, blocklength);
  PN532DEBUGPRINT.print(F("RndB: "));
  Adafruit_PN532::PrintHexChar(RndB, blocklength);
  PN532DEBUGPRINT.print(F("RndBRotl: "));
  Adafruit_PN532::PrintHexChar(RndBRotl, blocklength);
#endif
  memcpy(&answer, RndA, blocklength);
  memcpy(&answer[blocklength], RndBRotl, blocklength);
  Adafruit_PN532::ntag424_encrypt(key, sizeof(answer), answer, answer_enc);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("answer: "));
  Adafruit_PN532::PrintHexChar(answer, blocklength * 2);
  PN532DEBUGPRINT.println(F("answer_encrypted: "));
  Adafruit_PN532::PrintHexChar(answer_enc, blocklength * 2);
#endif

  /*
   * send the answer
   */
  uint8_t prefix[7] = {
      PN532_COMMAND_INDATAEXCHANGE, 0x01, 0x90, 0xaf, 0x00, 0x00, 0x20};
  uint8_t postfix[1] = {0x00};
  int apdusize = sizeof(prefix) + sizeof(answer_enc) + sizeof(postfix);
  uint8_t apdu[apdusize];
  memcpy(&apdu[0], prefix, sizeof(prefix));
  memcpy(&apdu[sizeof(prefix)], answer_enc, sizeof(answer_enc));
  memcpy(&apdu[sizeof(prefix) + sizeof(answer_enc)], postfix, sizeof(postfix));
  if (!sendCommandCheckAck((uint8_t *)apdu, apdusize)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  /* Read the response packet */
  readdata(pn532_packetbuffer, 42);
  //#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("> AUTH 2 - PCD encrypted answer: "));
  Adafruit_PN532::PrintHexChar(apdu, apdusize);
  PN532DEBUGPRINT.print(F("Received: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 42);
  //#endif
  if (pn532_packetbuffer[7] != 0x00 || pn532_packetbuffer[40] != 0x91 ||
      pn532_packetbuffer[41] != 0x00) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("AuthenticateFirst part 2 ResultError"));
    Adafruit_PN532::PrintHexChar(&pn532_packetbuffer[8], 2);
#endif
    return 0;
  }

  // decrypt the response
  uint8_t auth2_response_enc[NTAG424_AUTHRESPONSE_ENC_SIZE];
  uint8_t auth2_response[NTAG424_AUTHRESPONSE_ENC_SIZE];
  memcpy(&auth2_response_enc, pn532_packetbuffer + 8,
         NTAG424_AUTHRESPONSE_ENC_SIZE);
  if (!Adafruit_PN532::ntag424_decrypt(key, NTAG424_AUTHRESPONSE_ENC_SIZE,
                                       auth2_response_enc, auth2_response)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Decryption error"));
#endif
  }
  // save the authresponse
  memcpy(&ntag424_authresponse_TI,
         auth2_response + NTAG424_AUTHRESPONSE_TI_OFFSET,
         NTAG424_AUTHRESPONSE_TI_SIZE);
  memcpy(&ntag424_authresponse_RNDA,
         auth2_response + NTAG424_AUTHRESPONSE_RNDA_OFFSET,
         NTAG424_AUTHRESPONSE_RNDA_SIZE);
  memcpy(&ntag424_authresponse_PDCAP2,
         auth2_response + NTAG424_AUTHRESPONSE_PDCAP2_OFFSET,
         NTAG424_AUTHRESPONSE_PDCAP2_SIZE);
  memcpy(&ntag424_authresponse_PCDCAP2,
         auth2_response + NTAG424_AUTHRESPONSE_PCDCAP2_OFFSET,
         NTAG424_AUTHRESPONSE_PCDCAP2_SIZE);
  // cleanup session
  ntag424_Session.cmd_counter = 0;

  // Return OK signal
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print(F("Enrypted response: "));
  Adafruit_PN532::PrintHexChar(auth2_response_enc,
                               NTAG424_AUTHRESPONSE_ENC_SIZE);
  PN532DEBUGPRINT.print(F("Decrypted response: "));
  Adafruit_PN532::PrintHexChar(auth2_response, NTAG424_AUTHRESPONSE_ENC_SIZE);
  PN532DEBUGPRINT.print(F("TI: "));
  Adafruit_PN532::PrintHexChar(ntag424_authresponse_TI,
                               NTAG424_AUTHRESPONSE_TI_SIZE);
  PN532DEBUGPRINT.println(F("RNDA: "));
  Adafruit_PN532::PrintHexChar(ntag424_authresponse_RNDA,
                               NTAG424_AUTHRESPONSE_RNDA_SIZE);
  PN532DEBUGPRINT.println(F("PDCAP2: "));
  Adafruit_PN532::PrintHexChar(ntag424_authresponse_PDCAP2,
                               NTAG424_AUTHRESPONSE_PDCAP2_SIZE);
  PN532DEBUGPRINT.println(F("PCDCAP2: "));
  Adafruit_PN532::PrintHexChar(ntag424_authresponse_PCDCAP2,
                               NTAG424_AUTHRESPONSE_PCDCAP2_SIZE);
#endif
  // save the session data
  // Adafruit_PN532::ntag424_derive_session_keys(key, RndA, RndB);

  // test vectors
  /*
  uint8_t RndAs[16] =
  {0xB0,0x4D,0x07,0x87,0xC9,0x3E,0xE0,0xCC,0x8C,0xAC,0xC8,0xE8,0x6F,0x16,0xC6,0xFE};
  uint8_t RndBs[16] =
  {0xFA,0x65,0x9A,0xD0,0xDC,0xA7,0x38,0xDD,0x65,0xDC,0x7D,0xC3,0x86,0x12,0xAD,0x81};
  uint8_t TestSessionKey[16] =
  {0x82,0x48,0x13,0x4A,0x38,0x6E,0x86,0xEB,0x7F,0xAF,0x54,0xA5,0x2E,0x53,0x6C,0xB6};
  uint8_t TestTI[4] = {0x7A,0x21,0x08,0x5E} ;
  */
  Adafruit_PN532::ntag424_derive_session_keys(key, RndA, RndB);
  // Return OK signal
  return 1;
}

/*!
    @brief   sends a GetFileSettings-call to the picc, copies result into
   buffer.

    @param   fileno       fileno
    @param   buffer       buffer
    @param   comm_mode    one off NTAG424_COMM_MODE_PLAIN, NTAG424_COMM_MODE_MAC
   or NTAG424_COMM_MODE_FULL

    @return  length of result
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_GetFileSettings(uint8_t fileno, uint8_t *buffer,
                                                uint8_t comm_mode) {
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_CLA};
  uint8_t ins[1] = {NTAG424_CMD_GETFILESETTINGS};
  uint8_t p1[1] = {0x0};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {fileno};
  uint8_t cmd_data[1] = {0x00};
  uint8_t result[64];
  int resultlength = Adafruit_PN532::ntag424_apdu_send(
      cla, ins, p1, p2, cmd_header, sizeof(cmd_header), cmd_data, 0, 0,
      comm_mode, result, sizeof(result)

  );
  memcpy(buffer, result, resultlength);
  return resultlength;
}

/*!
    @brief   sends a ChangeFileSettings-call to the picc.

    @param   fileno                 fileno
    @param   filesettings           buffer with encoded filesettings
    @param   filesettings_length    size of filesettings buffer
    @param   comm_mode    one off NTAG424_COMM_MODE_PLAIN, NTAG424_COMM_MODE_MAC
   or NTAG424_COMM_MODE_FULL

    @return  length of result
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_ChangeFileSettings(uint8_t fileno,
                                                   uint8_t *filesettings,
                                                   uint8_t filesettings_length,
                                                   uint8_t comm_mode) {
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_CLA};
  uint8_t ins[1] = {NTAG424_CMD_CHANGEFILESETTINGS};
  uint8_t p1[1] = {0x0};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {fileno};
  uint8_t cmd_data[1] = {0x0};
  uint8_t result[30];
  uint8_t resultlength = Adafruit_PN532::ntag424_apdu_send(
      cla, ins, p1, p2, cmd_header, sizeof(cmd_header), filesettings,
      filesettings_length, 0, comm_mode, result, sizeof(result));
  // memcpy(buffer, result, 16);
  return resultlength;
}

/*!
    @brief   Change key keynumber from oldkey to newkey.

    @param   oldkey       Current key (16 byte)
    @param   newkey       New key     (16 byte)
    @param   keynumber    Keynumber to change (0-4)

    @return  false=fail|true=success
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_ChangeKey(uint8_t *oldkey, uint8_t *newkey,
                                          uint8_t keynumber) {
  uint8_t keyversion[1] = {0x01};
  uint8_t xorkey[16];
  for (int i = 0; i < 16; ++i) {
    xorkey[i] = oldkey[i] ^ newkey[i];
  }
  uint32_t crc32_newkey = Adafruit_PN532::ntag424_crc32(newkey, 16);
  // we need JAMCRCn which is the binary invers
  crc32_newkey = ~crc32_newkey;
#ifdef NTAG424DEBUG
  Serial.println("old key");
  Adafruit_PN532::PrintHex(oldkey, 16);
  Serial.println("new key");
  Adafruit_PN532::PrintHex(newkey, 16);
  Serial.println("XOR key");
  Adafruit_PN532::PrintHex(xorkey, 16);
  Serial.println("CRC32 key");
#endif
  uint8_t crcbytes[4];
  memcpy(crcbytes, &crc32_newkey, sizeof(uint32_t));
#ifdef NTAG424DEBUG
  Serial.printf("ROM CRC: %02X %02X %02X %02X\n", crcbytes[0], crcbytes[1],
                crcbytes[2], crcbytes[3]);
  Serial.println(crc32_newkey, HEX);
#endif
  // assemble keydata
  uint8_t keydata[32];
  uint8_t keydata_length = 21;
  if (keynumber > 0) {
    memcpy(keydata, xorkey, 16);
    memcpy(keydata + 16, keyversion, 1);
    memcpy(keydata + 17, crcbytes, 4);
    keydata_length = 21;
  } else if (keynumber == 0) {
    memcpy(keydata, newkey, 16);
    memcpy(keydata + 16, keyversion, 1);
    keydata_length = 17;
  }
#ifdef NTAG424DEBUG
  Serial.println("keydata:");
  Adafruit_PN532::PrintHex(keydata, keydata_length);
  Serial.println("Sessionkey ENC:");
  Adafruit_PN532::PrintHex(ntag424_Session.session_key_enc, 16);
#endif
  uint8_t cla[1] = {NTAG424_COM_CLA};
  uint8_t ins[1] = {NTAG424_COM_CHANGEKEY};
  uint8_t p1[1] = {0x0};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {keynumber};
  // uint8_t cmd_data[1] = {0x00};
  uint8_t result[50];

  uint8_t response_length = Adafruit_PN532::ntag424_apdu_send(
      cla, ins, p1, p2, cmd_header, 1, keydata, keydata_length, 0,
      NTAG424_COMM_MODE_FULL, result, sizeof(result)

  );
  Adafruit_PN532::PrintHex(result, response_length);

  if ((result[0] != 0x91) || (result[1] != 0x00)) {
    return false;
  }
  return true;
}

/*!
    @brief   Send getCardUID request to picc. Works even if random uid is
   active. Authentication required (key0 only, but i am not sure)

    @param   buffer     response buffer for the carduid (7 byte)

    @return  size of uid
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_GetCardUID(uint8_t *buffer) {
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_CLA};
  uint8_t ins[1] = {NTAG424_CMD_GETCARDUUID};
  uint8_t p1[1] = {0x0};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {0x00};
  uint8_t cmd_data[1] = {0x00};
  uint8_t result[34];

  uint8_t resp_size = Adafruit_PN532::ntag424_apdu_send(
      cla, ins, p1, p2, cmd_header, 0, cmd_data, 0, 0, NTAG424_COMM_MODE_FULL,
      result, sizeof(result)

  );

  if ((resp_size > 4) && (result[resp_size - 2] == 0x91) &&
      (result[resp_size - 1] == 0x00)) {
    memcpy(buffer, result, resp_size - 2);
    return resp_size - 2;
  }
  return 0;
}

/*!
    @brief   Send GetTTStatus request to picc. (TagTamper status) Authentication
   required

    @param   buffer     response buffer for the status (7 byte)

    @return  size of status
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_GetTTStatus(uint8_t *buffer) {
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_CLA};
  uint8_t ins[1] = {NTAG424_CMD_GETTTSTATUS};
  uint8_t p1[1] = {0x0};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {0x00};
  uint8_t cmd_data[1] = {0x00};
  uint8_t result[32];

  uint8_t resp_size = Adafruit_PN532::ntag424_apdu_send(
      cla, ins, p1, p2, cmd_header, 0, cmd_data, 0, 0, NTAG424_COMM_MODE_FULL,
      result, sizeof(result)

  );
  if ((resp_size > 2) && (result[resp_size - 2] == 0x91) &&
      (result[resp_size - 1] == 0x00)) {
    memcpy(buffer, result, resp_size - 2);
    return resp_size - 2;
  }
  return 0;
}

/*!
    @brief   Send ReadSig request to picc. (Chip signature). Authentication
   required

    @param   buffer     response buffer for the signature

    @return  size of status
*/
/**************************************************************************/
// Attention: ReadSig crashes currently. Response exceeds
// sizeof(pn532_packetbuffer). Maybe multiple reads?
uint8_t Adafruit_PN532::ntag424_ReadSig(uint8_t *buffer) {
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_CLA};
  uint8_t ins[1] = {NTAG424_CMD_READSIG};
  uint8_t p1[1] = {0x0};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {0x00};
  uint8_t cmd_data[1] = {0x00};
  uint8_t result[58];
  uint8_t resp_size = Adafruit_PN532::ntag424_apdu_send(
      cla, ins, p1, p2, cmd_header, 0, cmd_data, 1, 0, NTAG424_COMM_MODE_MAC,
      result, sizeof(result));
  memcpy(buffer, result, resp_size);
  return resp_size;
}

/*!
    @brief   Send ReadData request to picc.

    @param   buffer     buffer for the read data
    @param   fileno     fileno to read
    @param   offset     offset where to start to read from
    @param   size       number of bytes to read

    @return  size of status
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_ReadData(uint8_t *buffer, int fileno,
                                         int offset, int size) {
  /*
  uint8_t cmd[1] = {NTAG424_CMD_READDATA};
  uint8_t cmd_header[7] = {fileno,  offset & 0xff, (offset >> 8) & 0xff, (offset
  >> 16) & 0xff, size & 0xff, (size >> 8) & 0xff,  (size >> 16) & 0xff}; uint8_t
  signature[8]; Adafruit_PN532::ntag424_MAC(cmd, cmd_header, sizeof(cmd_header),
  cmd_header, 0 , signature);
  */
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_CLA;
  pn532_packetbuffer[3] = NTAG424_CMD_READDATA;
  pn532_packetbuffer[4] = 0;
  pn532_packetbuffer[5] = 0;
  // Lc
  pn532_packetbuffer[6] = 0x07;
  // FileNo
  pn532_packetbuffer[7] = fileno;
  // offset
  pn532_packetbuffer[8] = offset & 0xff;
  pn532_packetbuffer[9] = (offset >> 8) & 0xff;
  pn532_packetbuffer[10] = (offset >> 16) & 0xff;
  // length
  pn532_packetbuffer[11] = size & 0xff;
  pn532_packetbuffer[12] = (size >> 8) & 0xff;
  pn532_packetbuffer[13] = (size >> 16) & 0xff;

  // memcpy(&pn532_packetbuffer + 14, signature, 8);

  // Le
  // pn532_packetbuffer[14 + 8] = 0;
  pn532_packetbuffer[14] = 0;
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("> ReadData - PCD apdu: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 15);
#endif

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 15)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }

  /* Read the response packet */
  readdata(pn532_packetbuffer, 17 + size);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("Received: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 80);
#endif
  uint8_t datasize = pn532_packetbuffer[12];
  uint8_t offsetPW = 8 + size;
  /* If byte 8 isn't 0x00 we probably have an error */
  if ((pn532_packetbuffer[7] == 0x00) &&
      (pn532_packetbuffer[offsetPW] == 0x91) &&
      (pn532_packetbuffer[offsetPW + 1] == 0x00)) {
#ifdef NTAG424DEBUG
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer + 17, datasize);
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer + offsetPW, 1);
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer + offsetPW + 1, 1);
    Serial.println(datasize);
#endif
    memcpy(buffer, pn532_packetbuffer + 17, datasize);
  } else {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Unexpected response reading block: "));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif
    return 0;
  }
  // Return OK signal
  return datasize;
}

/*!
    @brief   Send GetVersion Request and check HW type in response.

    @return  1 = its an NTAG424-Tag; 0 = its something else
*/
/**************************************************************************/

uint8_t Adafruit_PN532::ntag424_isNTAG424() {
  ntag424_GetVersion();
  // HW type (Byte 2) for NTAG424 = 0x04
  if (ntag424_VersionInfo.HWType == NTAG424_RESPONE_GETVERSION_HWTYPE_NTAG424) {
    return 1;
  }
  return 0;
}

/*!
    @brief   Send GetVersion Requests to picc. Data goes to global
   ntag424_VersionInfo

    @return  1 = its an NTAG424-Tag; 0 = its something else
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_GetVersion() {
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_CLA;
  pn532_packetbuffer[3] = NTAG424_CMD_GETVERSION;
  pn532_packetbuffer[4] = 0x0;
  pn532_packetbuffer[5] = 0x0;
  pn532_packetbuffer[6] = 0x0;

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 7)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  readdata(pn532_packetbuffer, 15);
  ntag424_VersionInfo.VendorID = pn532_packetbuffer[8];
  ntag424_VersionInfo.HWType = pn532_packetbuffer[9];
  ntag424_VersionInfo.HWSubType = pn532_packetbuffer[10];
  ntag424_VersionInfo.HWMajorVersion = pn532_packetbuffer[11];
  ntag424_VersionInfo.HWMinorVersion = pn532_packetbuffer[12];
  ntag424_VersionInfo.HWStorageSize = pn532_packetbuffer[13];
  ntag424_VersionInfo.HWProtocol = pn532_packetbuffer[14];

  if (!pn532_packetbuffer[14] == 0xaf) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Missing additional frame request 1."));
#endif
    return 0;
  }
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_CLA;
  pn532_packetbuffer[3] = NTAG424_CMD_NEXTFRAME;
  pn532_packetbuffer[4] = 0x0;
  pn532_packetbuffer[5] = 0x0;
  pn532_packetbuffer[6] = 0x0;
  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 7)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  readdata(pn532_packetbuffer, 15);
  ntag424_VersionInfo.VendorID = pn532_packetbuffer[8];
  ntag424_VersionInfo.SWType = pn532_packetbuffer[9];
  ntag424_VersionInfo.SWSubType = pn532_packetbuffer[10];
  ntag424_VersionInfo.SWMajorVersion = pn532_packetbuffer[11];
  ntag424_VersionInfo.SWMinorVersion = pn532_packetbuffer[12];
  ntag424_VersionInfo.SWStorageSize = pn532_packetbuffer[13];
  ntag424_VersionInfo.SWProtocol = pn532_packetbuffer[14];

  if (!pn532_packetbuffer[14] == 0xaf) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Missing additional frame request 2."));
#endif
    return 0;
  }
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_CLA;
  pn532_packetbuffer[3] = NTAG424_CMD_NEXTFRAME;
  pn532_packetbuffer[4] = 0x0;
  pn532_packetbuffer[5] = 0x0;
  pn532_packetbuffer[6] = 0x0;
  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 7)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  readdata(pn532_packetbuffer, 15);
  memcpy(&ntag424_VersionInfo.UID, (uint8_t *) pn532_packetbuffer + 8, 7);
  uint8_t BatchNo[5] = {pn532_packetbuffer[15], pn532_packetbuffer[16],
                        pn532_packetbuffer[17], pn532_packetbuffer[18],
                        (byte) (pn532_packetbuffer[19] &  0xf0)
					   };
  memcpy(&ntag424_VersionInfo.BatchNo, BatchNo, 5);
  uint8_t FabKey[5] = {(byte) (pn532_packetbuffer[19] & 0x0f), 
						pn532_packetbuffer[20],
                       (byte) (pn532_packetbuffer[21] & 0x80)};
  memcpy(&ntag424_VersionInfo.FabKey, FabKey, 5);
  ntag424_VersionInfo.CWProd = (byte) (pn532_packetbuffer[21] & 0x3f);
  ntag424_VersionInfo.YearProd = pn532_packetbuffer[22];
  if (pn532_packetbuffer[23] != 0x91) {
    ntag424_VersionInfo.FabKeyID = pn532_packetbuffer[23];
  } else {
    ntag424_VersionInfo.FabKeyID = 0;
  }
#ifdef NTAG424DEBUG
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 18);
#endif

  if (pn532_packetbuffer[9] == NTAG424_RESPONE_GETVERSION_HWTYPE_NTAG424) {
    return 1;
  }
  return 0;
}

/*!
    @brief   Zero the tags default file.

    @return  false on fail|true on success
*/
/**************************************************************************/
bool Adafruit_PN532::ntag424_FormatNDEF() {
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_ISOCLA};
  uint8_t ins[1] = {NTAG424_CMD_ISOUPDATEBINARY};
  uint8_t p1[1] = {0x84};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {0x00};
  uint8_t ndefdata[PN532_PACKBUFFSIZ - 10];
  uint8_t memsize = 248;
  memset(ndefdata, 0, sizeof(ndefdata));
  uint8_t result[12];
  bool ret = true;
  uint8_t offset = 0;
  uint8_t datalen = sizeof(ndefdata);
  for (int i = 0; i < memsize; i += sizeof(ndefdata)) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(offset);
    p2[0] = offset;
    if ((offset + datalen) > memsize) {
      datalen = memsize - offset;
    }
    uint8_t bytesread = Adafruit_PN532::ntag424_apdu_send(
        cla, ins, p1, p2, cmd_header, 0, ndefdata, datalen, 0,
        NTAG424_COMM_MODE_PLAIN, result, sizeof(result)

    );
    if ((result[0] != 0x90) || (result[1] != 0x00)) {
      ret = false;
    }
    offset += datalen;

    Serial.println(bytesread);
  }
  return ret;
}

/*!
    @brief   write data_to_write to picc. (Chip signature).

    @param   data_to_write     buffer containg the data to write
    @param   length            length of the buffer

    @return  false on fail|true on success
*/
/**************************************************************************/
bool Adafruit_PN532::ntag424_ISOUpdateBinary(uint8_t *data_to_write,
                                             uint8_t length) {
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_ISOCLA};
  uint8_t ins[1] = {NTAG424_CMD_ISOUPDATEBINARY};
  uint8_t p1[1] = {0x84};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {0x00};
  uint8_t cmd_data[1] = {0x00};
  uint8_t result[12];

  uint8_t offset = 0;
  uint8_t datalen = PN532_PACKBUFFSIZ - 10;
  for (int i = 0; i < length; i += datalen) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(offset);
    p2[0] = offset;
    if ((offset + datalen) > length) {
      datalen = length - offset;
    }
    if (datalen > 0) {
      uint8_t bytesread = Adafruit_PN532::ntag424_apdu_send(
          cla, ins, p1, p2, cmd_header, 0, data_to_write + offset, datalen, 0,
          NTAG424_COMM_MODE_PLAIN, result, sizeof(result)

      );
    }
    offset += datalen;
  }
  if ((result[0] != 0x90) || (result[1] != 0x00)) {
    return false;
  }
  return true;
}

/*!
    @brief   select file by fileid for the following commands.

    @param   fileid      fileid

    @return  false on fail|true on success
*/
/**************************************************************************/
bool Adafruit_PN532::ntag424_ISOSelectFileById(int fileid) {
  // Select the default ISO-7816-4 name of the application file
  /* Prepare the command */
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_ISOCLA};
  uint8_t ins[1] = {NTAG424_CMD_ISOSELECTFILE};
  uint8_t p1[1] = {0x0};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {0x00};
  uint8_t cmd_data[2] = {(byte)((fileid >> 8) & 0xff),(byte) (fileid & 0xff)};
  uint8_t result[12];

  /* Send the command */
  Adafruit_PN532::ntag424_apdu_send(cla, ins, p1, p2, cmd_header, 0, cmd_data,
                                    2, 0, NTAG424_COMM_MODE_PLAIN, result,
                                    sizeof(result)

  );
  if ((result[0] != 0x90) || (result[1] != 0x00)) {
    return false;
  }
  return true;
}

/*!
    @brief   Select the default ISO-7816-4 dedicated filename of the application
   file.

    @param   dfn      buffer containing the ISO-7816-4 dedicated filename

    @return  false on fail|true on success
*/
/**************************************************************************/
bool Adafruit_PN532::ntag424_ISOSelectFileByDFN(uint8_t *dfn) {
  /* Prepare the command */
  uint8_t cmac_short[8];
  uint8_t cla[1] = {NTAG424_COM_ISOCLA};
  uint8_t ins[1] = {NTAG424_CMD_ISOSELECTFILE};
  uint8_t p1[1] = {0x4};
  uint8_t p2[1] = {0x0};
  uint8_t cmd_header[1] = {0x00};
  uint8_t result[12];

  /* Send the command */
  Adafruit_PN532::ntag424_apdu_send(cla, ins, p1, p2, cmd_header, 0, dfn, 7, 0,
                                    NTAG424_COMM_MODE_PLAIN, result,
                                    sizeof(result));
  if ((result[0] != 0x90) || (result[1] != 0x00)) {
    return false;
  }
  return true;
}

/*!
    @brief   read the default ISO-7816-4 dedicated file / read the tag for
   example ndef-data.

    @param   buffer     response buffer

    @return  datasize
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag424_ISOReadFile(uint8_t *buffer) {
  // uint16_t filesize =  Adafruit_PN532::ntag424_GetFileSize(buffer);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("ISOGetFileSettings"));
#endif
  // call getfilesettings
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_CLA;
  pn532_packetbuffer[3] = NTAG424_CMD_GETFILESETTINGS;
  pn532_packetbuffer[4] = 0x0;
  pn532_packetbuffer[5] = 0x0;
  pn532_packetbuffer[6] = 0x1;
  pn532_packetbuffer[7] = 0x2;
  pn532_packetbuffer[8] = 0x0;
  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 9)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  readdata(pn532_packetbuffer, 26);
#ifdef NTAG424DEBUG
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
  PN532DEBUGPRINT.println(F("ISOReadFile"));
  PN532DEBUGPRINT.println(F("ISOSelectFile1"));
#endif
  // Select the default ISO-7816-4 DF name of the application file
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_ISOCLA;
  pn532_packetbuffer[3] = NTAG424_CMD_ISOSELECTFILE;
  pn532_packetbuffer[4] = 0x4;
  pn532_packetbuffer[5] = 0x0;
  pn532_packetbuffer[6] = 0x7;
  pn532_packetbuffer[7] = 0xd2;
  pn532_packetbuffer[8] = 0x76;
  pn532_packetbuffer[9] = 0x0;
  pn532_packetbuffer[10] = 0x0;
  pn532_packetbuffer[11] = 0x85;
  pn532_packetbuffer[12] = 0x01;
  pn532_packetbuffer[13] = 0x01;
  pn532_packetbuffer[14] = 0x0;
  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 15)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("Received: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif

  /* If byte 8 isn't 0x00 we probably have an error */
  if (!((pn532_packetbuffer[7] == 0x00) && (pn532_packetbuffer[8] == 0x90))) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Error while selecting iso-file 1: "));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif
    return 0;
  }

#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("ISOSelectFile2"));
#endif
  // Select the default ISO-7816-4 DF name of the application file
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_ISOCLA;
  pn532_packetbuffer[3] = NTAG424_CMD_ISOSELECTFILE;
  pn532_packetbuffer[4] = 0x0;
  pn532_packetbuffer[5] = 0x0;
  pn532_packetbuffer[6] = 0x2;
  pn532_packetbuffer[7] = 0xe1;
  pn532_packetbuffer[8] = 0x04;
  pn532_packetbuffer[9] = 0x0;
  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 10)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print("GetFileInfo: ");
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif

  /* If byte 8 isn't 0x00 we probably have an error */
  if (!((pn532_packetbuffer[7] == 0x00) && (pn532_packetbuffer[8] == 0x90))) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Error while selecting iso-file 2"));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif
    return 0;
  }
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.println(F("ISOReadBinary1 to get the filesize"));
#endif
  // Select the default ISO-7816-4 DF name of the application file
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] = NTAG424_COM_ISOCLA;
  pn532_packetbuffer[3] = NTAG424_CMD_ISOREADBINARY;
  pn532_packetbuffer[4] = 0x0;
  pn532_packetbuffer[5] = 0x0;
  pn532_packetbuffer[6] = 0x3;

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 7)) {
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }
  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);
  int filesize = (int)pn532_packetbuffer[9] - 5;

#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print("filesize: ");
  PN532DEBUGPRINT.println(filesize);
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif

  uint8_t pagesize = 32;
  uint8_t pages = (filesize / pagesize) + 1;
  uint8_t offset = 0;
#ifdef NTAG424DEBUG
  PN532DEBUGPRINT.print("pages: ");
  PN532DEBUGPRINT.println(pages);
#endif
  for (int i = 0; i < pages; i++) {
    offset = i * pagesize;
    if (offset + pagesize > filesize) {
      pagesize = filesize - offset;
    }

#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.print(F("ISOReadBinary2-"));
    PN532DEBUGPRINT.println(i);
#endif

    // Select the default ISO-7816-4 DF name of the application file
    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1; /* Card number */
    pn532_packetbuffer[2] = NTAG424_COM_ISOCLA;
    pn532_packetbuffer[3] = NTAG424_CMD_ISOREADBINARY;
    pn532_packetbuffer[4] = 0x0;
    pn532_packetbuffer[5] = 7 + offset;
    pn532_packetbuffer[6] = pagesize;

    /* Send the command */
    if (!sendCommandCheckAck(pn532_packetbuffer, 7)) {
#ifdef NTAG424DEBUG
      PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
      return 0;
    }

    /* Read the response packet */
    readdata(pn532_packetbuffer, 64);
#ifdef NTAG424DEBUG
    PN532DEBUGPRINT.println(F("Received: "));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 64);
#endif
    /* If byte 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[7] == 0x00) {
      /* Copy the the data bytes to the output buffer         */
      /* Block content starts at byte 9 of a valid response */
      memcpy(&buffer[offset], pn532_packetbuffer + 8, pagesize);
    } else {
#ifdef NTAG424DEBUG
      PN532DEBUGPRINT.println(F("Unexpected response reading block: "));
#endif
      return 0;
    }
  }
  // Return OK signal
  return filesize;
}

/***** NTAG2xx Functions ******/

/**************************************************************************/
/*!
    @brief   Tries to read an entire 4-byte page at the specified address.

    @param   page        The page number (0..63 in most cases)
    @param   buffer      Pointer to the byte array that will hold the
                         retrieved data (if any)
    @return  1 on success, 0 on error.
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag2xx_ReadPage(uint8_t page, uint8_t *buffer) {
  // TAG Type       PAGES   USER START    USER STOP
  // --------       -----   ----------    ---------
  // NTAG 203       42      4             39
  // NTAG 213       45      4             39
  // NTAG 215       135     4             129
  // NTAG 216       231     4             225

  if (page >= 231) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Page value out of range"));
#endif
    return 0;
  }

#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Reading page "));
  PN532DEBUGPRINT.println(page);
#endif

  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;               /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
  pn532_packetbuffer[3] = page; /* Page Number (0..63 in most cases) */

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 4)) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif
    return 0;
  }

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.println(F("Received: "));
  Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif

  /* If byte 8 isn't 0x00 we probably have an error */
  if (pn532_packetbuffer[7] == 0x00) {
    /* Copy the 4 data bytes to the output buffer         */
    /* Block content starts at byte 9 of a valid response */
    /* Note that the command actually reads 16 byte or 4  */
    /* pages at a time ... we simply discard the last 12  */
    /* bytes                                              */
    memcpy(buffer, pn532_packetbuffer + 8, 4);
  } else {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Unexpected response reading block: "));
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
#endif
    return 0;
  }

/* Display data for debug if requested */
#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Page "));
  PN532DEBUGPRINT.print(page);
  PN532DEBUGPRINT.println(F(":"));
  Adafruit_PN532::PrintHexChar(buffer, 4);
#endif

  // Return OK signal
  return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 4-byte page at the specified block
    address.

    @param  page          The page number to write.  (0..63 for most cases)
    @param  data          The byte array that contains the data to write.
                          Should be exactly 4 bytes long.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag2xx_WritePage(uint8_t page, uint8_t *data) {
  // TAG Type       PAGES   USER START    USER STOP
  // --------       -----   ----------    ---------
  // NTAG 203       42      4             39
  // NTAG 213       45      4             39
  // NTAG 215       135     4             129
  // NTAG 216       231     4             225

  if ((page < 4) || (page > 225)) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Page value out of range"));
#endif
    // Return Failed Signal
    return 0;
  }

#ifdef MIFAREDEBUG
  PN532DEBUGPRINT.print(F("Trying to write 4 byte page"));
  PN532DEBUGPRINT.println(page);
#endif

  /* Prepare the first command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1; /* Card number */
  pn532_packetbuffer[2] =
      MIFARE_ULTRALIGHT_CMD_WRITE; /* Mifare Ultralight Write command = 0xA2 */
  pn532_packetbuffer[3] = page;    /* Page Number (0..63 for most cases) */
  memcpy(pn532_packetbuffer + 4, data, 4); /* Data Payload */

  /* Send the command */
  if (!sendCommandCheckAck(pn532_packetbuffer, 8)) {
#ifdef MIFAREDEBUG
    PN532DEBUGPRINT.println(F("Failed to receive ACK for write command"));
#endif

    // Return Failed Signal
    return 0;
  }
  delay(10);

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);

  // Return OK Signal
  return 1;
}

/**************************************************************************/
/*!
    Writes an NDEF URI Record starting at the specified page (4..nn)

    Note that this function assumes that the NTAG2xx card is
    already formatted to work as an "NFC Forum Tag".

    @param  uriIdentifier The uri identifier code (0 = none, 0x01 =
                          "http://www.", etc.)
    @param  url           The uri text to write (null-terminated string).
    @param  dataLen       The size of the data area for overflow checks.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::ntag2xx_WriteNDEFURI(uint8_t uriIdentifier, char *url,
                                             uint8_t dataLen) {
  uint8_t pageBuffer[4] = {0, 0, 0, 0};

  // Remove NDEF record overhead from the URI data (pageHeader below)
  uint8_t wrapperSize = 12;

  // Figure out how long the string is
  uint8_t len = strlen(url);

  // Make sure the URI payload will fit in dataLen (include 0xFE trailer)
  if ((len < 1) || (len + 1 > (dataLen - wrapperSize)))
    return 0;

  // Setup the record header
  // See NFCForum-TS-Type-2-Tag_1.1.pdf for details
  uint8_t pageHeader[12] = {
      /* NDEF Lock Control TLV (must be first and always present) */
      0x01, /* Tag Field (0x01 = Lock Control TLV) */
      0x03, /* Payload Length (always 3) */
      0xA0, /* The position inside the tag of the lock bytes (upper 4 = page
               address, lower 4 = byte offset) */
      0x10, /* Size in bits of the lock area */
      0x44, /* Size in bytes of a page and the number of bytes each lock bit can
               lock (4 bit + 4 bits) */
      /* NDEF Message TLV - URI Record */
      0x03,               /* Tag Field (0x03 = NDEF Message) */
      (uint8_t)(len + 5), /* Payload Length (not including 0xFE trailer) */
      0xD1, /* NDEF Record Header (TNF=0x1:Well known record + SR + ME + MB) */
      0x01, /* Type Length for the record type indicator */
      (uint8_t)(len + 1), /* Payload len */
      0x55,               /* Record Type Indicator (0x55 or 'U' = URI Record) */
      uriIdentifier       /* URI Prefix (ex. 0x01 = "http://www.") */
  };

  // Write 12 byte header (three pages of data starting at page 4)
  memcpy(pageBuffer, pageHeader, 4);
  if (!(ntag2xx_WritePage(4, pageBuffer)))
    return 0;
  memcpy(pageBuffer, pageHeader + 4, 4);
  if (!(ntag2xx_WritePage(5, pageBuffer)))
    return 0;
  memcpy(pageBuffer, pageHeader + 8, 4);
  if (!(ntag2xx_WritePage(6, pageBuffer)))
    return 0;

  // Write URI (starting at page 7)
  uint8_t currentPage = 7;
  char *urlcopy = url;
  while (len) {
    if (len < 4) {
      memset(pageBuffer, 0, 4);
      memcpy(pageBuffer, urlcopy, len);
      pageBuffer[len] = 0xFE; // NDEF record footer
      if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
        return 0;
      // DONE!
      return 1;
    } else if (len == 4) {
      memcpy(pageBuffer, urlcopy, len);
      if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
        return 0;
      memset(pageBuffer, 0, 4);
      pageBuffer[0] = 0xFE; // NDEF record footer
      currentPage++;
      if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
        return 0;
      // DONE!
      return 1;
    } else {
      // More than one page of data left
      memcpy(pageBuffer, urlcopy, 4);
      if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
        return 0;
      currentPage++;
      urlcopy += 4;
      len -= 4;
    }
  }

  // Seems that everything was OK (?!)
  return 1;
}

/************** high level communication functions (handles both I2C and SPI) */

/**************************************************************************/
/*!
    @brief  Tries to read the SPI or I2C ACK signal
*/
/**************************************************************************/
bool Adafruit_PN532::readack() {
  uint8_t ackbuff[6];

  if (spi_dev) {
    uint8_t cmd = PN532_SPI_DATAREAD;
    spi_dev->write_then_read(&cmd, 1, ackbuff, 6);
  } else if (i2c_dev || ser_dev) {
    readdata(ackbuff, 6);
  }

  return (0 == memcmp((char *)ackbuff, (char *)pn532ack, 6));
}

/**************************************************************************/
/*!
    @brief  Return true if the PN532 is ready with a response.
*/
/**************************************************************************/
bool Adafruit_PN532::isready() {
  if (spi_dev) {
    // SPI ready check via Status Request
    uint8_t cmd = PN532_SPI_STATREAD;
    uint8_t reply;
    spi_dev->write_then_read(&cmd, 1, &reply, 1);
    return reply == PN532_SPI_READY;
  } else if (i2c_dev) {
    // I2C ready check via reading RDY byte
    uint8_t rdy[1];
    i2c_dev->read(rdy, 1);
    return rdy[0] == PN532_I2C_READY;
  } else if (ser_dev) {
    // Serial ready check based on non-zero read buffer
    return (ser_dev->available() != 0);
  } else if (_irq != -1) {
    uint8_t x = digitalRead(_irq);
    return x == 0;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief  Waits until the PN532 is ready.

    @param  timeout   Timeout before giving up
*/
/**************************************************************************/
bool Adafruit_PN532::waitready(uint16_t timeout) {
  uint16_t timer = 0;
  while (!isready()) {
    if (timeout != 0) {
      timer += 10;
      if (timer > timeout) {
#ifdef PN532DEBUG
        PN532DEBUGPRINT.println("TIMEOUT!");
#endif
        return false;
      }
    }
    delay(10);
  }
  return true;
}

/**************************************************************************/
/*!
    @brief  Reads n bytes of data from the PN532 via SPI or I2C.

    @param  buff      Pointer to the buffer where data will be written
    @param  n         Number of bytes to be read
*/
/**************************************************************************/
void Adafruit_PN532::readdata(uint8_t *buff, uint8_t n) {
  if (spi_dev) {
    // SPI read
    uint8_t cmd = PN532_SPI_DATAREAD;
    spi_dev->write_then_read(&cmd, 1, buff, n);
  } else if (i2c_dev) {
    // I2C read
    uint8_t rbuff[n + 1]; // +1 for leading RDY byte
    i2c_dev->read(rbuff, n + 1);
    for (uint8_t i = 0; i < n; i++) {
      buff[i] = rbuff[i + 1];
    }
  } else if (ser_dev) {
    // Serial read
    ser_dev->readBytes(buff, n);
  }
#ifdef PN532DEBUG
  PN532DEBUGPRINT.print(F("Reading: "));
  for (uint8_t i = 0; i < n; i++) {
    PN532DEBUGPRINT.print(F(" 0x"));
    PN532DEBUGPRINT.print(buff[i], HEX);
  }
  PN532DEBUGPRINT.println();
#endif
}

/**************************************************************************/
/*!
    @brief   set the PN532 as iso14443a Target behaving as a SmartCard
    @return  true on success, false otherwise.
    @note    Author: Salvador Mendoza (salmg.net) new functions:
             -AsTarget
             -getDataTarget
             -setDataTarget
*/
/**************************************************************************/
uint8_t Adafruit_PN532::AsTarget() {
  pn532_packetbuffer[0] = 0x8C;
  uint8_t target[] = {
      0x8C,             // INIT AS TARGET
      0x00,             // MODE -> BITFIELD
      0x08, 0x00,       // SENS_RES - MIFARE PARAMS
      0xdc, 0x44, 0x20, // NFCID1T
      0x60,             // SEL_RES
      0x01, 0xfe, // NFCID2T MUST START WITH 01fe - FELICA PARAMS - POL_RES
      0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xc0,
      0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, // PAD
      0xff, 0xff,                               // SYSTEM CODE
      0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44,
      0x33, 0x22, 0x11, 0x01, 0x00, // NFCID3t MAX 47 BYTES ATR_RES
      0x0d, 0x52, 0x46, 0x49, 0x44, 0x49, 0x4f,
      0x74, 0x20, 0x50, 0x4e, 0x35, 0x33, 0x32 // HISTORICAL BYTES
  };
  if (!sendCommandCheckAck(target, sizeof(target)))
    return false;

  // read data packet
  readdata(pn532_packetbuffer, 8);

  int offset = 6;
  return (pn532_packetbuffer[offset] == 0x15);
}
/**************************************************************************/
/*!
    @brief   Retrieve response from the emulation mode

    @param   cmd    = data
    @param   cmdlen = data length
    @return  true on success, false otherwise.
*/
/**************************************************************************/
uint8_t Adafruit_PN532::getDataTarget(uint8_t *cmd, uint8_t *cmdlen) {
  uint8_t length;
  pn532_packetbuffer[0] = 0x86;
  if (!sendCommandCheckAck(pn532_packetbuffer, 1, 1000)) {
    PN532DEBUGPRINT.println(F("Error en ack"));
    return false;
  }

  // read data packet
  readdata(pn532_packetbuffer, 64);
  length = pn532_packetbuffer[3] - 3;

  // if (length > *responseLength) {// Bug, should avoid it in the reading
  // target data
  //  length = *responseLength; // silent truncation...
  //}

  for (int i = 0; i < length; ++i) {
    cmd[i] = pn532_packetbuffer[8 + i];
  }
  *cmdlen = length;
  return true;
}

/**************************************************************************/
/*!
    @brief   Set data in PN532 in the emulation mode

    @param   cmd    = data
    @param   cmdlen = data length
    @return  true on success, false otherwise.
*/
/**************************************************************************/
uint8_t Adafruit_PN532::setDataTarget(uint8_t *cmd, uint8_t cmdlen) {
  uint8_t length;
  // cmd1[0] = 0x8E; Must!

  if (!sendCommandCheckAck(cmd, cmdlen))
    return false;

  // read data packet
  readdata(pn532_packetbuffer, 8);
  length = pn532_packetbuffer[3] - 3;
  for (int i = 0; i < length; ++i) {
    cmd[i] = pn532_packetbuffer[8 + i];
  }
  // cmdl = 0
  cmdlen = length;

  int offset = 6;
  return (pn532_packetbuffer[offset] == 0x15);
}

/**************************************************************************/
/*!
    @brief  Writes a command to the PN532, automatically inserting the
            preamble and required frame details (checksum, len, etc.)

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    Command length in bytes
*/
/**************************************************************************/
void Adafruit_PN532::writecommand(uint8_t *cmd, uint8_t cmdlen) {
  if (spi_dev) {
    // SPI command write.
    uint8_t checksum;
    uint8_t packet[9 + cmdlen];
    uint8_t *p = packet;
    cmdlen++;

    p[0] = PN532_SPI_DATAWRITE;
    p++;

    p[0] = PN532_PREAMBLE;
    p++;
    p[0] = PN532_STARTCODE1;
    p++;
    p[0] = PN532_STARTCODE2;
    p++;
    checksum = PN532_PREAMBLE + PN532_STARTCODE1 + PN532_STARTCODE2;

    p[0] = cmdlen;
    p++;
    p[0] = ~cmdlen + 1;
    p++;

    p[0] = PN532_HOSTTOPN532;
    p++;
    checksum += PN532_HOSTTOPN532;

    for (uint8_t i = 0; i < cmdlen - 1; i++) {
      p[0] = cmd[i];
      p++;
      checksum += cmd[i];
    }

    p[0] = ~checksum;
    p++;
    p[0] = PN532_POSTAMBLE;
    p++;

#ifdef PN532DEBUG
    PN532DEBUGPRINT.print("Sending : ");
    for (int i = 1; i < 8 + cmdlen; i++) {
      PN532DEBUGPRINT.print("0x");
      PN532DEBUGPRINT.print(packet[i], HEX);
      PN532DEBUGPRINT.print(", ");
    }
    PN532DEBUGPRINT.println();
#endif

    spi_dev->write(packet, 8 + cmdlen);
  } else if (i2c_dev || ser_dev) {
    // I2C or Serial command write.
    uint8_t packet[8 + cmdlen];
    uint8_t LEN = cmdlen + 1;

    packet[0] = PN532_PREAMBLE;
    packet[1] = PN532_STARTCODE1;
    packet[2] = PN532_STARTCODE2;
    packet[3] = LEN;
    packet[4] = ~LEN + 1;
    packet[5] = PN532_HOSTTOPN532;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < cmdlen; i++) {
      packet[6 + i] = cmd[i];
      sum += cmd[i];
    }
    packet[6 + cmdlen] = ~(PN532_HOSTTOPN532 + sum) + 1;
    packet[7 + cmdlen] = PN532_POSTAMBLE;

#ifdef PN532DEBUG
    Serial.print("Sending : ");
    for (int i = 1; i < 8 + cmdlen; i++) {
      Serial.print("0x");
      Serial.print(packet[i], HEX);
      Serial.print(", ");
    }
    Serial.println();
#endif

    if (i2c_dev) {
      i2c_dev->write(packet, 8 + cmdlen);
    } else {
      ser_dev->write(packet, 8 + cmdlen);
    }
  }
}
