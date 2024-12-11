#include "octoMapSerializer.h"
#include "crossSystem_tool.h"
#include "string.h"
#include "time.h"
#include "octoMap.h"
#define MAX_QUEUE_BUFFER_SIZE 2048

#define COUNT_TIME_NUMS 10000

bool checkData(uint8_t* data1, uint8_t* data2, uint16_t dataLength1, uint16_t dataLength2);

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

    time_t startTime = time(NULL);

    octoNode_t *root = octoMap->octoTree->root;
    result->center = octoMap->octoTree->center;
    result->resolution = octoMap->octoTree->resolution;
    result->maxDepth = octoMap->octoTree->maxDepth;
    result->width = octoMap->octoTree->width;

    uint8_t checkCode = CHECK_CODE_INIT_VALUE;
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

    // printF("serializeOctoMapLossy time:%lds\n",time(NULL)-startTime);

    return true;
}

// 八叉树有损反序列化 
bool deserializeOctoMapLossy(octoMap_t *octoMap, octoMapSerializerResult_t *data){

    time_t startTime = time(NULL);

    octoMap->octoTree->center = data->center;
    octoMap->octoTree->origin.x = data->center.x - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.y = data->center.y - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.z = data->center.z - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->resolution = data->resolution;
    octoMap->octoTree->maxDepth = data->maxDepth;
    octoMap->octoTree->width = data->width;
    octoNodeSplit(octoMap->octoTree->root, octoMap);

    uint8_t checkCode = CHECK_CODE_INIT_VALUE;

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
        // 读取数据,拼成16位一起处理
        for(int i = 0;i<2;++i){
            if(dataIndex >= data->dataLength){
                printf("[deserializeOctoMap]data is not enough\n");
                return false;
            }
            uint8_t val = data->data[dataIndex++];
            checkCode = checkCode ^ val;
            for(int j = 0;j<4;++j){
                uint8_t nodeStatus = (val >> (6-2*j)) & 0x3;
                octoNode_t *node = &(brothers->data[i*4+j]);
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
    }
    if(checkCode != data->checkCode){
        printf("[deserializeOctoMap]checkCode is wrong\n");
        return false;
    }

    // printF("deserializeOctoMapLossy time:%lds\n",time(NULL)-startTime);

    return true;
}

// 八叉树无损序列化,保留完整logOdds信息
bool serializeOctoMap(octoMap_t *octoMap, octoMapSerializerResult_t *result){

    time_t startTime = time(NULL);

    octoNode_t *root = octoMap->octoTree->root;
    result->center = octoMap->octoTree->center;
    result->resolution = octoMap->octoTree->resolution;
    result->maxDepth = octoMap->octoTree->maxDepth;
    result->width = octoMap->octoTree->width;

    uint8_t checkCode = CHECK_CODE_INIT_VALUE;
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

    // printF("serializeOctoMap time:%lds\n",time(NULL)-startTime);

    return true;
}

// 八叉树反序列化
bool deserializeOctoMap(octoMap_t *octoMap, octoMapSerializerResult_t *data){

    time_t startTime = time(NULL);

    octoMap->octoTree->center = data->center;
    octoMap->octoTree->origin.x = data->center.x - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.y = data->center.y - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->origin.z = data->center.z - data->resolution * (1 << data->maxDepth) / 2;
    octoMap->octoTree->resolution = data->resolution;
    octoMap->octoTree->maxDepth = data->maxDepth;
    octoMap->octoTree->width = data->width;
    octoNodeSplit(octoMap->octoTree->root, octoMap);

    uint8_t checkCode = CHECK_CODE_INIT_VALUE;

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

    // printF("deserializeOctoMap time:%lds\n",time(NULL)-startTime);

    return true;
}

bool checkOctoMapisConsist(octoMap_t *octoMap1, octoMap_t *octoMap2){
    // todo
    return true;
}

bool checkData(uint8_t* data1, uint8_t* data2, uint16_t dataLength1, uint16_t dataLength2){
    if(dataLength1 != dataLength2){
        printF("[checkData]dataLength1 = %d, dataLength2 = %d\n", dataLength1, dataLength2);
        // return false;
    }
    for (int i = 0; i < dataLength1; i++)
    {
        if(data1[i] != data2[i]){
            printF("[checkData]data1[%d] = %d, data2[%d] = %d\n", i, data1[i], i, data2[i]);
            return false;
        }
    }
    return true;
}

bool generateOctoMapSerializerResult(octoMap_t *octoMap, octoMapSerializerResult_t *result, DictType dictType){
    sleep_ms(1000);
    // 实验测试各种方法最终数据长度
    // Lossy
    struct timespec startTime_Lossy, endTime_Lossy;
    clock_gettime(CLOCK_REALTIME, &startTime_Lossy);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initOctoMapSerializerResult(result);
        serializeOctoMapLossy(octoMap, result);
    }
    // 获取结束时间
    clock_gettime(CLOCK_REALTIME, &endTime_Lossy);
    double elapsedTime = (endTime_Lossy.tv_sec - startTime_Lossy.tv_sec) + 
                        (endTime_Lossy.tv_nsec - startTime_Lossy.tv_nsec) / 1e9;
    uint16_t totalDataLength_Lossy = result->dataLength;
    printf("Lossy data length: %d, elapsed time: %f seconds\n", totalDataLength_Lossy, elapsedTime);

    // Lossy+LZW
    LZWDict lzwDict;
    
    uint8_t newData_Lossy_LZW[MAX_DATA_SIZE];
    uint16_t dataLength_Lossy_LZW = 0;

    struct timespec startTime_Lossy_LZW, endTime_Lossy_LZW;
    clock_gettime(CLOCK_REALTIME, &startTime_Lossy_LZW);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initLZWDict(&lzwDict);
        dataLength_Lossy_LZW = LZWEncode(result->data, totalDataLength_Lossy, &lzwDict, newData_Lossy_LZW, MAX_SERIALIZER_SIZE);
        memcpy(&(newData_Lossy_LZW[dataLength_Lossy_LZW]),lzwDict.nodes,sizeof(LZWDictNode)*lzwDict.size);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Lossy_LZW);
    double elapsedTime_Lossy_LZW = (endTime_Lossy_LZW.tv_sec - startTime_Lossy_LZW.tv_sec) + 
                        (endTime_Lossy_LZW.tv_nsec - startTime_Lossy_LZW.tv_nsec) / 1e9;
    uint16_t totalDataLength_Lossy_LZW = lzwDict.size*sizeof(LZWDictNode) + dataLength_Lossy_LZW;
    printF("Lossy+LZW data length:%d, dict Length: %d, finish Times:%f\n",dataLength_Lossy_LZW,lzwDict.size*sizeof(LZWDictNode),elapsedTime_Lossy_LZW);

    // Lossy+Huffman
    uint8_t newData_Lossy_Huffman[MAX_DATA_SIZE];
    HuffmanTree huffmanTree;
    uint16_t dataLength_Lossy_Huffman = 0;
    
    struct timespec startTime_Lossy_Huffman, endTime_Lossy_Huffman;
    clock_gettime(CLOCK_REALTIME, &startTime_Lossy_Huffman);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initHuffmanTree(&huffmanTree);
        dataLength_Lossy_Huffman = huffmanEnCode(result->data, totalDataLength_Lossy, &huffmanTree, newData_Lossy_Huffman, MAX_SERIALIZER_SIZE);
        memcpy(&(newData_Lossy_Huffman[dataLength_Lossy_Huffman]),huffmanTree.nodes,sizeof(HuffmanTreeNode)*huffmanTree.size);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Lossy_Huffman);
    double elapsedTime_Lossy_Huffman = (endTime_Lossy_Huffman.tv_sec - startTime_Lossy_Huffman.tv_sec) + 
                        (endTime_Lossy_Huffman.tv_nsec - startTime_Lossy_Huffman.tv_nsec) / 1e9;
    printF("Lossy+Huffman data length:%d, dict Length: %d, finish Times:%f\n",dataLength_Lossy_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode),elapsedTime_Lossy_Huffman);
    uint16_t totalDataLength_Lossy_Huffman = dataLength_Lossy_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode);
    // Lossy+LZW+Huffman
    uint16_t dataLength_Lossy_LZW_Huffman = 0;
    uint16_t totalDataLength_Lossy_LZW_Huffman = 0;
    uint8_t newData_Lossy_LZW_Huffman[MAX_DATA_SIZE];
    initHuffmanTree(&huffmanTree);
    dataLength_Lossy_LZW_Huffman = huffmanEnCode(newData_Lossy_LZW, dataLength_Lossy_LZW, &huffmanTree, newData_Lossy_LZW_Huffman, MAX_SERIALIZER_SIZE);
    printF("Lossy+LZW+Huffman data length:%d, dict Length: %d\n",dataLength_Lossy_LZW_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode));
    totalDataLength_Lossy_LZW_Huffman = dataLength_Lossy_LZW_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode);

    // check
    // Lossy+LZW -> Lossy
    uint16_t dictSize_Lossy_LZW = totalDataLength_Lossy_LZW-dataLength_Lossy_LZW;
    uint8_t newData_Lossy_LZW_Check[MAX_DATA_SIZE];
    LZWDict lzwDict_Lossy_LZW;
    uint16_t dataLength_Lossy_LZW_Check = 0;
    
    struct timespec startTime_Lossy_LZW_Check, endTime_Lossy_LZW_Check;
    clock_gettime(CLOCK_REALTIME, &startTime_Lossy_LZW_Check);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initLZWDict(&lzwDict_Lossy_LZW);
        memcpy(lzwDict_Lossy_LZW.nodes,&(newData_Lossy_LZW[dataLength_Lossy_LZW]),dictSize_Lossy_LZW);
        lzwDict_Lossy_LZW.size = dictSize_Lossy_LZW/sizeof(LZWDictNode);
        dataLength_Lossy_LZW_Check = LZWDecode(newData_Lossy_LZW, dataLength_Lossy_LZW, &lzwDict_Lossy_LZW, newData_Lossy_LZW_Check, MAX_SERIALIZER_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Lossy_LZW_Check);
    double elapsedTime_Lossy_LZW_Check = (endTime_Lossy_LZW_Check.tv_sec - startTime_Lossy_LZW_Check.tv_sec) + 
                        (endTime_Lossy_LZW_Check.tv_nsec - startTime_Lossy_LZW_Check.tv_nsec) / 1e9;
    printF("Lossy+LZW -> Lossy decode time:%f\n",elapsedTime_Lossy_LZW_Check);
    
    // printLZWDict(&lzwDict_Lossy_LZW,lzwDict_Lossy_LZW.size);
    // printLZWDict(&lzwDict,10);
    
    if(!checkData(result->data,newData_Lossy_LZW_Check,totalDataLength_Lossy,dataLength_Lossy_LZW_Check) != 0){
        printF("Lossy+LZW -> Lossy check failed\n");
        // return false;
    }else{
        printF("Lossy+LZW -> Lossy check success\n");
    }
    // Lossy+Huffman -> Lossy
    uint16_t dictSize_Lossy_Huffman = totalDataLength_Lossy_Huffman-dataLength_Lossy_Huffman;
    uint8_t newData_Lossy_Huffman_Check[MAX_DATA_SIZE];
    HuffmanTree huffmanTree_Lossy_Huffman;
    uint16_t dataLength_Lossy_Huffman_Check = 0;

    struct timespec startTime_Lossy_Huffman_Check, endTime_Lossy_Huffman_Check;
    clock_gettime(CLOCK_REALTIME, &startTime_Lossy_Huffman_Check);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initHuffmanTree(&huffmanTree_Lossy_Huffman);
        memcpy(huffmanTree_Lossy_Huffman.nodes,&(newData_Lossy_Huffman[dataLength_Lossy_Huffman]),dictSize_Lossy_Huffman);
        huffmanTree_Lossy_Huffman.size = dictSize_Lossy_Huffman/sizeof(HuffmanTreeNode);
        huffmanTree_Lossy_Huffman.root = huffmanTree_Lossy_Huffman.size-1;
        dataLength_Lossy_Huffman_Check = huffmanDecode(newData_Lossy_Huffman, dataLength_Lossy_Huffman, &huffmanTree_Lossy_Huffman, newData_Lossy_Huffman_Check, MAX_SERIALIZER_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Lossy_Huffman_Check);
    double elapsedTime_Lossy_Huffman_Check = (endTime_Lossy_Huffman_Check.tv_sec - startTime_Lossy_Huffman_Check.tv_sec) + 
                        (endTime_Lossy_Huffman_Check.tv_nsec - startTime_Lossy_Huffman_Check.tv_nsec) / 1e9;
    printF("Lossy+Huffman -> Lossy decode time:%f\n",elapsedTime_Lossy_Huffman_Check);

    
    // printHuffmanTree(&huffmanTree,10);
    // printHuffmanTree(&huffmanTree_Lossy_Huffman,10);
    if(!checkData(result->data,newData_Lossy_Huffman_Check,totalDataLength_Lossy,dataLength_Lossy_Huffman_Check) != 0){
        printF("Lossy+Huffman -> Lossy check failed\n");
        // return false;
    }else{
        printF("Lossy+Huffman -> Lossy check success\n");
    }

    // deSerialize
    octoMap_t octoMap_Lossy;
    
    struct timespec startTime_Lossy_Deserialize, endTime_Lossy_Deserialize;
    clock_gettime(CLOCK_REALTIME, &startTime_Lossy_Deserialize);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        octoMapInit(&octoMap_Lossy);
        deserializeOctoMapLossy(&octoMap_Lossy,result);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Lossy_Deserialize);
    double elapsedTime_Lossy_Deserialize = (endTime_Lossy_Deserialize.tv_sec - startTime_Lossy_Deserialize.tv_sec) + 
                        (endTime_Lossy_Deserialize.tv_nsec - startTime_Lossy_Deserialize.tv_nsec) / 1e9;
    printF("Lossy deserialize time:%f\n",elapsedTime_Lossy_Deserialize);

    // Origin
    struct timespec startTime_Origin, endTime_Origin;
    clock_gettime(CLOCK_REALTIME, &startTime_Origin);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initOctoMapSerializerResult(result);
        serializeOctoMap(octoMap, result);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Origin);
    double elapsedTime_Origin = (endTime_Origin.tv_sec - startTime_Origin.tv_sec) + 
                        (endTime_Origin.tv_nsec - startTime_Origin.tv_nsec) / 1e9;
    uint16_t totalDataLength_Origin = result->dataLength;
    printF("Origin data length: %d, elapsed time: %f seconds\n", totalDataLength_Origin, elapsedTime_Origin);
    // Origin+LZW
    
    uint8_t newData_Origin_LZW[MAX_DATA_SIZE];
    uint16_t dataLength_Origin_LZW = 0;

    struct timespec startTime_Origin_LZW, endTime_Origin_LZW;
    clock_gettime(CLOCK_REALTIME, &startTime_Origin_LZW);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {   
        initLZWDict(&lzwDict);
        dataLength_Origin_LZW = LZWEncode(result->data, totalDataLength_Origin, &lzwDict, newData_Origin_LZW, MAX_SERIALIZER_SIZE);
        memcpy(&(newData_Origin_LZW[dataLength_Origin_LZW]),lzwDict.nodes,sizeof(LZWDictNode)*lzwDict.size);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Origin_LZW);
    double elapsedTime_Origin_LZW = (endTime_Origin_LZW.tv_sec - startTime_Origin_LZW.tv_sec) + 
                        (endTime_Origin_LZW.tv_nsec - startTime_Origin_LZW.tv_nsec) / 1e9;
    printF("Origin+LZW data length:%d, dict Length: %d, finish Times:%f\n",dataLength_Origin_LZW,lzwDict.size*sizeof(LZWDictNode),elapsedTime_Origin_LZW);
    uint16_t totalDataLength_Origin_LZW = lzwDict.size*sizeof(LZWDictNode) + dataLength_Origin_LZW;
    // Origin+Huffman
    uint8_t newData_Origin_Huffman[MAX_DATA_SIZE];
    
    uint16_t dataLength_Origin_Huffman = 0;
    struct timespec startTime_Origin_Huffman, endTime_Origin_Huffman;
    clock_gettime(CLOCK_REALTIME, &startTime_Origin_Huffman);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {   
        initHuffmanTree(&huffmanTree);
        dataLength_Origin_Huffman = huffmanEnCode(result->data, totalDataLength_Origin, &huffmanTree, newData_Origin_Huffman, MAX_SERIALIZER_SIZE);
        memcpy(&(newData_Origin_Huffman[dataLength_Origin_Huffman]),huffmanTree.nodes,sizeof(HuffmanTreeNode)*huffmanTree.size);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Origin_Huffman);
    double elapsedTime_Origin_Huffman = (endTime_Origin_Huffman.tv_sec - startTime_Origin_Huffman.tv_sec) + 
                        (endTime_Origin_Huffman.tv_nsec - startTime_Origin_Huffman.tv_nsec) / 1e9;
    printF("Origin+Huffman data length:%d, dict Length: %d, finish Times:%f\n",dataLength_Origin_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode),elapsedTime_Origin_Huffman);
    uint16_t totalDataLength_Origin_Huffman = dataLength_Origin_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode);
    // Origin+LZW+Huffman
    uint16_t dataLength_Origin_LZW_Huffman = 0;
    uint16_t totalDataLength_Origin_LZW_Huffman = 0;
    uint8_t newData_Origin_LZW_Huffman[MAX_DATA_SIZE];
    initHuffmanTree(&huffmanTree);
    dataLength_Origin_LZW_Huffman = huffmanEnCode(newData_Origin_LZW, dataLength_Origin_LZW, &huffmanTree, newData_Origin_LZW_Huffman, MAX_SERIALIZER_SIZE);
    printF("Origin+LZW+Huffman data length:%d, dict Length: %d\n",dataLength_Origin_LZW_Huffman,huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode));
    totalDataLength_Origin_LZW_Huffman = dataLength_Origin_LZW_Huffman + huffmanTree.size*sizeof(HuffmanTreeNode) + lzwDict.size*sizeof(LZWDictNode);

    // check
    // Origin+LZW -> Origin
    uint16_t dictSize_Origin_LZW = totalDataLength_Origin_LZW-dataLength_Origin_LZW;
    uint8_t newData_Origin_LZW_Check[MAX_DATA_SIZE];
    LZWDict lzwDict_Origin_LZW;
    uint16_t dataLength_Origin_LZW_Chekc = 0;
    
    struct timespec startTime_Origin_LZW_Check, endTime_Origin_LZW_Check;
    clock_gettime(CLOCK_REALTIME, &startTime_Origin_LZW_Check);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initLZWDict(&lzwDict_Origin_LZW);
        memcpy(lzwDict_Origin_LZW.nodes,&(newData_Origin_LZW[dataLength_Origin_LZW]),dictSize_Origin_LZW);
        lzwDict_Origin_LZW.size = dictSize_Origin_LZW/sizeof(LZWDictNode);
        dataLength_Origin_LZW_Chekc = LZWDecode(newData_Origin_LZW, dataLength_Origin_LZW, &lzwDict_Origin_LZW, newData_Origin_LZW_Check, MAX_SERIALIZER_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Origin_LZW_Check);
    double elapsedTime_Origin_LZW_Check = (endTime_Origin_LZW_Check.tv_sec - startTime_Origin_LZW_Check.tv_sec) + 
                        (endTime_Origin_LZW_Check.tv_nsec - startTime_Origin_LZW_Check.tv_nsec) / 1e9;
    printF("Origin+LZW -> Origin decode time:%f\n",elapsedTime_Origin_LZW_Check);
    
    // printLZWDict(&lzwDict,10);
    // printLZWDict(&lzwDict_Origin_LZW,10);
    // for (int i = 0; i < 10; i++)
    // {
    //     printF("result->data[%d]=%x,checkData[%d]=%x\n",i,result->data[i],i,newData_Origin_LZW_Check[i]);
    // }
    if(!checkData(result->data,newData_Origin_LZW_Check,totalDataLength_Origin,dataLength_Origin_LZW_Chekc) != 0){
        printF("Origin+LZW -> Origin check failed\n");
        // return false;
    }else{
        printF("Origin+LZW -> Origin check success\n");
    }
    // Origin+Huffman -> Huffman
    uint16_t dictSize_Origin_Huffman = totalDataLength_Origin_Huffman-dataLength_Origin_Huffman;
    uint8_t newData_Origin_Huffman_Check[MAX_DATA_SIZE];
    HuffmanTree huffmanTree_Origin_Huffman;
    uint16_t dataLength_Origin_Huffman_Check = 0;
    
    struct timespec startTime_Origin_Huffman_Check, endTime_Origin_Huffman_Check;
    clock_gettime(CLOCK_REALTIME, &startTime_Origin_Huffman_Check);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        initHuffmanTree(&huffmanTree_Origin_Huffman);
        memcpy(huffmanTree_Origin_Huffman.nodes,&(newData_Origin_Huffman[dataLength_Origin_Huffman]),dictSize_Origin_Huffman);
        huffmanTree_Origin_Huffman.size = dictSize_Origin_Huffman/sizeof(HuffmanTreeNode);
        huffmanTree_Origin_Huffman.root = huffmanTree_Origin_Huffman.size-1;
        dataLength_Origin_Huffman_Check = huffmanDecode(newData_Origin_Huffman, dataLength_Origin_Huffman, &huffmanTree_Origin_Huffman, newData_Origin_Huffman_Check, MAX_SERIALIZER_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Origin_Huffman_Check);
    double elapsedTime_Origin_Huffman_Check = (endTime_Origin_Huffman_Check.tv_sec - startTime_Origin_Huffman_Check.tv_sec) + 
                        (endTime_Origin_Huffman_Check.tv_nsec - startTime_Origin_Huffman_Check.tv_nsec) / 1e9;
    printF("Origin+Huffman -> Origin decode time:%f\n",elapsedTime_Origin_Huffman_Check);

    // printHuffmanTree(&huffmanTree,10);
    // printHuffmanTree(&huffmanTree_Origin_Huffman,10);
    
    if(!checkData(result->data,newData_Origin_Huffman_Check,totalDataLength_Origin,dataLength_Origin_Huffman_Check) != 0){
        printF("Origin+Huffman -> Origin check failed\n");
        // return false;
    }else{
        printF("Origin+Huffman -> Origin check success\n");
    }

    // deSerialize
    octoMap_t octoMap_Origin;
    struct timespec startTime_Origin_Deserialize, endTime_Origin_Deserialize;
    clock_gettime(CLOCK_REALTIME, &startTime_Origin_Deserialize);
    for (int i = 0; i < COUNT_TIME_NUMS; i++)
    {
        octoMapInit(&octoMap_Origin);
        deserializeOctoMap(&octoMap_Origin,result);
    }
    clock_gettime(CLOCK_REALTIME, &endTime_Origin_Deserialize);
    double elapsedTime_Origin_Deserialize = (endTime_Origin_Deserialize.tv_sec - startTime_Origin_Deserialize.tv_sec) + 
                        (endTime_Origin_Deserialize.tv_nsec - startTime_Origin_Deserialize.tv_nsec) / 1e9;
    printF("Origin deserialize time:%f\n",elapsedTime_Origin_Deserialize);

    printF("Lossy:%d,Lossy+LZW:%d,Lossy+Huffman:%d,Lossy+LZW+Huffman:%d\n",totalDataLength_Lossy,totalDataLength_Lossy_LZW,totalDataLength_Lossy_Huffman,totalDataLength_Lossy_LZW_Huffman);
    printF("Origin:%d,Origin+LZW:%d,Origin+Huffman:%d,Origin+LZW+Huffman:%d\n",totalDataLength_Origin,totalDataLength_Origin_LZW,totalDataLength_Origin_Huffman,totalDataLength_Origin_LZW_Huffman);
}