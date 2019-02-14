#include "intset.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static inline void panic(char *msg) {
    printf("%s\n", msg);
    exit(1);
}

#define INT_BYTES(type) (sizeof(int##type##_t))

struct intset *intsetNew() {
    struct intset *set;
    if ((set = malloc(sizeof(*set))) == NULL) {
        panic("intset malloc failed");
    }
    set->eleBytes = INT_BYTES(8);
    set->size = 0;
    return set;
}

static inline void setElementBytes(struct intset *set, uint8_t bytes) {
    set->eleBytes = bytes;
}

static inline uint8_t getElementBytes(struct intset *set) {
    return set->eleBytes;
}

static inline void setIntsetSize(struct intset *set, uint32_t size) {
    set->size = size;
}


static inline size_t elementTotalBytes(struct intset *set) {
    return getElementBytes(set) * set->size;
}

static inline size_t headerBytes() {
    return sizeof(struct intset);
}

static inline size_t totalBytes(struct intset *set) {
    return headerBytes() + elementTotalBytes(set);
}

static inline uint8_t bytesForValue(int64_t val) {
    if (val <= INT8_MAX && val >= INT8_MIN) {
        return INT_BYTES(8);
    } else if (val <= INT16_MAX && val >= INT16_MIN) {
        return INT_BYTES(16);
    } else if (val <= INT32_MAX && val >= INT32_MIN) {
        return INT_BYTES(32);
    } else {
        return INT_BYTES(64);
    }
}

static inline int64_t elementLastIdx(struct intset *set) {
    return intsetSize(set) - 1;
}

static inline char *prevElement(char *ele, struct intset *set) {
    return ele - getElementBytes(set);
}

static inline char *nextElement(char *ele, struct intset *set) {
    return ele + getElementBytes(set);
}

static inline char *firstElement(struct intset *set) {
    return set->elements;
}

static inline void validIndex(struct intset *set, int64_t idx) {
    if (idx < 0 || idx >= intsetSize((set))) {
        panic("index out of range");
    }
}

static inline char *elementAtIdxByType(struct intset *set, int64_t idx, uint8_t bytes) {
    return firstElement(set) + idx * bytes;
}

static inline char *elementAt(struct intset *set, int64_t idx) {
    return elementAtIdxByType(set, idx, getElementBytes(set));
}

static inline char *lastElement(struct intset *set) {
    return firstElement(set) + elementLastIdx(set) * getElementBytes(set);
}

static inline int64_t getValueAtIdxByType(struct intset *set, int64_t idx, uint8_t bytes) {
    validIndex(set, idx);
    char *ele = elementAtIdxByType(set, idx, bytes);
    switch (bytes) {
        case 1:
            return ((int8_t *) ele)[0];
            break;
        case 2:
            return ((int16_t *) ele)[0];
            break;
        case 4:
            return ((int32_t *) ele)[0];
            break;
        case 8:
            return ((int64_t *) ele)[0];
            break;
        default:
            printf("unknown bytes: %d\n", bytes);
            panic("");
            break;
    }
}

static inline int64_t getValueAt(struct intset *set, int64_t idx) {
    return getValueAtIdxByType(set, idx, getElementBytes(set));
}

static inline int64_t getFirstValue(struct intset *set) {
    return getValueAt(set, 0);
}

static inline int64_t getLastValue(struct intset *set) {
    return getValueAt(set, elementLastIdx(set));
}

static inline void setValueAt(struct intset *set, int64_t idx, int64_t val) {
    char *ele = elementAt(set, idx);
    switch (getElementBytes(set)) {
        case 1:
            ((int8_t *) ele)[0] = (int8_t) val;
            break;
        case 2:
            ((int16_t *) ele)[0] = (int16_t) val;
            break;
        case 4:
            ((int32_t *) ele)[0] = (int32_t) val;
            break;
        case 8:
            ((int64_t *) ele)[0] = (int64_t) val;
            break;
        default:
            panic("unknown bytes for value");
            break;
    }
}

static inline struct intset *resize(struct intset *set, size_t size) {
    if ((set = realloc(set, size)) == NULL) {
        panic("intset realloc failed");
    }
    return set;
}

/**
 * Make room for 1 element.
 * Note: Call upgrade() before calling this function.
 **/
static struct intset *makeRoomForOne(struct intset *set) {
    size_t bytes = totalBytes(set) + getElementBytes(set);
    set = resize(set, bytes);
    return set;
}

static struct intset *upgradeIfNeeded(struct intset *set, uint8_t bytes) {
    uint8_t oldBytes = getElementBytes(set);

    if (bytes > oldBytes) {
        setElementBytes(set, bytes);
        set = resize(set, totalBytes(set));

        int64_t idx = elementLastIdx(set);
        while (idx >= 0) {
            int64_t val = getValueAtIdxByType(set, idx, oldBytes);
            setValueAt(set, idx, val);
            idx--;
        }
    }
    return set;
}

static int64_t binarySearch(struct intset *set, int64_t x) {
    uint32_t size = set->size;
    if (size == 0) {
        return -1;
    }
    int64_t lf = 0, rt = size - 1;

    // printf("bisearch\n");
    while (lf <= rt) {
        // printf("lf: %ld, rt: %ld\n", lf, rt);
        if (lf == rt) {
            int64_t val = getValueAt(set, lf);
            return val == x ? lf : -1;
        } else {
            int64_t mid = (lf + rt) / 2;

            int64_t midVal = getValueAt(set, mid);
            if (x > midVal) {
                lf = mid + 1;
            } else if (x < midVal) {
                rt = mid - 1;
            } else {
                return mid;
            }
        }
    }
    return -1;
}

//TODO O(N) time now
static inline int64_t where2Insert(struct intset *set, int64_t val) {
    int64_t i = 0;
    while (i < set->size && getValueAt(set, i) < val) {
        i++;
    }
    return i;
}

static inline void shiftOneStepAtRight(struct intset *set, int64_t start, int64_t end) {
    int64_t i = end;
    if (end + 1 < set->size) {
        panic("No extra space for shifting");
    }
    while (i >= start) {
        int64_t val = getValueAt(set, i);
        setValueAt(set, i + 1, val);
        i--;
    }
}

static inline void shiftOneStepLeft(struct intset *set, uint32_t start, uint32_t end) {
    size_t bytes = (end - start + 1) * getElementBytes(set);
    memcpy(elementAt(set, start - 1), elementAt(set, start), bytes);
}

/* --------------------------------- API ----------------------------------- */

int intsetContains(struct intset *set, int64_t val) {
    if (set->size == 0 || getElementBytes(set) < bytesForValue(val)) {
        return 0;
    } else {
        int64_t idx = binarySearch(set, val);
        return idx == -1 ? 0 : 1;
    }
}

struct intset *intsetPut(struct intset *set, int64_t val, int *ret) {
    set = upgradeIfNeeded(set, bytesForValue(val));

    if (intsetContains(set, val)) {
        if (ret)
            *ret = 0;
    } else {
        if (ret)
            *ret = 1;
        set = makeRoomForOne(set);
        int64_t i = where2Insert(set, val);
        printf("intset insert at %ld\n", i);
        if (set->size > 0 && i < set->size) {
            shiftOneStepAtRight(set, i, elementLastIdx(set));
        }
        setValueAt(set, i, val);
        set->size++;
    }
    return set;
}

size_t intsetSize(struct intset *set) {
    return set->size;
}

struct intset *intsetRemove(struct intset *set, int64_t val, int *ret) {
    int64_t idx = binarySearch(set, val);

    // printf("remove [%ld]\n", idx);
    if (idx == -1) {
        if (ret)
            *ret = 0;
    } else {
        if (ret)
            *ret = 1;
        if (idx != elementLastIdx(set)) {
            shiftOneStepLeft(set, idx + 1, elementLastIdx(set));
        }
        set = resize(set, totalBytes(set) - getElementBytes(set));
        set->size--;
    }
    return set;
}

#define INTSET_TEST
#ifdef INTSET_TEST

#include <assert.h>

#define TEST_PUT(type)                       \
    do                                       \
    {                                        \
        val = INT##type##_MAX;               \
        size = intsetSize(set);              \
        set = intsetPut(set, val, &ret);     \
        assert(set->eleBytes == (type) / 8); \
        assert(intsetSize(set) == size + 1); \
        assert(intsetContains(set, val));    \
    } while (0);

#define TEST_REMOVE(type)                    \
    do                                       \
    {                                        \
        val = INT##type##_MAX;               \
        size = intsetSize(set);              \
        set = intsetRemove(set, val, &ret);  \
        assert(ret == 1);                    \
        assert(intsetSize(set) == size - 1); \
    } while (0);

static inline void putsElementAt(struct intset *set, uint32_t idx) {
    printf("set[%d] = %ld\n", idx, getValueAt(set, idx));
}

static void showBytes(struct intset *set) {
    size_t bytes = elementTotalBytes(set);
    char *ele = firstElement(set);
    for (int i = 0; i < bytes; i++) {
        printf("%x ", *ele);
        ele++;
    }
    printf("\n");
}

static void intsetPutAndRemoveTest() {
    struct intset *set = intsetNew();
    int ret = 0;
    int64_t val;
    uint32_t size;

    TEST_PUT(8);
    TEST_PUT(16);
    TEST_PUT(32);
    TEST_PUT(64);

    TEST_REMOVE(8);
    TEST_REMOVE(16);
    TEST_REMOVE(32);
    TEST_REMOVE(64);
}

int main() {
    // intsetPutAndRemoveTest();

    struct intset *set = intsetNew();
    for (int i = 20; i > 0; i--) {
        intsetPut(set, i, NULL);
        showBytes(set);
    }
    return 0;
}

#endif