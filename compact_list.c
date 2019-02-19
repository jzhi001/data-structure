
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "integer.h"
#include "compact_list.h"
#include "panic.h"

#define COMPACT_LIST_DEBUG
#ifdef COMPACT_LIST_DEBUG

static void show_bytes(CompactList *list) {
    unsigned char *pt = (unsigned char *) list;
    printf("bytes: ");
    for (int i = 0; i < 8; i++) {
        printf("%2X ", pt[i]);
    }
    printf("\nsize: ");

    pt += 8;
    for (int i = 0; i < 4; i++) {
        printf("%2X ", pt[i]);
    }
    printf("\nelements: ");
    pt += 4;

    for (int i = 12; i < list->bytes - 1; i++) {
        printf("%2X ", pt[i - 12]);
    }

    printf("\nend: %02X\n", ((char *) list)[list->bytes - 1]);
}

#endif

/**
 * Used for constructing an entry.
 */
typedef struct {
    char type;
    unsigned char encoding;

    char len[8];
    uint8_t sizeofLen;

    char *data;
    uint32_t sizeofData;

    char total[10];
    int sizeofTotal;
} CompactListNode;

inline int64_t CompactListSize(CompactList *list) {
    return list->size;
}

static inline char *cl_getEndOfList(CompactList *list) {
    return ((char *) list) + list->bytes - 1;
}

static inline size_t cl_headerBytes() {
    return sizeof(CompactList);
}

static inline size_t cl_sizeofEmptyList() {
    return cl_headerBytes() + CL_END_BYTES;
}


CompactList *CompactListNew() {
    CompactList *list;
    if ((list = malloc(cl_sizeofEmptyList())) == NULL) {
        panic("CompactList malloc failed\n");
    }
    list->size = 0;
    list->bytes = cl_sizeofEmptyList();
    ((char *) list)[cl_headerBytes()] = (char) CL_END;
    return list;
}

inline void CompactListFree(CompactList *list) {
    free(list);
}

static int cl_getEntryType(char encoding) {
    return (unsigned char) encoding >> 6;
}

static inline int cl_isIntEntry(const char *ele) {
    return cl_getEntryType(ele[0]) == CL_TYPE_INT;
}

static inline int cl_isStrEntry(const char *ele) {
    return cl_getEntryType(ele[0]) == CL_TYPE_STR;
}

static inline int cl_isEndOfList(const char *ele) {
    return ele[0] == (char) CL_END;
}

static void cl_strNode_setEncoding(CompactListNode *node) {
    assert(node->type == CL_TYPE_STR);
    int64_t dataLen = node->sizeofData;
    uint8_t dataLenbits = bitCount(dataLen);

    if (dataLenbits <= 4) {
        node->encoding = (unsigned char) (CL_STR4 | dataLen);
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


static void cl_intNode_setEncodingAndData(CompactListNode *node, int64_t val) {
    assert(node->type == CL_TYPE_INT);

    uint8_t bytes = bytesForInt(val), bits = bitCount(val);
    if (bits <= 4) {
        node->encoding = (unsigned char) (CL_INT4 | val);
        node->sizeofData = 0;
    } else {
        char *intVal;
        if ((intVal = malloc(bytes)) == NULL) {
            panic("CompactList malloc for int val node failed\n");
        }
        int_setValue(intVal, val);
        node->data = intVal;
        node->sizeofData = bytes;
        switch (bytes) {
            case INT8_BYTES:
                node->encoding = CL_INT8;
                break;
            case INT16_BYTES:
                node->encoding = CL_INT16;
                break;
            case INT32_BYTES:
                node->encoding = CL_INT32;
                break;
            case INT64_BYTES:
                node->encoding = CL_INT64;
                break;
            default:
                panic("CompactList unknown int type %d\n", bytes);
        }
    }
}


//size used to store data length
static uint32_t cl_getDataLenSize(const char *ele) {
    unsigned char enc = (unsigned char) ele[0];
    switch (enc & 0xF0) {
        case CL_STR4:
        case CL_INT4:
        case CL_INT8:
        case CL_INT16:
        case CL_INT32:
        case CL_INT64:
            return 0;
        case CL_STR8:
            return 1;
        case CL_STR16:
            return 2;
        case CL_STR32:
            return 4;
        default:
            panic("CompactList getDataLenSize: unknown entry encoding %d\n", enc & 0xF0);
    }
}

//get data size
static inline uint32_t cl_getDataSize(char *ele) {
    unsigned char enc = (unsigned char) ele[0];
    unsigned char type = (unsigned char) (enc & 0xF0);

    if (type == CL_INT4) {
        return 0;
    } else if (type == CL_STR4) {
        return (uint32_t) (enc & 0x0F);
    } else if (cl_isIntEntry(ele)) {
        //TODO refactor
        if (type == CL_INT8) {
            return 1;
        } else if (type == CL_INT16) {
            return 2;
        } else if (enc == CL_INT32) {
            return 4;
        } else if (enc == CL_INT64) {
            return 8;
        } else {
            goto err;
        }
    } else if (cl_isStrEntry(ele)) {
        uint32_t dataLenSize = cl_getDataLenSize(ele);
        return (uint32_t) int_getValue(ele + 1, dataLenSize);
    } else {
        goto err;
    }

    err:
    panic("CompactList getDataSize: unknown string entry encoding 0x%x\n", enc & 0xF0);
}

//get total size
static inline uint32_t cl_getTotalSize(char *ele) {
    uint32_t tot = cl_getDataLenSize(ele) + CL_ENC_BYTES + cl_getDataSize(ele);
    uint32_t totBits = bitCount(tot);
    return totBits / 7 + (totBits % 7 > 0 ? 1 : 0);
}


static inline void cl_node_setTotal(CompactListNode *node) {
    uint32_t encDataSize = node->sizeofLen + node->sizeofData + CL_ENC_BYTES;
    uint32_t encDataBits = bitCount(encDataSize);

    uint32_t sizeofTotal = encDataBits / 7 + (encDataBits % 7 > 0 ? 1 : 0);
    node->sizeofTotal = sizeofTotal;

    for (int i = node->sizeofTotal - 1; i >= 0; i--) {
        char part = (char) (encDataSize & 0x7F);
        if (i == 0) {
            part |= 0x80;
        }
        node->total[i] = part;
        encDataSize >>= 7;
    }
}

static inline void cl_ensureNode(const char *ele) {
    char enc = ele[0];
    if (enc != (char) CL_END) {
        char type = (char) cl_getEntryType(enc);
        assert(type == CL_TYPE_STR || type == CL_TYPE_INT);
    }
}

static inline uint32_t cl_getEntrySize(char *ele) {
    return CL_ENC_BYTES + cl_getDataLenSize(ele) + cl_getDataSize(ele) + cl_getTotalSize(ele);
}

static char *cl_nextElement(char *ele) {
    cl_ensureNode(ele);
    if (ele[0] == (char) CL_END) {
        return NULL;
    } else {
        return ele + cl_getEntrySize(ele);
    }
}

//idx is index of ele
static char *cl_prevElement(char *ele, int64_t idx) {
    if (idx - 1 < 0) return NULL;
    cl_ensureNode(ele);

    char *pt = ele - 1;
    unsigned char totPart, weight = 0;
    uint64_t entrySize = 0;
    int stop = 0;
    do {
        totPart = (unsigned char) (pt[0]);
        entrySize |= (totPart & 0x7F) << (7 * weight);
        weight++;
        pt--;
        stop = totPart >> 7;
    } while (!stop);
    return pt - entrySize + 1;
}

static char *cl_firstElement(CompactList *list) {
    return list->size > 0 ? (char *) list + cl_headerBytes() : NULL;
}

static char *cl_lastElement(CompactList *list) {
    if (list->size == 0) {
        return NULL;
    } else {
        char *end = (char *) list + list->bytes - 1;
        return cl_prevElement(end, list->size);
    }
}

static char *cl_elementAt(CompactList *list, int64_t idx) {
    assert(list->size > 0 && idx >= 0 && idx < list->size);
    char *node;

    if (idx <= list->size / 2) {
        //iter from head
        node = cl_firstElement(list);
        while (idx > 0) {
            node = cl_nextElement(node);
            idx--;
        }
    } else {
        //iter from tail
        node = cl_lastElement(list);
        int64_t curIdx = list->size - 1;
        while (curIdx > idx) {
            node = cl_prevElement(node, curIdx);
            curIdx--;
        }
    }

    return node;
}

//return -1 if int val, 0 or positive number if string val
static int64_t cl_valueAt(CompactList *list, int64_t idx, int64_t *intVal, char **strVal) {
    char *ele = cl_elementAt(list, idx);
    unsigned char enc = (unsigned char) ele[0];
    unsigned char type = (unsigned char) cl_getEntryType(enc);
    uint32_t dataLenSize = cl_getDataLenSize(ele);

    if (type == CL_TYPE_STR) {
        if ((enc & 0xF0) == CL_STR4) {
            if (strVal) *strVal = ele + 1;
            return ele[0] & 0x0F;
        } else {
            if (strVal) *strVal = ele + CL_ENC_BYTES + dataLenSize;
            return int_getValue(ele + 1, dataLenSize);
        }
    } else if (type == CL_TYPE_INT) {
        //TODO refactor encoding
        if (enc & 0xF0 == CL_INT4) {
            if (intVal) *intVal = ele[0] & 0x0F;
        } else if (enc == CL_INT64) {
            if (intVal) *intVal = int_getValue(ele + 1, INT64_BYTES);
        } else if ((enc & 0xF0) == CL_INT16) {
            if (intVal) *intVal = int_getValue(ele + 1, INT16_BYTES);
        } else if ((enc & 0xF0) == CL_INT32) {
            if (intVal) *intVal = int_getValue(ele + 1, INT32_BYTES);
        } else {
            panic("CompactList valueAt: unknown encoding 0x%x\n", enc);
        }
        return -1;
    } else {
        panic("CompactList valueAt: unknown entry type 0x%x\n", type);
    }
}

static inline CompactListNode *cl_node_init(CompactListNode *node) {
    node->data = NULL;
    node->sizeofData = 0;
    node->sizeofLen = 0;
    node->sizeofTotal = 0;
    return node;
}

static inline uint32_t cl_node_size(CompactListNode *node) {
    return CL_ENC_BYTES + node->sizeofLen + node->sizeofData + node->sizeofTotal;
}

int64_t CompactListIndexOf(CompactList *list, char *data, size_t len){
    if(list->size == 0){
        return -1;
    }

    uint32_t idx = 0;
    char *strVal;
    int64_t intVal;
    while (idx < list->size ) {
        int64_t entryDataLen = cl_valueAt(list, idx, &intVal, &strVal);
        if (entryDataLen == -1) {
            int64_t idata;
            int succ = string2int(data, len, &idata);
            if (succ && (idata == intVal)) {
                return idx;
            }
        } else {
            if ((entryDataLen == len) && (strncmp(strVal, data, len) == 0)) {
                return idx;
            }
        }
        idx++;
    }
    return -1;
}

//TODO getValue(entry)
CompactList *CompactListRemove(CompactList *list, char *data, size_t len, int *ret) {
    int64_t idx = CompactListIndexOf(list, data, len);
    if(idx == -1){
        if(ret) *ret = 0;
        return list;
    }

    if(ret) *ret = 1;
    char *tar = cl_elementAt(list, idx);
    //shift left
    uint32_t entrySize = cl_getEntrySize(tar);
    int64_t cpyLen = cl_getEndOfList(list) - (tar + entrySize) + 1;
    memcpy(tar, tar + entrySize, (size_t)cpyLen);

    list->bytes -= entrySize;
    list->size--;
    if((list = realloc(list, list->bytes)) == NULL){
        panic("CompactList remove: realloc failed\n");
    }

    return list;

}

CompactList *CompactListInsert(CompactList *list, char *data, size_t dataLen, int64_t idx) {
    if (list->size == UINT32_MAX) {
        panic("CompactList list is full\n");
    } else if (idx > list->size) {
        panic("index too big, max index is list size\n");
    }
    //create node
    int64_t val;
    CompactListNode node;
    cl_node_init(&node);
    node.sizeofData = (uint32_t) dataLen;
    node.data = data;

    if (string2int(data, dataLen, &val) == 1) {
        node.type = CL_TYPE_INT;
        cl_intNode_setEncodingAndData(&node, val);
    } else {
        node.type = CL_TYPE_STR;
        cl_strNode_setEncoding(&node);
    }
    cl_node_setTotal(&node);

    //resize
    if ((list = realloc(list, list->bytes + cl_node_size(&node))) == NULL) {
        panic("CompactList realloc failed\n");
    }

    //shift right
    char *ele;
    if (idx == list->size) {
        //add at tail
        ele = cl_getEndOfList(list);
        list->bytes += cl_node_size(&node);
        char *end = cl_getEndOfList(list);
        end[0] = (char) CL_END;
    } else {
        char *start = cl_elementAt(list, idx);
        char *end = cl_getEndOfList(list);
        ele = start;
        memcpy(start + cl_node_size(&node), start, end - start + 1);
        list->bytes += cl_node_size(&node);
    }
    list->size++;

    //insert node
    ele[0] = node.encoding;
    ele++;
    if (node.sizeofLen > 0) {
        memcpy(ele, node.len, node.sizeofLen);
        ele += node.sizeofLen;
    }

    if (node.sizeofData > 0) {
        memcpy(ele, node.data, node.sizeofData);
    }
    ele += node.sizeofData;

    memcpy(ele, node.total, (size_t) node.sizeofTotal);
    if (node.type == CL_TYPE_INT) free(node.data);
    return list;
}


#define COMPACT_LIST_TEST
#ifdef COMPACT_LIST_TEST

int main() {
    CompactList *list = CompactListNew();

    char *strVal;
    int64_t intVal, ret;
    int rmRet;

    list = CompactListInsert(list, "hello", 5, 0);
    ret = cl_valueAt(list, 0, &intVal, &strVal);
    assert(ret == 5);
    assert(strncmp(strVal, "hello", (size_t) ret) == 0);


    list = CompactListInsert(list, "world", 5, 0);
    ret = cl_valueAt(list, 0, &intVal, &strVal);
    assert(ret >= 0);
    assert(strncmp(strVal, "world", (size_t) ret) == 0);

    list = CompactListInsert(list, "12345", 5, 1);
    ret = cl_valueAt(list, 1, &intVal, &strVal);
    assert(ret == -1);
    assert(intVal == 12345);

    list = CompactListInsert(list, "9223372036854775807", 19, 2);
    ret = cl_valueAt(list, 2, &intVal, &strVal);
    assert(ret == -1);
    assert(intVal == INT64_MAX);

    char *str = "AAAAAAAAAAAAAAAAAAAAAAAA";
    list = CompactListInsert(list, str, strlen(str), 0);
    ret = cl_valueAt(list, 0, &intVal, &strVal);
    assert(ret == 24);
    assert(strncmp(strVal, str, strlen(str)) == 0);


    show_bytes(list);
    list = CompactListRemove(list, "12345", 5, &rmRet);
    assert(rmRet == 1);
    show_bytes(list);

    CompactListFree(list);
    return 0;
}

#endif
