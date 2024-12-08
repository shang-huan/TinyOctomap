#include "octoMapSerializer.h"
#include "crossSystem_tool.h"
#include "string.h"
#define MAX_QUEUE_BUFFER_SIZE 2048

void initOctoMapSerializerResult(octoMapSerializerResult_t *result){
    result->center.x = 0;
    result->center.y = 0;
    result->center.z = 0;
    result->resolution = 0;
    result->maxDepth = 0;
    result->width = 0;

    result->checkCode = 0;

    result->dictType = ORIGIN;
    result->dictLength = 0;

    result->dataLength = 0;

    memset(result->data,0,MAX_DATA_SIZE);
}

// 八叉树有损序列化,每个节点仅占2bit位
bool serializeOctoMapLossy(octoMap_t *octoMap, octoMapSerializerResult_t *result){
    octoNode_t *root = octoMap->octoTree->root;
    result->center = octoMap->octoTree->center;
    result->resolution = octoMap->octoTree->resolution;
    result->maxDepth = octoMap->octoTree->maxDepth;
    result->width = octoMap->octoTree->width;

    uint8_t checkCode = 0xff;
    // 层次遍历
    octoNode_t* queue[MAX_QUEUE_BUFFER_SIZE];
    int front = 0, rear = 0;
    octoNode_t *p = root;
    queue[rear] = p;
    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
    int dataIndex = 0;
    uint16_t dataLength = 0;
    int i = 0;
    while (rear != front)
    {
        i++;
        p = queue[front];
        front = (front + 1) % MAX_QUEUE_BUFFER_SIZE;
        uint16_t index = p->children;
        octoNodeSetItem_t* brothers = &(octoMap->octoNodeSet->setData[index]);
        uint8_t item = 0;
        uint8_t curLength = 0;
        for (int i = 0; i < 8; i++)
        {
            octoNode_t *node = &(brothers->data[i]);
            if(node->isLeaf == 1){
                if(node->logOdds == LOG_ODDS_OCCUPIED){
                    item = (item << 2) + OCCUPIED;
                }
                else if(node->logOdds == LOG_ODDS_FREE){
                    item = (item << 2) + FREE;
                }
                else{
                    item = (item << 2) + UNKNOWN;
                }
            }else{
                item = (item << 2) + SPLIT;
                if( (rear+1)%MAX_QUEUE_BUFFER_SIZE != front ){
                    queue[rear] = node;
                    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
                }else{
                    printf("[serializeOctoMap]queue is full\n");
                    return false;
                }
            }
            curLength += 2;
            if(curLength >= 8){
                if(dataIndex+1 >= MAX_DATA_SIZE){
                    printf("[serializeOctoMap]data is full\n");
                    return false;
                }
                checkCode = checkCode ^ item;
                result->data[dataIndex++] = item;
                dataLength++;
                curLength = 0;
                item = 0;
            }
        }
    }
    result->checkCode = checkCode;
    result->dataLength = dataLength;

    return true;
}

// 八叉树有损反序列化 
bool deserializeOctoMapLossy(octoMap_t *octoMap, octoMapSerializerResult_t *data){
    octoMap->octoTree->center = data->center;
    octoMap->octoTree->origin.x = data->center.x - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.y = data->center.y - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.z = data->center.z - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->resolution = data->resolution;
    octoMap->octoTree->maxDepth = data->maxDepth;
    octoMap->octoTree->width = data->width;
    octoNodeSplit(octoMap->octoTree->root, octoMap);

    uint16_t checkCode = 1<<15;

    octoNode_t* queue[MAX_QUEUE_BUFFER_SIZE];
    int front = 0, rear = 0;
    octoNode_t *p = octoMap->octoTree->root;
    queue[rear] = p;
    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
    int dataIndex = 0;
    while (rear != front)
    {
        p = queue[front];
        front = (front + 1) % MAX_QUEUE_BUFFER_SIZE;
        uint16_t index = p->children;
        // 读取数据,拼成16位一起处理
        uint16_t val1 = data->data[dataIndex++];
        uint16_t val2 = data->data[dataIndex++];
        uint16_t val = (val1 << 8) + val2;

        checkCode = checkCode ^ val;
        octoNodeSetItem_t* brothers = &(octoMap->octoNodeSet->setData[index]);
        for (int i = 0; i < 8; i++)
        {
            // 获取前两位
            uint16_t nodeStatus = (val >> (14-2*i)) & 0x3;
            octoNode_t *node = &(brothers->data[i]);
            if(nodeStatus == OCCUPIED){
                node->logOdds = LOG_ODDS_OCCUPIED;
                node->isLeaf = TRUE;
                node->children = 0;
            }
            else if(nodeStatus == FREE){
                node->logOdds = LOG_ODDS_FREE;
                node->isLeaf = TRUE;
                node->children = 0;
            }
            else if(nodeStatus == UNKNOWN){
                node->logOdds = LOG_ODDS_UNKNOWN;
                node->isLeaf = TRUE;
                node->children = 0;
            }
            else{
                octoNodeSplit(node, octoMap);
                if( (rear+1)%MAX_QUEUE_BUFFER_SIZE != front ){
                    queue[rear] = node;
                    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
                }else{
                    printf("[deserializeOctoMap]queue is full\n");
                    return false;
                }
            }
        }
    }
    if(checkCode != data->checkCode){
        printf("[deserializeOctoMap]checkCode is wrong\n");
        return false;
    }
    return true;
}

// 八叉树无损序列化,保留完整logOdds信息
bool serializeOctoMap(octoMap_t *octoMap, octoMapSerializerResult_t *result){
    octoNode_t *root = octoMap->octoTree->root;
    result->center = octoMap->octoTree->center;
    result->resolution = octoMap->octoTree->resolution;
    result->maxDepth = octoMap->octoTree->maxDepth;
    result->width = octoMap->octoTree->width;

    uint8_t checkCode = 0xff;
    // 层次遍历
    octoNode_t* queue[MAX_QUEUE_BUFFER_SIZE];
    int front = 0, rear = 0;
    octoNode_t *p = root;
    queue[rear] = p;
    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
    int dataIndex = 0;
    uint16_t dataLength = 0;
    int i = 0;
    while (rear != front)
    {
        i++;
        p = queue[front];
        front = (front + 1) % MAX_QUEUE_BUFFER_SIZE;
        uint16_t index = p->children;
        octoNodeSetItem_t* brothers = &(octoMap->octoNodeSet->setData[index]);
        uint8_t item = 0;
        uint8_t curLength = 0;

        // 由于每个节点8个孩子，每个节点占4bit，所以每个节点刚好占4个字节
        for (int i = 0; i < 8; i++)
        {
            octoNode_t *node = &(brothers->data[i]);
            if(node->isLeaf == 1){
                item = item<<4 | node->logOdds;
            }else{
                item = (item << 4) | 0xf;
                if( (rear+1)%MAX_QUEUE_BUFFER_SIZE != front ){
                    queue[rear] = node;
                    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
                }else{
                    printf("[serializeOctoMap]queue is full\n");
                    return false;
                }
            }
            curLength += 4;
            if(curLength >= 8){
                if(dataIndex+1 >= MAX_DATA_SIZE){
                    printf("[serializeOctoMap]data is full\n");
                    return false;
                }
                checkCode = checkCode ^ item;
                result->data[dataIndex++] = item;
                dataLength++;
                curLength = 0;
                item = 0;
            }
        }
    }
    result->checkCode = checkCode;
    result->dataLength = dataLength;

    return true;
}

// 八叉树反序列化
bool deserializeOctoMap(octoMap_t *octoMap, octoMapSerializerResult_t *data){
    octoMap->octoTree->center = data->center;
    octoMap->octoTree->origin.x = data->center.x - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.y = data->center.y - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.z = data->center.z - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->resolution = data->resolution;
    octoMap->octoTree->maxDepth = data->maxDepth;
    octoMap->octoTree->width = data->width;
    octoNodeSplit(octoMap->octoTree->root, octoMap);

    uint8_t checkCode = 0xff;

    octoNode_t* queue[MAX_QUEUE_BUFFER_SIZE];
    int front = 0, rear = 0;
    octoNode_t *p = octoMap->octoTree->root;
    queue[rear] = p;
    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
    int dataIndex = 0;
    while (rear != front)
    {
        p = queue[front];
        front = (front + 1) % MAX_QUEUE_BUFFER_SIZE;
        uint16_t index = p->children;
        octoNodeSetItem_t* brothers = &(octoMap->octoNodeSet->setData[index]);
        for (int i = 0; i < 4; i++)
        {
            uint8_t val = data->data[dataIndex++];
            checkCode = checkCode ^ val;
            // 获取前四位
            uint8_t first = (val >> 4) & 0xf;
            // 获取后四位
            uint8_t second = val & 0xf;
            octoNode_t *node = &(brothers->data[i*2]);
            if(first == 0xf){
                octoNodeSplit(node, octoMap);
                if( (rear+1)%MAX_QUEUE_BUFFER_SIZE != front ){
                    queue[rear] = node;
                    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
                }else{
                    printf("[deserializeOctoMap]queue is full\n");
                    return false;
                }
            }else{
                node->isLeaf = TRUE;
                node->logOdds = first;
            }
            node = &(brothers->data[i*2+1]);
            if(second == 0xf){
                octoNodeSplit(node, octoMap);
                if( (rear+1)%MAX_QUEUE_BUFFER_SIZE != front ){
                    queue[rear] = node;
                    rear = (rear + 1) % MAX_QUEUE_BUFFER_SIZE;
                }else{
                    printf("[deserializeOctoMap]queue is full\n");
                    return false;
                }
            }else{
                node->isLeaf = TRUE;
                node->logOdds = second;
            }
        }
    }
    if(checkCode != data->checkCode){
        printf("[deserializeOctoMap]checkCode is wrong\n");
        return false;
    }
    return true;
}

bool checkOctoMapisConsist(octoMap_t *octoMap1, octoMap_t *octoMap2){
    // todo
    return true;
}

bool generateOctoMapSerializerResult(octoMap_t *octoMap, octoMapSerializerResult_t *result, DictType dictType){
    // 实验测试各种方法最终数据长度
    // Lossy
    serializeOctoMapLossy(octoMap, result);
    uint16_t totalDataLength_Lossy = result->dataLength;
    printF("Lossy data length:%d\n",totalDataLength_Lossy);
    // Lossy+LZW
    LZWDict lzwDict;
    initLZWDict(&lzwDict);
    uint8_t newData_Lossy_LZW[MAX_SERIALIZER_SIZE];
    uint16_t dataLength_Lossy_LZW = LZWCompressData(result->data, totalDataLength_Lossy, &lzwDict, newData_Lossy_LZW, MAX_SERIALIZER_SIZE);
    printF("Lossy+LZW data length:%d, dict Length: %d\n",dataLength_Lossy_LZW,lzwDict.size*sizeof(LZWDictNode));
    uint16_t totalDataLength_Lossy_LZW = lzwDict.size*sizeof(LZWDictNode) + dataLength_Lossy_LZW;
    // Lossy+Huffman
    uint8_t newData_Lossy_Huffman[MAX_SERIALIZER_SIZE];
    HuffmanTree huffmanTree;
    initHuffmanTree(&huffmanTree);
    uint16_t dataLength_Lossy_Huffman = huffmanCode(result->data, totalDataLength_Lossy, &huffmanTree, newData_Lossy_Huffman, MAX_SERIALIZER_SIZE);
    printF("Lossy+Huffman data length:%d, dict Length: %d\n",dataLength_Lossy_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode));
    uint16_t totalDataLength_Lossy_Huffman = dataLength_Lossy_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode);
    // Lossy+LZW+Huffman
    uint8_t newData_Lossy_LZW_Huffman[MAX_SERIALIZER_SIZE];
    initHuffmanTree(&huffmanTree);
    uint16_t dataLength_Lossy_LZW_Huffman = huffmanCode(newData_Lossy_LZW, dataLength_Lossy_LZW, &huffmanTree, newData_Lossy_LZW_Huffman, MAX_SERIALIZER_SIZE);
    printF("Lossy+LZW+Huffman data length:%d, dict Length: %d\n",dataLength_Lossy_LZW_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode));
    uint16_t totalDataLength_Lossy_LZW_Huffman = dataLength_Lossy_LZW_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode);

    // Origin
    serializeOctoMap(octoMap, result);
    uint16_t totalDataLength_Origin = result->dataLength;
    printF("Origin data length:%d\n",totalDataLength_Origin);
    // Origin+LZW
    initLZWDict(&lzwDict);
    uint8_t newData_Origin_LZW[MAX_SERIALIZER_SIZE];
    uint16_t dataLength_Origin_LZW = LZWCompressData(result->data, totalDataLength_Origin, &lzwDict, newData_Origin_LZW, MAX_SERIALIZER_SIZE);
    printF("Origin+LZW data length:%d, dict Length: %d\n",dataLength_Origin_LZW,lzwDict.size*sizeof(LZWDictNode));
    uint16_t totalDataLength_Origin_LZW = lzwDict.size*sizeof(LZWDictNode) + dataLength_Origin_LZW;
    // Origin+Huffman
    uint8_t newData_Origin_Huffman[MAX_SERIALIZER_SIZE];
    initHuffmanTree(&huffmanTree);
    uint16_t dataLength_Origin_Huffman = huffmanCode(result->data, totalDataLength_Origin, &huffmanTree, newData_Origin_Huffman, MAX_SERIALIZER_SIZE);
    printF("Origin+Huffman data length:%d, dict Length: %d\n",dataLength_Origin_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode));
    uint16_t totalDataLength_Origin_Huffman = dataLength_Origin_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode);
    // Origin+LZW+Huffman
    uint8_t newData_Origin_LZW_Huffman[MAX_SERIALIZER_SIZE];
    initHuffmanTree(&huffmanTree);
    uint16_t dataLength_Origin_LZW_Huffman = huffmanCode(newData_Origin_LZW, dataLength_Origin_LZW, &huffmanTree, newData_Origin_LZW_Huffman, MAX_SERIALIZER_SIZE);
    printF("Origin+LZW+Huffman data length:%d, dict Length: %d\n",dataLength_Origin_LZW_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode));
    uint16_t totalDataLength_Origin_LZW_Huffman = dataLength_Origin_LZW_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode);

    printF("Lossy:%d,Lossy+LZW:%d,Lossy+Huffman:%d,Lossy+LZW+Huffman:%d\n",totalDataLength_Lossy,totalDataLength_Lossy_LZW,totalDataLength_Lossy_Huffman,totalDataLength_Lossy_LZW_Huffman);
    printF("Origin:%d,Origin+LZW:%d,Origin+Huffman:%d,Origin+LZW+Huffman:%d\n",totalDataLength_Origin,totalDataLength_Origin_LZW,totalDataLength_Origin_Huffman,totalDataLength_Origin_LZW_Huffman);

}