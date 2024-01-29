#include "aescmac.h"

// Helper function to XOR two blocks
void xor_block(uint8_t *output, const uint8_t *input1, const uint8_t *input2,
               size_t len) {
  for (size_t i = 0; i < len; ++i) {
    output[i] = input1[i] ^ input2[i];
  }
}

// Helper function to shift a block to the left by one bit
void shift_left(uint8_t *output, const uint8_t *input, size_t len) {
  uint8_t overflow = 0;
  for (int i = len - 1; i >= 0; i--) {
    output[i] = input[i] << 1 | overflow;
    overflow = (input[i] & 0x80) ? 1 : 0;
  }
}
void PrintHex(const byte *data, const uint32_t numBytes) {
  uint32_t szPos;
  for (szPos = 0; szPos < numBytes; szPos++) {
    Serial.print(F("0x"));
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      Serial.print(F("0"));
    Serial.print(data[szPos] & 0xff, HEX);
    if ((numBytes > 1) && (szPos != numBytes - 1)) {
      Serial.print(F(" "));
    }
  }
  Serial.println();
}

void AES128_CMAC(const uint8_t key[16], const uint8_t *input, size_t length,
                 uint8_t output[16]) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, key, 128);

  uint8_t k1[16] = {0}, k2[16] = {0};
  uint8_t buffer[16] = {0};

  // Generate subkeys k1 and k2
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, buffer, buffer);
  shift_left(k1, buffer, 16);
  if (buffer[0] & 0x80) {
    k1[15] ^= 0x87;
  }
  shift_left(k2, k1, 16);
  if (k1[0] & 0x80) {
    k2[15] ^= 0x87;
  }

  // Process the message blocks
  size_t n = (length + 15) / 16;
  bool lastBlockComplete = (length % 16 == 0);
  if (n == 0) {
    n = 1;
    lastBlockComplete = false;
  }

  uint8_t lastBlock[16];
  if (lastBlockComplete) {
    xor_block(lastBlock, &input[(n - 1) * 16], k1, 16);
  } else {
    size_t lastBlockSize = length % 16;
    memset(lastBlock, 0, 16);
    memcpy(lastBlock, &input[(n - 1) * 16], lastBlockSize);
    lastBlock[lastBlockSize] = 0x80;
    xor_block(lastBlock, lastBlock, k2, 16);
  }

  uint8_t x[16] = {0};
  for (size_t i = 0; i < n - 1; i++) {
    xor_block(buffer, x, &input[i * 16], 16);
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, buffer, x);
  }

  xor_block(buffer, lastBlock, x, 16);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, buffer, output);

  mbedtls_aes_free(&ctx);
  Serial.println("");
  Serial.print("output:");
  PrintHex(output, 16);
}
