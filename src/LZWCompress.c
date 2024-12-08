#include "stdlib.h"
#include "stdbool.h"
#include "LZWCompress.h"
#include "crossSystem_tool.h"

uint16_t mallocTrieNode(Trie* tree);
uint16_t addLZWDictRecode(LZWDict* lzwDict,value_t value, uint16_t pre);
uint16_t searchTrieNode(Trie* tree, TrieNode* root, value_t value);
bool addTrieNode(Trie* tree, TrieNode* root, value_t value, uint16_t seq);

void initLZWDict(LZWDict* lzwDict){
    lzwDict->size = 0;
    for(int i = 0;i<MAX_LZWDICT_SIZE;++i){
        lzwDict->nodes[i].pre = NULL_SEQ;
        lzwDict->nodes[i].value = NULL_VALUE;
    }
}

uint16_t addLZWDictRecode(LZWDict* lzwDict,value_t value, uint16_t pre){
    if(lzwDict->size >= MAX_LZWDICT_SIZE){
        printF("LZWDict is full\n");
        return NULL_SEQ;
    }
    lzwDict->nodes[lzwDict->size].pre = pre;
    lzwDict->nodes[lzwDict->size].value = value;
    lzwDict->size++;
    return lzwDict->size-1;
}

void initTrie(Trie* tree){
    tree->size = 0;
    tree->root = 0;
    for(int i = 0;i<MAX_TRIENODE_NUM;++i){
        tree->nodes[i].value = NULL_VALUE;
        tree->nodes[i].seq = NULL_SEQ;
        tree->nodes[i].child = NULL_SEQ;
        tree->nodes[i].brother = NULL_SEQ;
    }
}

uint16_t mallocTrieNode(Trie* tree){
    if(tree->size >= MAX_TRIENODE_NUM){
        printF("Trie is full\n");
        return NULL_SEQ;
    }
    uint16_t index = tree->size;
    tree->size++;
    return index;
}

uint16_t searchTrieNode(Trie* tree, TrieNode* root, value_t value){
    uint16_t p = root->child;
    while (p != NULL_SEQ)
    {
        if(tree->nodes[p].value == value){
            return p;
        }
        p = tree->nodes[p].brother;
    }
    return NULL_SEQ;
}

bool addTrieNode(Trie* tree, TrieNode* root, value_t value, uint16_t seq){
    uint16_t p = root->child;
    if(p == NULL_SEQ){
        uint16_t index = mallocTrieNode(tree);
        if(index == NULL_SEQ){
            return false;
        }
        tree->nodes[index].value = value;
        tree->nodes[index].seq = seq;
        root->child = index;
        return true;
    }
    if(tree->nodes[p].value == value){
        return true;
    }
    uint16_t q = tree->nodes[p].brother;
    while (q != NULL_SEQ)
    {
        if(tree->nodes[q].value == value){
            return true;
        }
        p = q;
        q = tree->nodes[q].brother;
    }
    uint16_t index = mallocTrieNode(tree);
    if(index == NULL_SEQ){
        return false;
    }
    tree->nodes[index].value = value;
    tree->nodes[index].seq = seq;
    tree->nodes[p].brother = index;
    return true;
}

uint16_t LZWCompressData(uint8_t* data,uint16_t dataLength,LZWDict* lzwDict, uint8_t* newData, uint16_t newDataMaxLength){

    Trie trie;
    initTrie(&trie);
    TrieNode* root = &trie.nodes[0];

    initLZWDict(lzwDict);

    uint16_t top = 0;
    uint8_t curDepth = 0;

    TrieNode* p = root;
    for(int i = 0; i < dataLength; i++){
        uint8_t val = data[i];
        uint16_t child = searchTrieNode(&trie, p, val);
        if(child != NULL_SEQ){
            p = &trie.nodes[child];
            curDepth++;
            if(curDepth == MAX_TRIE_DEPTH){
                // 达到最大深度，重新开始
                if(top >= newDataMaxLength){
                    printF("newData is full\n");
                    return 0;
                }
                newData[top++] = p->seq;
                p = root;
                curDepth = 0;
            }
        }else{
            // 未找到，添加到字典
            uint16_t seq = addLZWDictRecode(lzwDict, val, p->seq);
            if(seq == NULL_SEQ){
                printF("addLZWDictRecode failed\n");
                return 0;
            }
            bool res = addTrieNode(&trie, p, val, seq);
            if(!res){
                printF("addTrieNode failed\n");
                return 0;
            }
            // 记录当前节点
            if(top >= newDataMaxLength){
                printF("newData is full\n");
                return 0;
            }
            newData[top++] = p->seq;
            p = root;
            curDepth = 0;
            // 回溯，重新检查当前数据
            i--;
        }
    }
    return top;
}