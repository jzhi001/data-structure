

#ifndef INT_VECTOR_H
#define INT_VECTOR_H

#include <glob.h>
#include <stdint.h>
/**
 * IntVector is a COMPACTED dynamic sized array, which
 * means size is always equals to capacity.
 */

typedef struct __attribute__((__packed__)){
    uint8_t encoding; // sizeof one element
    uint32_t size; // element count
    char elements[];
} IntVector;


#define INT_VECTOR_MAX_SIZE UINT32_MAX

IntVector *IntVectorNew();
uint32_t IntVectorSize(IntVector *vector);
int IntVectorIsEmpty(IntVector *vector);
int IntVectorIsFull(IntVector *vector);

int64_t IntVectorIndexOf(IntVector *vector, int64_t val);

IntVector *IntVectorSetValueAt(IntVector *vector, int64_t val, int64_t idx);
IntVector *IntVectorInsert(IntVector *vector, int64_t val, int64_t idx);
IntVector *IntVectorAppend(IntVector *vector, int64_t val);
IntVector *IntVectorPrepend(IntVector *vector, int64_t val);

int64_t IntVectorValueAt(IntVector *vector, int64_t idx);

IntVector *IntVectorRemove(IntVector *vector, int64_t val, int *success);
IntVector *IntVectorRemoveAt(IntVector *vector, int64_t idx);
IntVector *IntVectorRemoveHead(IntVector *vector, int64_t *val);
IntVector *IntVectorRemoveTail(IntVector *vector, int64_t *val);

int64_t IntVectorBinarySearch(IntVector *vector, int64_t x);

typedef struct{
    IntVector *vector;
    int direction;
    int64_t curIdx;
} IntVectorIterator;


IntVectorIterator *IntVectorIteratorNew(IntVector *vector);
IntVectorIterator *IntVectorReverseIteratorNew(IntVector *vector);
int IntVectorIteratorHasNext(IntVectorIterator *iter);
int64_t IntVectorIteratorNext(IntVectorIterator *iter);

#endif //INT_VECTOR_H
