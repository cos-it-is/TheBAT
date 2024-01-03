# Adafruit-PN532-NTAG424

This is a fork of the library for the Adafruit PN532 NFC/RFID breakout boards.
It adds some support for communication with NTAG424 tags.
Currently this works only for ESP32, due to the use of mbedtls.
Check the additional methods prefixed with ntag424_.
Also check ntag424_examples.ino in the examples folder.
Tested with ESP32-D0WDQ6-V3 (TTGO T-Display) and Arduino 1.8.13.

Additional libraries:
In Arduino-ESP32 the mbedtls-cmac library has been excluded, and it is needed to sign the apdus.
It is in the libraries/mbedtlscmac folder, copy the folder into your libraries folder.
Its basically a copy of https://github.com/Mbed-TLS/mbedtls/blob/v2.16.7/include/mbedtls/cmac.h and https://raw.githubusercontent.com/Mbed-TLS/mbedtls/v2.16.7/library/cmac.c




# Adafruit-PN532


This is a library for the Adafruit PN532 NFC/RFID breakout boards
This library works with the Adafruit NFC breakout

  * https://www.adafruit.com/products/364

Check out the links above for our tutorials and wiring diagrams
These chips use I2C or SPI to communicate.

# Dependencies
* [Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)


# Contributing

Contributions are welcome! Please read our [Code of Conduct](https://github.com/adafruit/Adafruit-PN532/blob/master/CODE_OF_CONDUCT.md>)
before contributing to help this project stay welcoming.

## Documentation and doxygen
Documentation is produced by doxygen. Contributions should include documentation for any new code added.

Some examples of how to use doxygen can be found in these guide pages:

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen-tips

## Formatting and clang-format
This library uses [`clang-format`](https://releases.llvm.org/download.html) to standardize the formatting of `.cpp` and `.h` files.
Contributions should be formatted using `clang-format`:

The `-i` flag will make the changes to the file.
```bash
clang-format -i *.cpp *.h
```
If you prefer to make the changes yourself, running `clang-format` without the `-i` flag will print out a formatted version of the file. You can save this to a file and diff it against the original to see the changes.

Note that the formatting output by `clang-format` is what the automated formatting checker will expect. Any diffs from this formatting will result in a failed build until they are addressed. Using the `-i` flag is highly recommended.

### clang-format resources
  * [Binary builds and source available on the LLVM downloads page](https://releases.llvm.org/download.html)
  * [Documentation and IDE integration](https://clang.llvm.org/docs/ClangFormat.html)
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada & Kevin Townsend for Adafruit Industries.
BSD license, check license.txt for more information
All text above must be included in any redistribution

To install, use the Arduino Library Manager and search for "Adafruit PN532" and install the library. Adafruit BusIO library is also required
