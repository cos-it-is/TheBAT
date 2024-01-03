Arduino_CRC32
=============

This Arduino library provides a simple interface to perform checksum calculations utilizing the CRC-32 algorithm. The C code for the CRC-32 algorithm was generated using [PyCRC](https://pycrc.org) with the predefined [crc-32 model](https://pycrc.org/models.html#crc-32).

## Usage

```C++
#include <Arduino_CRC32.h>
/* ... */
Arduino_CRC32 crc32;
/* ... */
char const str[] = "Hello CRC32 ;)";
uint32_t const crc32_res = crc32.calc((uint8_t const *)str, strlen(str));
```
