#ifndef TINYCMAC_H
#define TINYCMAC_H
#include "Arduino.h"
#include "mbedtls/aes.h"

void xorBlock(uint8_t *output, const uint8_t *input1, const uint8_t *input2,
              size_t len);

void AES128_CMAC(const uint8_t *key, const uint8_t *input, size_t length,
                 uint8_t output[16]);
#endif

void aes_encrypt(uint8_t *key, uint8_t *iv, uint8_t length, uint8_t *input,
                 uint8_t *output);

void PrintHex(const byte *data, const uint32_t numBytes);
