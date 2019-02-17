
#ifndef COMPACT_LIST_H
#define COMPACT_LIST_H

#include <stdint.h>
#include <stdlib.h>

/**
 * Compact list.
 *
 * [cl-bytes] [size] [entry] ... [entry] [cl-end]
 * entry: [encoding & data-len] [data] [entry-bytes]
 */
typedef struct {
    uint64_t bytes; //bytes used of the list and entries
    uint32_t size; //element count
} CompactList;

#define CL_END 0xFF;
#define CL_END_BYTES 1;

#define CL_TYPE_INT 1
#define CL_INT4 0x40
#define CL_INT8 0x50
#define CL_INT16 0x60
#define CL_INT32 0x70
#define CL_INT64 0x80

#define CL_TYPE_STR 0
#define CL_STR4 0x00
#define CL_STR8 0x10
#define CL_STR16 0x20
#define CL_STR32 0x34

#define CL_ENTRY_TOTAL 0x70

/**
 * Encoding:
 * [11 111111] end of list
 *
 * string must less than 512MB
 * [00 00 len] string4
 * [00 01 0000] [len] string8
 * [00 10 0000] [len] string16
 * [00 11 0100] [len] * 4 string32
 *
 * [01 00 data] int4
 * [01 01 0000] [data] int8
 * [01 10 0000] [data] * 2 int16
 * [01 01 0000] [data] * 4 int32
 * [01 11 0000] [data] * 8 int64
 *
 * total:
 * [10 xxxxxx] [10 xxxxxx] ...
 */
typedef struct {
    char type;
    char encoding;

    char len[8];
    uint8_t sizeofLen;

    char *data;
    uint32_t sizeofData;

    char total[10];
    int sizeofTotal;
} CompactListNode;


CompactList *CompactListNew();

int64_t CompactListSize(CompactList *list);

char *CompactListGetFirst(CompactList *list);

char *CompactListGetLast(CompactList *list);

CompactList *CompactListAdd(CompactList *list, char *data, size_t dataLen);

CompactList *CompactListRemove(CompactList *list, char *data, size_t len);

#endif //COMPACT_LIST_H
