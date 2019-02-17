#include "int_vector.h"
#include "panic.h"
#include "integer.h"
#include <string.h>

static inline size_t iv_headerBytes() {
    return sizeof(IntVector);
}


inline uint32_t IntVectorSize(IntVector *vector) {
    return vector->size;
}

static inline void iv_setSize(IntVector *vector, uint32_t size) {
    vector->size = size;
}

inline int IntVectorIsFull(IntVector *vector) {
    return IntVectorSize(vector) == INT_VECTOR_MAX_SIZE;
}

inline int IntVectorIsEmpty(IntVector *vector) {
    return IntVectorSize(vector) == 0;
}

static inline uint8_t iv_getEncoding(IntVector *vector) {
    return vector->encoding;
}

static inline void iv_setEncoding(IntVector *vector, uint8_t enc) {
    vector->encoding = enc;
}

static inline size_t iv_totalBytes(IntVector *vector) {
    return iv_headerBytes() + IntVectorSize(vector) * iv_getEncoding(vector);
}


static inline uint8_t iv_encodingOf(int64_t val) {
    return bytesForInt(val);
}

static inline char *iv_firstElement(IntVector *vector) {
    return vector->elements;
}

static inline char *iv_elementAtIdxByType(IntVector *vector, int64_t idx, uint8_t encoding) {
    return iv_firstElement(vector) + idx * encoding;
}

static inline char *iv_elementAt(IntVector *vector, int64_t idx) {
    return iv_elementAtIdxByType(vector, idx, iv_getEncoding(vector));
}


static inline void iv_validIndex(IntVector *vector, int64_t idx) {
    if (idx < 0 || IntVectorIsEmpty(vector) || idx >= IntVectorSize(vector)) {
        panic("index out of range: %ld\n", idx);
    }
}

static inline int64_t iv_lastIdx(IntVector *vector) {
    return (int64_t) IntVectorSize(vector) - 1;
}

static inline int64_t iv_valueAtIdxByEncoding(IntVector *vector, int64_t idx, uint8_t encoding) {
    iv_validIndex(vector, idx);
    char *ele = iv_elementAtIdxByType(vector, idx, encoding);
    return int_getValue(ele, encoding);
}

static inline int64_t iv_valueAt(IntVector *vector, int64_t idx) {
    return iv_valueAtIdxByEncoding(vector, idx, iv_getEncoding(vector));
}


static inline void iv_setValueAt(IntVector *vector, int64_t idx, int64_t val) {
    char *ele = iv_elementAt(vector, idx);
    int_setValue(ele, val);
}

static IntVector *iv_resize(IntVector *vector, size_t size) {
    if ((vector = realloc(vector, size)) == NULL) {
        panic("IntVector realloc failed\n");
    }
    return vector;
}

static IntVector *iv_makeRoom(IntVector *vector, int64_t n) {
    return iv_resize(vector, iv_totalBytes(vector) + n * iv_getEncoding(vector));
}

static IntVector *iv_upgradeIfNeeded(IntVector *vector, uint8_t valEnc) {
    uint8_t curEnc = iv_getEncoding(vector);
    if (valEnc > curEnc) {
        iv_setEncoding(vector, valEnc);
        vector = iv_resize(vector, iv_totalBytes(vector));

        int64_t idx = iv_lastIdx(vector);
        while (idx >= 0) {
            int64_t val = iv_valueAtIdxByEncoding(vector, idx, curEnc);
            iv_setValueAt(vector, idx, val);
            idx--;
        }
    }
    return vector;
}

IntVector *IntVectorNew() {
    IntVector *vector;
    if ((vector = malloc(iv_headerBytes())) == NULL) {
        panic("IntVector malloc failed");
    }
    iv_setSize(vector, 0);
    iv_setEncoding(vector, INT8_BYTES);
    return vector;
}

inline void IntVectorFree(IntVector *vector){
    free(vector);
}

IntVector *IntVectorSetValueAt(IntVector *vector, int64_t val, int64_t idx) {
    if (idx < 0 || idx >= INT_VECTOR_MAX_SIZE - 1) {
        panic("idx is negative or greater than capacity: %ld\n", idx);
    }
    vector = iv_upgradeIfNeeded(vector, iv_encodingOf(val));
    if (idx <= iv_lastIdx(vector)) {
        iv_setValueAt(vector, idx, val);
    } else {
        int64_t i = iv_lastIdx(vector) + 1;
        iv_setSize(vector, (uint32_t) (idx + 1));
        vector = iv_resize(vector, iv_totalBytes(vector));
        while (i < idx) {
            iv_setValueAt(vector, i, 0);
            i++;
        }
        iv_setValueAt(vector, idx, val);
    }
    return vector;
}

inline IntVector *IntVectorAppend(IntVector *vector, int64_t val) {
    return IntVectorSetValueAt(vector, val, iv_lastIdx(vector) + 1);
}

int64_t IntVectorValueAt(IntVector *vector, int64_t idx) {
    iv_validIndex(vector, idx);
    return iv_valueAt(vector, idx);
}

int64_t IntVectorIndexOf(IntVector *vector, int64_t val) {
    if (IntVectorSize(vector) == 0) {
        return -1;
    }
    for (int i = 0; i < IntVectorSize(vector); i++) {
        if (iv_valueAt(vector, i) == val) {
            return i;
        }
    }
    return -1;
}

static inline void iv_shiftOneStepAtRight(IntVector *vector, int64_t start, int64_t end) {
    iv_validIndex(vector, start);
    iv_validIndex(vector, end);

    int64_t i = end;
    if (end + 1 >= IntVectorSize(vector)) {
        panic("No extra space for shifting right: start %ld end %ld size %ld\n", start, end, IntVectorSize(vector));
    }
    while (i >= start) {
        int64_t val = IntVectorValueAt(vector, i);
        IntVectorSetValueAt(vector, val, i + 1);
        i--;
    }
}

IntVector *IntVectorInsert(IntVector *vector, int64_t val, int64_t idx) {
    if (idx < 0 || idx >= INT_VECTOR_MAX_SIZE - 1) {
        panic("idx is negative or greater than capacity: %ld\n", idx);
    } else if (IntVectorIsFull(vector)) {
        panic("IntVector is full\n");
    }
//    printf("insert %ld at %ld, size %d\n", val, idx, IntVectorSize(vector));
    if (idx > iv_lastIdx(vector)) {
        return IntVectorSetValueAt(vector, val, idx);
    } else {
        vector = iv_makeRoom(vector, 1);
        int64_t tail = iv_lastIdx(vector);
        iv_setSize(vector, IntVectorSize(vector) + 1);
        iv_shiftOneStepAtRight(vector, idx, tail);
        iv_setValueAt(vector, idx, val);
        return vector;
    }
}

IntVector *IntVectorPrepend(IntVector *vector, int64_t val) {
    return IntVectorInsert(vector, val, 0);
}

static inline void iv_shiftOneStepLeft(IntVector *vector, int64_t start, int64_t end) {
    iv_validIndex(vector, start);
    iv_validIndex(vector, end);

    size_t bytes = (end - start + 1) * (size_t) iv_getEncoding(vector);
    memcpy(iv_elementAt(vector, start - 1), iv_elementAt(vector, start), bytes);
}

IntVector *IntVectorRemoveAt(IntVector *vector, int64_t idx) {
    iv_validIndex(vector, idx);

    if (idx < iv_lastIdx(vector)) {
        iv_shiftOneStepLeft(vector, idx + 1, iv_lastIdx(vector));
    }
    iv_setSize(vector, IntVectorSize(vector) - 1);
    return iv_resize(vector, iv_totalBytes(vector));
}

IntVector *IntVectorRemove(IntVector *vector, int64_t val, int *success) {
    int64_t idx = IntVectorIndexOf(vector, val);
    if (idx == -1) {
        if (success) *success = 0;
    } else {
        if (success) *success = 1;
        vector = IntVectorRemoveAt(vector, idx);
    }
    return vector;
}

IntVector *IntVectorRemoveHead(IntVector *vector, int64_t *val) {
    if (IntVectorIsEmpty(vector)) {
        panic("IntVector remove from empty vector\n");
    }
    if (val) *val = IntVectorValueAt(vector, 0);
    return IntVectorRemoveAt(vector, 0);
}

IntVector *IntVectorRemoveTail(IntVector *vector, int64_t *val) {
    if (IntVectorIsEmpty(vector)) {
        panic("IntVector remove from empty vector\n");
    }
    if (val) *val = IntVectorValueAt(vector, iv_lastIdx(vector));
    return IntVectorRemoveAt(vector, iv_lastIdx(vector));
}

int64_t IntVectorBinarySearch(IntVector *vector, int64_t x) {
    if (IntVectorIsEmpty(vector)) {
        return -1;
    }

    int64_t lf = 0, rt = iv_lastIdx(vector);

    while (lf <= rt) {
        if (lf == rt) {
            int64_t val = iv_valueAt(vector, lf);
            return val == x ? lf : -1;
        } else {
            int64_t mid = (lf + rt) / 2;

            int64_t midVal = iv_valueAt(vector, mid);
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

#define IV_ITER_HEAD 1
#define IV_ITER_TAIL 0

static IntVectorIterator *iv_iter_new(IntVector *vector, int direction) {
    IntVectorIterator *iter;
    if ((iter = malloc(sizeof(*iter))) == NULL) {
        panic("IntVector Iterator malloc failed\n");
    }
    iter->vector = vector;
    iter->direction = direction;
    iter->curIdx = -1;
    return iter;
}

IntVectorIterator *IntVectorIteratorNew(IntVector *vector) {
    return iv_iter_new(vector, IV_ITER_HEAD);
}

IntVectorIterator *IntVectorReverseIteratorNew(IntVector *vector) {
    return iv_iter_new(vector, IV_ITER_TAIL);
}

int IntVectorIteratorHasNext(IntVectorIterator *iter) {
    if (IntVectorIsEmpty(iter->vector)) {
        return 0;
    } else {
        if (iter->direction == IV_ITER_HEAD) {
            return iter->curIdx < iv_lastIdx(iter->vector);
        } else {
            return iter->curIdx >= 1;
        }
    }
}

int64_t IntVectorIteratorNext(IntVectorIterator *iter) {
    if (iter->direction == IV_ITER_HEAD) {
        iter->curIdx++;
    } else {
        iter->curIdx--;
    }
    return iv_valueAt(iter->vector, iter->curIdx);
}

//#define INT_VECTOR_TEST
#ifdef INT_VECTOR_TEST

#include <assert.h>

int main() {
    IntVector *vector = IntVectorNew();

    for(int i=0; i<10; i++){
        vector = IntVectorAppend(vector, i);
        assert(IntVectorValueAt(vector, i) == i);
    }
    assert(IntVectorSize(vector) == 10);

    assert(9 == IntVectorBinarySearch(vector, 9));
    assert(0 == IntVectorBinarySearch(vector, 0));
    assert(-1 == IntVectorBinarySearch(vector, 100));

    IntVectorIterator *iter = IntVectorIteratorNew(vector);
    int correct = 0;
    while (IntVectorIteratorHasNext(iter)){
        int64_t x = IntVectorIteratorNext(iter);
        assert(correct == x);
        correct++;
    }

    correct = 9;
    iter = IntVectorReverseIteratorNew(vector);
    while (IntVectorIteratorHasNext(iter)){
        int64_t x = IntVectorIteratorNext(iter);
        assert(correct == x);
        correct--;
    }

    int succ = 0;
    for(int i=0; i<10; i++){
        vector = IntVectorRemove(vector, i, &succ);
        assert(succ == 1);
        assert(IntVectorIndexOf(vector, i) == -1);
    }
    assert(IntVectorIsEmpty(vector));

    vector = IntVectorAppend(vector, INT8_MAX);
    assert(INT8_MAX == IntVectorValueAt(vector, 0));
    vector = IntVectorAppend(vector, INT16_MAX);
    assert(INT16_MAX == IntVectorValueAt(vector, 1));
    vector = IntVectorAppend(vector, INT32_MAX);
    assert(INT32_MAX == IntVectorValueAt(vector, 2));
    vector = IntVectorAppend(vector, INT64_MAX);
    assert(INT64_MAX == IntVectorValueAt(vector, 3));

    int64_t val = 0;
    vector = IntVectorRemoveHead(vector, &val);
    assert(val == INT8_MAX);
    vector = IntVectorRemoveTail(vector, &val);
    assert(val == INT64_MAX);
}

#endif
