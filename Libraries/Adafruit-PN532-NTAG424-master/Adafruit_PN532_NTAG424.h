/**************************************************************************/
/*!
    @file Adafruit_PN532.h

    v2.0  - Refactored to add I2C support from Adafruit_NFCShield_I2C library.

    v1.1  - Added full command list
          - Added 'verbose' mode flag to constructor to toggle debug output
          - Changed readPassiveTargetID() to return variable length values
*/
/**************************************************************************/

#ifndef ADAFRUIT_PN532_H
#define ADAFRUIT_PN532_H

#include "Arduino.h"

#include <Adafruit_I2CDevice.h>
#include <Adafruit_SPIDevice.h>
#include "mbedtls/aes.h"
#include "mbedtlscmac.h"
#include <Arduino_CRC32.h>

#define PN532_PREAMBLE (0x00)   ///< Command sequence start, byte 1/3
#define PN532_STARTCODE1 (0x00) ///< Command sequence start, byte 2/3
#define PN532_STARTCODE2 (0xFF) ///< Command sequence start, byte 3/3
#define PN532_POSTAMBLE (0x00)  ///< EOD

#define PN532_HOSTTOPN532 (0xD4) ///< Host-to-PN532
#define PN532_PN532TOHOST (0xD5) ///< PN532-to-host

// PN532 Commands
#define PN532_COMMAND_DIAGNOSE (0x00)              ///< Diagnose
#define PN532_COMMAND_GETFIRMWAREVERSION (0x02)    ///< Get firmware version
#define PN532_COMMAND_GETGENERALSTATUS (0x04)      ///< Get general status
#define PN532_COMMAND_READREGISTER (0x06)          ///< Read register
#define PN532_COMMAND_WRITEREGISTER (0x08)         ///< Write register
#define PN532_COMMAND_READGPIO (0x0C)              ///< Read GPIO
#define PN532_COMMAND_WRITEGPIO (0x0E)             ///< Write GPIO
#define PN532_COMMAND_SETSERIALBAUDRATE (0x10)     ///< Set serial baud rate
#define PN532_COMMAND_SETPARAMETERS (0x12)         ///< Set parameters
#define PN532_COMMAND_SAMCONFIGURATION (0x14)      ///< SAM configuration
#define PN532_COMMAND_POWERDOWN (0x16)             ///< Power down
#define PN532_COMMAND_RFCONFIGURATION (0x32)       ///< RF config
#define PN532_COMMAND_RFREGULATIONTEST (0x58)      ///< RF regulation test
#define PN532_COMMAND_INJUMPFORDEP (0x56)          ///< Jump for DEP
#define PN532_COMMAND_INJUMPFORPSL (0x46)          ///< Jump for PSL
#define PN532_COMMAND_INLISTPASSIVETARGET (0x4A)   ///< List passive target
#define PN532_COMMAND_INATR (0x50)                 ///< ATR
#define PN532_COMMAND_INPSL (0x4E)                 ///< PSL
#define PN532_COMMAND_INDATAEXCHANGE (0x40)        ///< Data exchange
#define PN532_COMMAND_INCOMMUNICATETHRU (0x42)     ///< Communicate through
#define PN532_COMMAND_INDESELECT (0x44)            ///< Deselect
#define PN532_COMMAND_INRELEASE (0x52)             ///< Release
#define PN532_COMMAND_INSELECT (0x54)              ///< Select
#define PN532_COMMAND_INAUTOPOLL (0x60)            ///< Auto poll
#define PN532_COMMAND_TGINITASTARGET (0x8C)        ///< Init as target
#define PN532_COMMAND_TGSETGENERALBYTES (0x92)     ///< Set general bytes
#define PN532_COMMAND_TGGETDATA (0x86)             ///< Get data
#define PN532_COMMAND_TGSETDATA (0x8E)             ///< Set data
#define PN532_COMMAND_TGSETMETADATA (0x94)         ///< Set metadata
#define PN532_COMMAND_TGGETINITIATORCOMMAND (0x88) ///< Get initiator command
#define PN532_COMMAND_TGRESPONSETOINITIATOR (0x90) ///< Response to initiator
#define PN532_COMMAND_TGGETTARGETSTATUS (0x8A)     ///< Get target status

#define PN532_RESPONSE_INDATAEXCHANGE (0x41)      ///< Data exchange
#define PN532_RESPONSE_INLISTPASSIVETARGET (0x4B) ///< List passive target

#define PN532_WAKEUP (0x55) ///< Wake

#define PN532_SPI_STATREAD (0x02)  ///< Stat read
#define PN532_SPI_DATAWRITE (0x01) ///< Data write
#define PN532_SPI_DATAREAD (0x03)  ///< Data read
#define PN532_SPI_READY (0x01)     ///< Ready

#define PN532_I2C_ADDRESS (0x48 >> 1) ///< Default I2C address
#define PN532_I2C_READBIT (0x01)      ///< Read bit
#define PN532_I2C_BUSY (0x00)         ///< Busy
#define PN532_I2C_READY (0x01)        ///< Ready
#define PN532_I2C_READYTIMEOUT (20)   ///< Ready timeout

#define PN532_MIFARE_ISO14443A (0x00) ///< MiFare

// NTAG242 Commands
#define NTAG424_COMM_MODE_PLAIN (0x00)        ///< Commmode plain
#define NTAG424_COMM_MODE_MAC (0x01)          ///< Commmode mac
#define NTAG424_COMM_MODE_FULL (0x02)         ///< Commmode full
#define NTAG424_COM_CLA (0x90)                ///< CLA prefix
#define NTAG424_COM_CHANGEKEY (0xC4)          ///< changekey
#define NTAG424_CMD_READSIG (0x3C)            ///< Read_Sig
#define NTAG424_CMD_GETTTSTATUS (0xF7)        ///< getttstatus
#define NTAG424_CMD_GETFILESETTINGS (0xF5)    ///< getfilesettings
#define NTAG424_CMD_CHANGEFILESETTINGS (0x5F) ///< changefilesettings
#define NTAG424_CMD_GETCARDUUID (0x51)        ///< getfilesettings
#define NTAG424_CMD_READDATA (0xAD)           ///< Readdata
#define NTAG424_CMD_GETVERSION (0x60)         ///< GetVersion
#define NTAG424_CMD_NEXTFRAME (0xAF)          ///< Nextframe

#define NTAG424_RESPONE_GETVERSION_HWTYPE_NTAG424                              \
  (0x04) ///< Response value HWType NTAG 424

#define NTAG424_COM_ISOCLA (0x00)          ///< ISO prefix
#define NTAG424_CMD_ISOSELECTFILE (0xA4)   ///< ISOSelectFile
#define NTAG424_CMD_ISOREADBINARY (0xB0)   ///< ISOReadBinary
#define NTAG424_CMD_ISOUPDATEBINARY (0xD6) ///< ISOUpdateBinary

// Mifare Commands
#define MIFARE_CMD_AUTH_A (0x60)           ///< Auth A
#define MIFARE_CMD_AUTH_B (0x61)           ///< Auth B
#define MIFARE_CMD_READ (0x30)             ///< Read
#define MIFARE_CMD_WRITE (0xA0)            ///< Write
#define MIFARE_CMD_TRANSFER (0xB0)         ///< Transfer
#define MIFARE_CMD_DECREMENT (0xC0)        ///< Decrement
#define MIFARE_CMD_INCREMENT (0xC1)        ///< Increment
#define MIFARE_CMD_STORE (0xC2)            ///< Store
#define MIFARE_ULTRALIGHT_CMD_WRITE (0xA2) ///< Write (MiFare Ultralight)

// Prefixes for NDEF Records (to identify record type)
#define NDEF_URIPREFIX_NONE (0x00)         ///< No prefix
#define NDEF_URIPREFIX_HTTP_WWWDOT (0x01)  ///< HTTP www. prefix
#define NDEF_URIPREFIX_HTTPS_WWWDOT (0x02) ///< HTTPS www. prefix
#define NDEF_URIPREFIX_HTTP (0x03)         ///< HTTP prefix
#define NDEF_URIPREFIX_HTTPS (0x04)        ///< HTTPS prefix
#define NDEF_URIPREFIX_TEL (0x05)          ///< Tel prefix
#define NDEF_URIPREFIX_MAILTO (0x06)       ///< Mailto prefix
#define NDEF_URIPREFIX_FTP_ANONAT (0x07)   ///< FTP
#define NDEF_URIPREFIX_FTP_FTPDOT (0x08)   ///< FTP dot
#define NDEF_URIPREFIX_FTPS (0x09)         ///< FTPS
#define NDEF_URIPREFIX_SFTP (0x0A)         ///< SFTP
#define NDEF_URIPREFIX_SMB (0x0B)          ///< SMB
#define NDEF_URIPREFIX_NFS (0x0C)          ///< NFS
#define NDEF_URIPREFIX_FTP (0x0D)          ///< FTP
#define NDEF_URIPREFIX_DAV (0x0E)          ///< DAV
#define NDEF_URIPREFIX_NEWS (0x0F)         ///< NEWS
#define NDEF_URIPREFIX_TELNET (0x10)       ///< Telnet prefix
#define NDEF_URIPREFIX_IMAP (0x11)         ///< IMAP prefix
#define NDEF_URIPREFIX_RTSP (0x12)         ///< RTSP
#define NDEF_URIPREFIX_URN (0x13)          ///< URN
#define NDEF_URIPREFIX_POP (0x14)          ///< POP
#define NDEF_URIPREFIX_SIP (0x15)          ///< SIP
#define NDEF_URIPREFIX_SIPS (0x16)         ///< SIPS
#define NDEF_URIPREFIX_TFTP (0x17)         ///< TFPT
#define NDEF_URIPREFIX_BTSPP (0x18)        ///< BTSPP
#define NDEF_URIPREFIX_BTL2CAP (0x19)      ///< BTL2CAP
#define NDEF_URIPREFIX_BTGOEP (0x1A)       ///< BTGOEP
#define NDEF_URIPREFIX_TCPOBEX (0x1B)      ///< TCPOBEX
#define NDEF_URIPREFIX_IRDAOBEX (0x1C)     ///< IRDAOBEX
#define NDEF_URIPREFIX_FILE (0x1D)         ///< File
#define NDEF_URIPREFIX_URN_EPC_ID (0x1E)   ///< URN EPC ID
#define NDEF_URIPREFIX_URN_EPC_TAG (0x1F)  ///< URN EPC tag
#define NDEF_URIPREFIX_URN_EPC_PAT (0x20)  ///< URN EPC pat
#define NDEF_URIPREFIX_URN_EPC_RAW (0x21)  ///< URN EPC raw
#define NDEF_URIPREFIX_URN_EPC (0x22)      ///< URN EPC
#define NDEF_URIPREFIX_URN_NFC (0x23)      ///< URN NFC

#define PN532_GPIO_VALIDATIONBIT (0x80) ///< GPIO validation bit
#define PN532_GPIO_P30 (0)              ///< GPIO 30
#define PN532_GPIO_P31 (1)              ///< GPIO 31
#define PN532_GPIO_P32 (2)              ///< GPIO 32
#define PN532_GPIO_P33 (3)              ///< GPIO 33
#define PN532_GPIO_P34 (4)              ///< GPIO 34
#define PN532_GPIO_P35 (5)              ///< GPIO 35

/**
 * @brief Class for working with Adafruit PN532 NFC/RFID breakout boards.
 */
class Adafruit_PN532 {
public:
  Adafruit_PN532(uint8_t clk, uint8_t miso, uint8_t mosi,
                 uint8_t ss);                          // Software SPI
  Adafruit_PN532(uint8_t ss, SPIClass *theSPI = &SPI); // Hardware SPI
  Adafruit_PN532(uint8_t irq, uint8_t reset,
                 TwoWire *theWire = &Wire);              // Hardware I2C
  Adafruit_PN532(uint8_t reset, HardwareSerial *theSer); // Hardware UART
  bool begin(void);

  void reset(void);
  void wakeup(void);

  // Generic PN532 functions
  bool SAMConfig(void);
  uint32_t getFirmwareVersion(void);
  bool sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen,
                           uint16_t timeout = 100);
  bool writeGPIO(uint8_t pinstate);
  uint8_t readGPIO(void);
  bool setPassiveActivationRetries(uint8_t maxRetries);

  // ISO14443A functions
  bool readPassiveTargetID(
      uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength,
      uint16_t timeout = 0); // timeout 0 means no timeout - will block forever.
  bool startPassiveTargetIDDetection(uint8_t cardbaudrate);
  bool readDetectedPassiveTargetID(uint8_t *uid, uint8_t *uidLength);
  bool inDataExchange(uint8_t *send, uint8_t sendLength, uint8_t *response,
                      uint8_t *responseLength);
  bool inListPassiveTarget();
  uint8_t AsTarget();
  uint8_t getDataTarget(uint8_t *cmd, uint8_t *cmdlen);
  uint8_t setDataTarget(uint8_t *cmd, uint8_t cmdlen);

  // Mifare Classic functions
  bool mifareclassic_IsFirstBlock(uint32_t uiBlock);
  bool mifareclassic_IsTrailerBlock(uint32_t uiBlock);
  uint8_t mifareclassic_AuthenticateBlock(uint8_t *uid, uint8_t uidLen,
                                          uint32_t blockNumber,
                                          uint8_t keyNumber, uint8_t *keyData);
  uint8_t mifareclassic_ReadDataBlock(uint8_t blockNumber, uint8_t *data);
  uint8_t mifareclassic_WriteDataBlock(uint8_t blockNumber, uint8_t *data);
  uint8_t mifareclassic_FormatNDEF(void);
  uint8_t mifareclassic_WriteNDEFURI(uint8_t sectorNumber,
                                     uint8_t uriIdentifier, const char *url);

  // Mifare Ultralight functions
  uint8_t mifareultralight_ReadPage(uint8_t page, uint8_t *buffer);
  uint8_t mifareultralight_WritePage(uint8_t page, uint8_t *data);

  // NTAG424 functions
  uint8_t ntag424_apdu_send(uint8_t *cla, uint8_t *ins, uint8_t *p1,
                            uint8_t *p2, uint8_t *cmd_header,
                            uint8_t cmd_header_length, uint8_t *cmd_data,
                            uint8_t cmd_data_length, uint8_t le,
                            uint8_t comm_mode, uint8_t *response,
                            uint8_t response_le);
  uint32_t ntag424_crc32(uint8_t *data, uint8_t datalength);
  uint8_t ntag424_addpadding(uint8_t inputlength, uint8_t paddinglength,
                             uint8_t *buffer);
  uint8_t ntag424_encrypt(uint8_t *key, uint8_t length, uint8_t *input,
                          uint8_t *output);
  uint8_t ntag424_encrypt(uint8_t *key, uint8_t *iv, uint8_t length,
                          uint8_t *input, uint8_t *output);
  uint8_t ntag424_decrypt(uint8_t *key, uint8_t length, uint8_t *input,
                          uint8_t *output);
  uint8_t ntag424_decrypt(uint8_t *key, uint8_t *iv, uint8_t length,
                          uint8_t *input, uint8_t *output);
  uint8_t ntag424_cmac_short(uint8_t *key, uint8_t *input, uint8_t length,
                             uint8_t *cmac);
  uint8_t ntag424_cmac(uint8_t *key, uint8_t *input, uint8_t length,
                       uint8_t *cmac);
  uint8_t ntag424_MAC(uint8_t *cmd, uint8_t *cmdheader,
                      uint8_t cmdheader_length, uint8_t *cmddata,
                      uint8_t cmddata_length, uint8_t *signature);
  uint8_t ntag424_MAC(uint8_t *key, uint8_t *cmd, uint8_t *cmdheader,
                      uint8_t cmdheader_length, uint8_t *cmddata,
                      uint8_t cmddata_length, uint8_t *signature);
  void ntag424_random(uint8_t *output, uint8_t bytecount);
  void ntag424_derive_session_keys(uint8_t *key, uint8_t *RndA, uint8_t *RndB);
  uint8_t ntag424_rotl(uint8_t *input, uint8_t *output, uint8_t bufferlen,
                       uint8_t rotation);
  uint8_t ntag424_ReadData(uint8_t *buffer, int fileno, int offset, int size);
  uint8_t ntag424_Authenticate(uint8_t *key, uint8_t keyno, uint8_t cmd);
  uint8_t ntag424_ChangeKey(uint8_t *oldkey, uint8_t *newkey,
                            uint8_t keynumber);
  uint8_t ntag424_ReadSig(uint8_t *buffer);
  uint8_t ntag424_GetTTStatus(uint8_t *buffer);
  uint8_t ntag424_GetCardUID(uint8_t *buffer);
  uint8_t ntag424_GetFileSettings(uint8_t fileno, uint8_t *buffer,
                                  uint8_t comm_mode);
  uint8_t ntag424_ChangeFileSettings(uint8_t fileno, uint8_t *filesettings,
                                     uint8_t filesettings_length,
                                     uint8_t comm_mode);
  uint8_t ntag424_ISOReadFile(uint8_t *buffer);
  bool ntag424_FormatNDEF();
  bool ntag424_ISOUpdateBinary(uint8_t *buffer, uint8_t length);
  bool ntag424_ISOSelectFileById(int fileid);
  bool ntag424_ISOSelectFileByDFN(uint8_t *dfn);
  uint8_t ntag424_isNTAG424();
  uint8_t ntag424_GetVersion();

// NTAG424 authresponse data
#define NTAG424_AUTHRESPONSE_ENC_SIZE 32    ///< Size of encoded Auth-Response
#define NTAG424_AUTHRESPONSE_TI_SIZE 4      ///< Size of TI
#define NTAG424_AUTHRESPONSE_RNDA_SIZE 16   ///< Size of RND
#define NTAG424_AUTHRESPONSE_PDCAP2_SIZE 6  ///< Size of PDCAP2
#define NTAG424_AUTHRESPONSE_PCDCAP2_SIZE 6 ///< Size of PCDCAP2

#define NTAG424_AUTHRESPONSE_TI_OFFSET 0 ///< Offset for TI
#define NTAG424_AUTHRESPONSE_RNDA_OFFSET                                       \
  NTAG424_AUTHRESPONSE_TI_SIZE ///< Offset for RND
#define NTAG424_AUTHRESPONSE_PDCAP2_OFFSET                                     \
  NTAG424_AUTHRESPONSE_TI_SIZE +                                               \
      NTAG424_AUTHRESPONSE_RNDA_SIZE ///< Offset for PDCAP2
#define NTAG424_AUTHRESPONSE_PCDCAP2_OFFSET                                    \
  NTAG424_AUTHRESPONSE_TI_SIZE + NTAG424_AUTHRESPONSE_RNDA_SIZE +              \
      NTAG424_AUTHRESPONSE_PDCAP2_SIZE ///< Offset for PCDCAP2

  uint8_t ntag424_authresponse_TI[NTAG424_AUTHRESPONSE_TI_SIZE]; ///< TI Buffer
  uint8_t ntag424_authresponse_RNDA[NTAG424_AUTHRESPONSE_RNDA_SIZE]; ///< RNDA
                                                                     ///< Buffer
  uint8_t
      ntag424_authresponse_PDCAP2[NTAG424_AUTHRESPONSE_PDCAP2_SIZE]; ///< PDCAP2
                                                                     ///< Buffer
  uint8_t ntag424_authresponse_PCDCAP2
      [NTAG424_AUTHRESPONSE_PCDCAP2_SIZE]; ///< PCDCAP2 Buffer

#define NTAG424_SESSION_KEYSIZE 16 ///< Size of auth aes keys in byte

  struct ntag424_SessionType {
    bool authenticated; ///< true = authenticated
    int cmd_counter;    ///< command counter
    uint8_t
        session_key_enc[NTAG424_SESSION_KEYSIZE]; ///< session encryption key
    uint8_t session_key_mac[NTAG424_SESSION_KEYSIZE]; ///< session mac key
  }; ///< struct type foir the authentication session data

  struct ntag424_SessionType
      ntag424_Session; ///< authentication session data are stored here

  struct ntag424_VersionInfoType {
    uint8_t VendorID;       ///< VendorID
    uint8_t HWType;         ///< HWType
    uint8_t HWSubType;      ///< HWSubType
    uint8_t HWMajorVersion; ///< HWMajorVersion
    uint8_t HWMinorVersion; ///< HWMinorVersion
    uint8_t HWStorageSize;  ///< HWStorageSize
    uint8_t HWProtocol;     ///< HWProtocol
    uint8_t SWType;         ///< SWType
    uint8_t SWSubType;      ///< SWSubType
    uint8_t SWMajorVersion; ///< SWMajorVersion
    uint8_t SWMinorVersion; ///< SWMinorVersion
    uint8_t SWStorageSize;  ///< SWStorageSize
    uint8_t SWProtocol;     ///< SWProtocol
    uint8_t UID[7];         ///< UID
    uint8_t BatchNo[5];     ///< BatchNo
    uint8_t FabKey[2];      ///< FabKey
    uint8_t CWProd;         ///< CWProd
    uint8_t YearProd;       ///< YearProd
    uint8_t FabKeyID;       ///< FabKeyID
  }; ///< struct type for ntag424 versioninfo

  struct ntag424_VersionInfoType ntag424_VersionInfo; ///< global version info

  struct ntag424_FileSettings { 
                                ///<  complex :-(
    uint8_t FileType;           ///< FileType
    uint8_t FileOption;         ///< FileOption
    uint8_t AccessRights;       ///< AccessRights
    uint8_t FileSize;           ///< FileSize
    uint8_t SDMOptions;         ///< SDMOptions
    uint8_t SMDAccessRights;    ///< SMDAccessRights
    uint8_t UIDOffset;          ///< UIDOffset
    uint8_t SDMReadCtrOffset;   ///< SDMReadCtrOffset
    uint8_t PICCDataOffset;     ///< PICCDataOffset
    uint8_t TTStatusOffset;     ///< TTStatusOffset
    uint8_t SDMMACInputOffset;  ///< SDMMACInputOffset
    uint8_t SDMENCOffset;       ///< SDMENCOffset
    uint8_t SDMENCLength;       ///< SDMENCLength
    uint8_t SDMMACOffset;       ///< SDMMACOffset
    uint8_t SDMReadCtrlLimit;   ///< SDMReadCtrlLimit
  };  ///<  currently not used. filesettings are more

  // NTAG2xx functions
  uint8_t ntag2xx_ReadPage(uint8_t page, uint8_t *buffer);
  uint8_t ntag2xx_WritePage(uint8_t page, uint8_t *data);
  uint8_t ntag2xx_WriteNDEFURI(uint8_t uriIdentifier, char *url,
                               uint8_t dataLen);

  // Help functions to display formatted text
  static void PrintHex(const byte *data, const uint32_t numBytes);
  static void PrintHexChar(const byte *pbtData, const uint32_t numBytes);

private:
  int8_t _irq = -1, _reset = -1, _cs = -1;
  int8_t _uid[7];      // ISO14443A uid
  int8_t _uidLen;      // uid len
  int8_t _key[6];      // Mifare Classic key
  int8_t _inListedTag; // Tg number of inlisted tag.

  // Low level communication functions that handle both SPI and I2C.
  void readdata(uint8_t *buff, uint8_t n);
  void writecommand(uint8_t *cmd, uint8_t cmdlen);
  bool isready();
  bool waitready(uint16_t timeout);
  bool readack();

  Adafruit_SPIDevice *spi_dev = NULL;
  Adafruit_I2CDevice *i2c_dev = NULL;
  HardwareSerial *ser_dev = NULL;
};

#endif
