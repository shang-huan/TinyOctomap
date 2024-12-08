#ifndef HUFFMANTREE_H
#define HUFFMANTREE_H
#include "compressBaseStruct.h"
#include "stdbool.h"
#include "stdint.h"

#define MAX_HUFFMAN_TREE_SIZE 512

typedef struct {
    uint16_t index;
    times_t times;
}HeapNode;

typedef struct HuffmanTreeNode
{
    value_t value;
    times_t times;
    uint16_t left;
    uint16_t right; //对于空闲节点指向下一个空闲节点
} HuffmanTreeNode;

typedef struct HuffmanTree
{
    uint16_t size;  //树的大小
    uint16_t root;  //根节点
    uint16_t free; //指向第一个空闲节点
    HuffmanTreeNode nodes[MAX_HUFFMAN_TREE_SIZE];
} HuffmanTree;

void initHuffmanTree(HuffmanTree *tree);
uint16_t mallocNode(HuffmanTree *tree);
void freeNode(HuffmanTree *tree, uint16_t index);

uint16_t huffmanCode(uint8_t *data, uint16_t dataLength, HuffmanTree *tree, uint8_t *newData, uint16_t newDataMAXLength);

void printHuffmanTree(HuffmanTree *tree);
#endif