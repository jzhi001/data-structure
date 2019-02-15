
#ifndef INTSET_INT_SET_H
#define INTSET_INT_SET_H

#include "int_vector.h"

/**
 * Integer set. Implement by ordered int vector.
 */
typedef IntVector IntSet;

IntSet *IntSetNew();
uint32_t IntSetSize(IntSet *set);
int IntSetIsEmpty(IntSet *set);
IntSet *IntSetPut(IntSet *set, int64_t val, int *ret);
int IntSetContains(IntSet *set, int64_t val);
IntSet *IntSetRemove(IntSet *set, int64_t val, int *ret);

typedef IntVectorIterator IntSetIterator;

IntSetIterator *IntSetIteratorNew(IntSet *set);
int IntSetIteratorHasNext(IntSetIterator *iter);
int64_t IntSetIteratorNext(IntSetIterator *iter);

#endif //INTSET_INT_SET_H
