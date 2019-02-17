
#include <string.h>
#include <assert.h>
#include "integer.h"
#include "compact_list.h"
#include "panic.h"

inline int64_t CompactListSize(CompactList *list) {
    return list->size;
}

static inline size_t cl_headerBytes() {
    return sizeof(CompactList);
}

static inline size_t cl_emptyListBytes() {
    return cl_headerBytes() + CL_END_BYTES;
}


CompactList *CompactListNew() {
    CompactList *list;
    if ((list = malloc(cl_emptyListBytes())) == NULL) {
        panic("CompactList malloc failed\n");
    }
    list->size = 0;
    list->bytes = cl_emptyListBytes();
    ((char *) list)[cl_headerBytes()] = (char) CL_END;
    return list;
}

static int cl_getEntryType(char encoding) {
    return encoding >> 6;
}

#define SET_INT_ENC_AND_DATA(intType) do { \
    enc = CL_INT ## intType; \
    int ## intType ##_t *pt = (int ## intType ##_t*)(encoding + 1); \
    pt[0] = (int ## intType ##_t)val; \
    len += INT ## intType ## _BYTES; \
} while(0);

static inline size_t cl_setIntEncodingAndData(int64_t val, char *encoding) {
    assert(encoding);

    uint8_t bits = bitCount(val);
    char enc;
    size_t len = 1;

    if (bits <= 4) {
        enc = (char) (CL_INT4 | val);
    } else if (bits <= 8 * INT8_BYTES) {
        SET_INT_ENC_AND_DATA(8);
    } else if (bits <= 8 * INT16_BYTES) {
        SET_INT_ENC_AND_DATA(16);
    } else if (bits <= 8 * INT32_BYTES) {
        SET_INT_ENC_AND_DATA(32);
    } else {
        SET_INT_ENC_AND_DATA(64);
    }
    encoding[0] = enc;
    return len;
}

static void cl_strNode_getEncoding(CompactListNode *node) {
    assert(node->type == CL_TYPE_STR);
    size_t dataLen = node->sizeofData;
    uint8_t dataLenbits = bitCount(dataLen);

    if (dataLenbits <= 4) {
        node->encoding = (char) (CL_STR4 | dataLen);
    } else if (dataLenbits <= 8 * INT8_BYTES) {
        node->encoding = CL_STR8;
        node->sizeofLen = (uint8_t) int_setValue(node->len, (int64_t) dataLen);
    } else if (dataLenbits <= 8 * INT16_BYTES) {
        node->encoding = CL_STR16;
        node->sizeofLen = (uint8_t) int_setValue(node->len, (int64_t) dataLen);
    } else {
        node->encoding = CL_STR32;
        node->sizeofLen = (uint8_t) int_setValue(node->len, (int64_t) dataLen);
    }
}

static void cl_intNode_getEncoding(CompactListNode *node, int64_t val){
    assert(node->type == CL_TYPE_INT);

    uint8_t bytes = bytesForInt(val), bits = bitCount(val);
    if (bits <= 4) {
        node->encoding = (char) (CL_INT4 | val);
        node->sizeofData = 0;
    } else{
        char *intVal;
        if((intVal = malloc(bytes)) == NULL){
            panic("CompactList malloc for int val node failed\n");
        }
        int_setValue(intVal, val);
        node->data = intVal;
        node->sizeofData = bytes;
    }
}


static inline void cl_node_getTotal(CompactListNode *node) {
    uint32_t encDataSize = node->sizeofLen + node->sizeofData + 1;
    uint32_t total = bytesForUnsignedInt(encDataSize) + encDataSize;
    node->sizeofTotal = bytesForUnsignedInt(total);

    //split [total] and pack bits with mask
    for (int i = node->sizeofTotal - 1; i >= 0; i--) {
        //get 6 least significant bits
        char cell = (char) (CL_ENTRY_TOTAL | (total & 0x3F));
        node->total[i] = cell;
        total >>= 6;
    }
}

CompactListNode *cl_node_new(){
    CompactListNode *node;
    if((node = malloc(sizeof(*node))) == NULL){
        panic("CompactList malloc node failed\n");
    }
    node->data = NULL;
    node->sizeofData = 0;
    node->sizeofLen = 0;
    node->sizeofTotal = 0;
    return node;
}

void cl_node_free(CompactListNode *node){
    free(node->data);
    free(node);
}

//TODO add tail now
CompactList *CompactListAdd(CompactList *list, char *data, size_t dataLen) {
    int64_t val;
    char encoding[24];

    if (string2int(data, dataLen, &val) == 1) {

        //TODO realloc, shift right, memcpy
    } else {
        cl_setStringEncodingAndLen(dataLen, encoding);

    }


    list->size++;
    //TODO update list->bytes
    return list;
}
