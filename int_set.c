
#include "int_set.h"

inline IntSet *IntSetNew(){
    return IntVectorNew();
}

inline void IntSetFree(IntSet *set){
    IntVectorFree(set);
}

inline uint32_t IntSetSize(IntSet *set){
    return IntVectorSize(set);
}

inline int IntSetIsEmpty(IntSet *set){
    return IntVectorIsEmpty(set);
}

int IntSetContains(IntSet *set, int64_t val){
    return IntVectorIndexOf(set, val) != -1;
}

static inline IntSet *intset_insertByOrder(IntSet *set, int64_t val){
    int64_t i = 0;
    while (i < set->size && IntVectorValueAt(set, i) < val) {
        i++;
    }
    set = IntVectorInsert(set, val, i);
    return set;
}

IntSet *IntSetPut(IntSet *set, int64_t val, int *ret){
    int64_t idx = IntVectorBinarySearch(set, val);
    if(idx != -1){
        if(ret) *ret = 0;
    }else{
        if(ret) *ret = 1;
        set = intset_insertByOrder(set, val);
    }
    return set;
}

IntSet *IntSetRemove(IntSet *set, int64_t val, int *ret){
    int64_t idx = IntVectorBinarySearch(set, val);
    if(idx != -1){
        if(ret) *ret = 1;
        set = IntVectorRemoveAt(set, idx);
    }else{
        if(ret) *ret = 0;
    }
    return set;
}

inline IntSetIterator *IntSetIteratorNew(IntSet *set){
    return IntVectorIteratorNew(set);
}
inline int IntSetIteratorHasNext(IntSetIterator *iter){
    return IntVectorIteratorHasNext(iter);
}
inline int64_t IntSetIteratorNext(IntSetIterator *iter){
    return IntVectorIteratorNext(iter);
}

//#define INT_SET_TEST
#ifdef INT_SET_TEST
#include <assert.h>
#include <stdio.h>

int main(){
    IntSet *set = IntSetNew();
    int ret;

    for(int i=10; i>=0; i--){
//        printf("%d\n", i);
        set = IntSetPut(set, i, &ret);
        assert(1 == ret);
    }

    IntSetIterator *iter = IntSetIteratorNew(set);
    int correct = 0;
    while(IntSetIteratorHasNext(iter)){
        assert(correct == IntSetIteratorNext(iter));
        correct++;
    }

    for(int i=10; i>=0; i--){
//        printf("%d\n", i);
        set = IntSetRemove(set, i, &ret);
        assert(1 == ret);
    }
    assert(IntVectorIsEmpty(set));
    return 0;
}
#endif