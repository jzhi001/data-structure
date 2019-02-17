
#include "integer.h"
#include <ctype.h>
#include "panic.h"

inline uint8_t bytesForInt(int64_t val) {
    if (val <= INT8_MAX && val >= INT8_MIN) {
        return INT8_BYTES;
    }
    if (val <= INT16_MAX && val >= INT16_MIN) {
        return INT16_BYTES;
    }
    if (val <= INT32_MAX && val >= INT32_MIN) {
        return INT32_BYTES;
    } else {
        return INT64_BYTES;
    }
}

inline uint8_t bytesForUnsignedInt(uint64_t val) {
    if (val <= UINT8_MAX) {
        return INT8_BYTES;
    } else if (val <= UINT16_MAX) {
        return INT16_BYTES;
    } else if (val <= UINT32_MAX) {
        return INT32_BYTES;
    } else {
        return INT64_BYTES;
    }

}

inline int int_setValue(char *pt, int64_t val) {
    int bytes = bytesForInt(val);
    switch (bytes) {
        case INT8_BYTES:
            ((int8_t *) pt)[0] = (int8_t) val;
            break;
        case INT16_BYTES:
            ((int16_t *) pt)[0] = (int16_t) val;
            break;
        case INT32_BYTES:
            ((int32_t *) pt)[0] = (int32_t) val;
            break;
        case INT64_BYTES:
            ((int64_t *) pt)[0] = (int64_t) val;
            break;
        default:
            panic("unknown encoding: %d\n", bytes);
            break;
    }
    return bytes;
}

inline int64_t int_getValue(char *pt, int type) {
    switch (type) {
        case INT8_BYTES:
            return ((int8_t *) pt)[0];

        case INT16_BYTES:
            return ((int16_t *) pt)[0];

        case INT32_BYTES:
            return ((int32_t *) pt)[0];

        case INT64_BYTES:
            return ((int64_t *) pt)[0];

        default:
            panic("unknown bytes: %d\n", type);
            break;
    }
}

int string2int(char *str, uint64_t len, int64_t *ret) {
    if (len >= UINT8_MAX) {
        return 0;
    }
    int64_t result = 0, weight = 1;

    for (int64_t i = len - 1; i >= 0; i--) {
        char c = str[i];
        if (isdigit(c)) {
            result += weight * (c - '0');
            weight *= 10;
        } else if (c == '-' && i == 0) {
            result = -result;
        } else {
            return 0;
        }
    }

    if (ret) *ret = result;
    return 1;
}

uint8_t countDigit(int64_t val) {
    uint8_t ret = 0;
    while (val != 0) {
        ret++;
        val /= 10;
    }
    return ret;
}

uint8_t bitCount(int64_t val) {
    uint8_t ret = 0;
    while (val != 0) {
        ret++;
        val >>= 1;
    }
    return ret;
}


//#define INTEGER_TEST
#ifdef INTEGER_TEST

#include <assert.h>

int main(){
    int64_t  ret, success;

    success = string2int("12345", 5, &ret);
    assert(success && ret == 12345);

    success = string2int("-12345", 6, &ret);
    assert(success && ret == -12345);

    success = string2int("abc", 3, &ret);
    assert(!success);
    return 0;
}
#endif