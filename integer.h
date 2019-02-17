
#ifndef INTEGER_H
#define INTEGER_H

#include <stdint.h>

#define _INT_BYTES(type) (sizeof(int##type##_t))

#define INT8_BYTES _INT_BYTES(8)
#define INT16_BYTES _INT_BYTES(16)
#define INT32_BYTES _INT_BYTES(32)
#define INT64_BYTES _INT_BYTES(64)

/**
 * Get number of bytes to store an integer.
 */
uint8_t bytesForInt(int64_t val);
uint8_t bytesForUnsignedInt(uint64_t val);

int int_setValue(char *pt, int64_t val);
int64_t int_getValue(char *pt, int type);
int string2int(char *str, uint64_t len, int64_t *ret);
uint8_t countDigit(int64_t val);
uint8_t bitCount(int64_t val);

#endif //INTEGER_H
