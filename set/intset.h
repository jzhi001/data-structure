//
// Created by zjc on 13/02/19.
//

#ifndef INTSET_INTSET_H
#define INTSET_INTSET_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct __attribute__((__packed__)) intset
{
    uint8_t eleBytes; //size of one element
    uint32_t size;   //element count
    char elements[];
} ;

struct intset *intsetNew();
size_t intsetSize(struct intset *set);
struct intset *intsetPut(struct intset *set, int64_t val, int *ret);
int intsetContains(struct intset *set, int64_t val);
struct intset *intsetRemove(struct intset *set, int64_t val, int *ret);

inline void intsetFree(struct intset *set){
    free(set);
}


#define HEAD_TO_TAIL 1
#define TAIL_TO_HEAD 0

struct intsetIterator
{
    struct intset *set;
};

struct intsetIterator *intsetIteratorNew(struct intset *set);


#endif //INTSET_INTSET_H
