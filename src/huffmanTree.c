#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"

#include "crossSystem_tool.h"
#include "compressBaseStruct.h"
#include "huffmanTree.h"

uint32_t newDataExpectedLength = 0;

bool push(HeapNode *heap, uint16_t index, times_t times, int size);
HeapNode pop(HeapNode *heap, int size);
void adjustHeapUp(HeapNode *heap, int i, int size);
void adjustHeapDown(HeapNode *heap, int i, int size);
bool processHuffmanTree(HuffmanTree *tree, uint16_t root, uint8_t length, long_value_t value, long_value_t* values);

void initHuffmanTree(HuffmanTree *tree){
    tree->size = 0;
    tree->root = NULL_SEQ;
    tree->free = 0;
    for(int i = 0; i < MAX_HUFFMAN_TREE_SIZE; i++){
        tree->nodes[i].value = NULL_VALUE;
        tree->nodes[i].times = NULL_TIMES;
        tree->nodes[i].left = NULL_SEQ;
        tree->nodes[i].right = i+1;
    }
    tree->nodes[MAX_HUFFMAN_TREE_SIZE-1].right = NULL_SEQ;
}

uint16_t mallocNode(HuffmanTree *tree){
    if(tree->free == NULL_SEQ){
        printF("HuffmanTree malloc Node failed\n");
        return NULL_SEQ;
    }
    
    uint16_t index = tree->free;
    tree->free = tree->nodes[index].right;
    // 初始化node
    tree->nodes[index].left = NULL_SEQ;
    tree->nodes[index].right = NULL_SEQ;
    tree->size++;
    return index;
}

void freeNode(HuffmanTree *tree, uint16_t index){
    if(tree->nodes[index].left != NULL_SEQ){
        freeNode(tree, tree->nodes[index].left);
    }
    if(tree->nodes[index].right != NULL_SEQ){
        freeNode(tree, tree->nodes[index].right);
    }
    tree->nodes[index].value = NULL_VALUE;
    tree->nodes[index].left = NULL_SEQ;
    tree->nodes[index].right = tree->free;
    tree->free = index;
    tree->size--;
}

// uint16_t **heap [index, times],index is the index of huffmanTree, times is the times of value
bool push(HeapNode *heap, uint16_t index, times_t times, int size){
    heap[size].times = times;
    heap[size].index = index;
    adjustHeapUp(heap, size, size+1);
}

HeapNode pop(HeapNode *heap, int size){
    HeapNode res = heap[0];
    heap[0] = heap[size-1];
    adjustHeapDown(heap, 0, size-1);
    return res;
}

void adjustHeapUp(HeapNode *heap, int i, int size){
    int parent = (i-1)/2;
    while(i > 0 && heap[parent].times > heap[i].times){
        HeapNode temp = heap[parent];
        heap[parent] = heap[i];
        heap[i] = temp;
        i = parent;
        parent = (i-1)/2;
    }
}

void adjustHeapDown(HeapNode *heap, int i, int size){
    int left = 2*i+1;
    while (left < size)
    {
        if(left+1 < size && heap[left+1].times < heap[left].times){
            left++;
        }
        if(heap[i].times <= heap[left].times){
            break;
        }
        HeapNode temp = heap[i];
        heap[i] = heap[left];
        heap[left] = temp;
        i = left;
        left = 2*i+1;
    }
}

uint16_t huffmanCode(uint8_t *data, uint16_t dataLength, HuffmanTree *tree, uint8_t *newData, uint16_t newDataMAXLength){
    dict_t dict;
    initDict(&dict);
    if(!fillDictFromData(data, dataLength, &dict)){
        printF("fillDictFromData failed\n");
        return NULL_SEQ;
    }
    if(dict.size == 0){
        printF("dict is empty\n");
        return NULL_SEQ;
    }

    initHuffmanTree(tree);

    HeapNode heap[MAX_DICT_SIZE];
    uint16_t heapSize = 0;
    for (int i = 0; i < dict.size; i++)
    {
        uint16_t index = mallocNode(tree);
        if(index == NULL_SEQ){
            printF("mallocNode failed\n");
            return NULL_SEQ;
        }
        tree->nodes[index].value = dict.value[i];
        tree->nodes[index].times = dict.times[i];
        push(heap, index, dict.times[i], heapSize);
        heapSize++;
    }

    // 构建Huffman树
    while (heapSize > 1)
    {
        HeapNode right = pop(heap, heapSize);
        heapSize--;
        HeapNode left = pop(heap, heapSize);
        heapSize--;
        uint16_t index = mallocNode(tree);
        if(index == NULL_SEQ){
            printF("mallocNode failed\n");
            return NULL_SEQ;
        }
        tree->nodes[index].left = left.index;
        tree->nodes[index].right = right.index;
        push(heap, index, left.times+right.times, heapSize);
        heapSize++;
    }
    tree->root = heap[0].index;
    printF("tree.size:%d\n",tree->size);

    // 生成Huffman编码
    long_value_t values[256];
    newDataExpectedLength = 0;
    if(!processHuffmanTree(tree,tree->root,0,0,values)){
        return NULL_SEQ;
    }
    printF("newDataExpectedLength:%d\n",newDataExpectedLength);
    if(newDataExpectedLength > newDataMAXLength*8){
        printF("newDataExpectedLength:%d,newDataMAXLength:%d,newData will overflow\n",newDataExpectedLength,newDataMAXLength);
        return NULL_SEQ;
    }

    
    

    // 生成newData
    uint8_t curRest = 8;
    uint16_t newDataTop = 0;
    uint16_t itemLength = 0;

    for (int i = 0; i < dataLength; i++)
    {
        uint8_t val = data[i];
        uint32_t code = values[val];
        // printF("val:%d,length:%d,value:%X,code:%X\n",val,code>>24,code & ((1<<24)-1),code);
        uint32_t length = code>>24;
        code = code & ((1<<length)-1);
        while(curRest < length){
            newData[newDataTop] |= code>>(length-curRest);
            code = code & ((1<<(length-curRest))-1);
            length -= curRest;
            curRest = 8;
            newDataTop++;
            if(newDataTop >= newDataMAXLength){
                printF("newDataTop:%d,newDataMAXLength:%d,newData is full\n",newDataTop,newDataMAXLength);
                return NULL_SEQ;
            }
            newData[newDataTop] = 0;
        }
        curRest -= length;
        newData[newDataTop] |= code<<curRest;
        if(curRest == 0){
            curRest = 8;
            newDataTop++;
            if(newDataTop >= newDataMAXLength){
                printF("newDataTop:%d,newDataMAXLength:%d,newData is full\n",newDataTop,newDataMAXLength);
                return NULL_SEQ;
            }
            newData[newDataTop] = 0;
        }
    }
    if(curRest == 8){
        newDataTop--;
    }
    return newDataTop;
}

// 递归遍历Huffman树，生成Huffman编码
bool processHuffmanTree(HuffmanTree *tree, uint16_t root, uint8_t length, long_value_t value, long_value_t* values){
    // 左取0，右取1
    if(tree->nodes[root].left != NULL_SEQ){
        if(!processHuffmanTree(tree, tree->nodes[root].left, length+1, value<<1, values)){
            return false;
        }
    }
    if(tree->nodes[root].right != NULL_SEQ){
        if(!processHuffmanTree(tree, tree->nodes[root].right, length+1, (value<<1)+1, values)){
            return false;
        }
    }
    if(tree->nodes[root].left == NULL_SEQ && tree->nodes[root].right == NULL_SEQ){
        if(length > 24){
            printF("HuffmanTree length is too long\n");
            return false;
        }
        
        long_value_t code = (length<<24) | value;
        printF("root:%d,length:%d,value:%X,code:%X,index:%d,times:%d\n",root,length,value,code,tree->nodes[root].value,tree->nodes[root].times);
        newDataExpectedLength += length * tree->nodes[root].times;
        // 记录旧字典和Huffman字典的对应关系
        values[tree->nodes[root].value] = code;
    }
    return true;
}

void printHuffmanTree(HuffmanTree *tree){
    for (int i = 0; i < MAX_HUFFMAN_TREE_SIZE; i++)
    {
        if(tree->nodes[i].value != NULL_VALUE){
            printF("index:%d,value:%X,left:%d,right:%d\n",i,tree->nodes[i].value,tree->nodes[i].left,tree->nodes[i].right);
        }
    }
}
