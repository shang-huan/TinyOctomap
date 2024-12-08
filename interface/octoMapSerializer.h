#ifndef OCTOMAPSERIALIZER_H
#define OCTOMAPSERIALIZER_H
#include "stdlib.h"
#include "octoMap.h"
#include "octoNodeSet.h"
#include "octoNode.h"
#include "octoTree.h"
#include "compressBaseStruct.h"
#include "LZWCompress.h"
#include "huffmanTree.h"

#define MAX_SERIALIZER_SIZE 4*1024
#define MAX_DICT_SIZE 2*1024
#define MAX_DATA_SIZE MAX_SERIALIZER_SIZE+MAX_DICT_SIZE

typedef enum{
    FREE = 0,
    OCCUPIED = 1,
    UNKNOWN = 2,
    SPLIT = 3,
}nodeStatus_t;

typedef enum{
    ORIGIN = 0,
    LOSSY = 1,
    LZW = 2,
    HUFFMAN = 3,
    LZW_HUFFMAN = 4
}DictType;

typedef struct 
{
    coordinate_t center;     // the coordinate of the center --- (x,y,z): tuple
    uint8_t resolution;      // resolution of the tree
    uint8_t maxDepth;        // max depth of the tree
    uint16_t width;

    uint8_t checkCode;      // check code
    DictType dictType;       // 字典类型
    uint16_t dictLength;         // 字典长度
    uint16_t dataLength;     // data length
    uint8_t data[MAX_DATA_SIZE];
}octoMapSerializerResult_t;

void initOctoMapSerializerResult(octoMapSerializerResult_t *result);

// 八叉树有损序列化,每个节点仅占2bit位
bool serializeOctoMapLossy(octoMap_t *octoMap, octoMapSerializerResult_t *result);

// 八叉树反序列化-有损
bool deserializeOctoMapLossy(octoMap_t *octoMap, octoMapSerializerResult_t *data);

// 八叉树序列化 
bool serializeOctoMap(octoMap_t *octoMap, octoMapSerializerResult_t *result);

// 八叉树反序列化 
bool deserializeOctoMap(octoMap_t *octoMap, octoMapSerializerResult_t *data);

// 压缩数据
bool compressData(octoMapSerializerResult_t *data);

// 校验两颗树是否一致
bool checkOctoMapisConsist(octoMap_t *octoMap1, octoMap_t *octoMap2);

// 生成octoMapSerializerResult_t
bool generateOctoMapSerializerResult(octoMap_t *octoMap, octoMapSerializerResult_t *result, DictType dictType);

// 从octoMapSerializerResult_t生成octoMap
bool generateOctoMapFromSerializerResult(octoMap_t *octoMap, octoMapSerializerResult_t *result);

#endif