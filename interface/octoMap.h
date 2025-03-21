#ifndef __OCTOMAP_H__
#define __OCTOMAP_H__
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "MySystem.h"
// for weak platform
typedef uint16_t setIndex_t;

#ifdef CRAZYFLIE    
    // 弱平台
    #define NODE_SET_SIZE 4096
#else
    #define NODE_SET_SIZE (4096*8)
#endif

#define BOOL uint16_t
#define TRUE 1
#define FALSE 0

// 树范围
#define TREE_CENTER_X 256
#define TREE_CENTER_Y 256
#define TREE_CENTER_Z 256
#define TREE_RESOLUTION 4
#define TREE_MAX_DEPTH 7

// 节点状态更新及更新布长
#define LOG_ODDS_OCCUPIED 6
#define LOG_ODDS_FREE 0
#define LOG_ODDS_UNKNOWN 3
#define LOG_ODDS_DIFF_STEP 3
// 节点状态
#define LOG_ODDS_OCCUPIED_FLAG 1
#define LOG_ODDS_FREE_FLAG 0

// coordinate
typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
} coordinate_t;

// OctoNode
#ifdef CRAZYFLIE
    // 弱平台
    typedef struct
    {
        uint16_t children : 12 ; // first child node index (the following 7 children are in order, rft, rbt, lbt, lft, rfn, rbn, lbn, lfn)
        uint16_t logOdds : 3 ;   // occupation probability level
        uint16_t isLeaf : 1 ;    // whether is leaf node
    }
#else
    typedef struct
    {
        uint16_t children ; // first child node index (the following 7 children are in order, rft, rbt, lbt, lft, rfn, rbn, lbn, lfn)
        uint16_t logOdds ;   // occupation probability level
        uint16_t isLeaf ;    // whether is leaf node
        coordinate_t origin;    // origin coordinate of the voxel node
        uint16_t width;
        uint8_t uav_id;
    } octoNode_t;
#endif

// OctoNodeSet
typedef struct
{
    octoNode_t data[8]; // data of the item
    setIndex_t next; // next item index
} octoNodeSetItem_t;

typedef struct
{
    octoNodeSetItem_t setData[NODE_SET_SIZE];     // data set
    setIndex_t freeQueueEntry;                    // first free item index
    setIndex_t fullQueueEntry;                    // first full item index
    short numOccupied;  // number of occupied nodes
    short numFree;  // number of free nodes
    short length;   
    unsigned int volumeFree;  // volume of free nodes
    unsigned int volumeOccupied; // volume of occupied nodes
} octoNodeSet_t;

// OctoTree
typedef struct
{
    coordinate_t center;     // the coordinate of the center --- (x,y,z): tuple
    coordinate_t origin;     // the origin coordinate of this tree --- (x,y,z): tuple
    uint8_t resolution;      // resolution of the tree
    uint8_t maxDepth;        // max depth of the tree
    uint16_t width;          // width of the tree
    octoNode_t *root;        // root node of the tree
} octoTree_t;

// OctoMap
typedef struct
{
    octoTree_t *octoTree;
    octoNodeSet_t *octoNodeSet;
} octoMap_t;

void octoMapInit(octoMap_t *octoMap);
void recursiveExportOctoMap(octoMap_t* octoMap, octoNode_t* node, coordinate_t origin, uint16_t width, FILE* f_octoMap);
#endif
