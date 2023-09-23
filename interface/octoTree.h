#ifndef __OCTOTREE_H__
#define __OCTOTREE_H__
#include <stdint.h>
#include "octoMap.h"
#include "octoNode.h"

//#define BOOL int
#define TRUE 1
#define FALSE 0

#define P_GLOBAL 0.2
#define MIN_OCCUPIED 6
#define MAX_NOT_OCCUPIED 0

typedef enum direction_t{
    UP = 0,
    DOWN = 1,
    LEFT =  2,
    RIGHT = 3,
    FRONT = 4,
    BACK = 5
}direction_t;


octoTree_t* octoTreeInit();
void octoTreeInsertPoint(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t *point, uint8_t diffLogOdds, uint8_t uav_id);
void octoTreeRayCasting(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t *startPoint, coordinate_t *endPoint, uint8_t uav_id);
uint8_t octoTreeGetLogProbability(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t *point);
void bresenham3D(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t *startPoint, coordinate_t *endPoint, uint8_t uav_id);
direction_t increment_direction(direction_t dir);

double caldistance(coordinate_t* A,coordinate_t* B);
octoNode_t *findTargetParent(octoNode_t *octoNode, octoMap_t *octoMap, coordinate_t *point, coordinate_t origin, uint16_t* width, uint8_t* maxDepth);
costParameter_t Cost(coordinate_t *Point,octoTree_t *octoTree, octoMap_t *octoMap, octoNode_t *LastoctoNode);
double Cost_Sum(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t *start, coordinate_t *end);
Cost_C_t Cost_Sum_simulate(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t *start,direction_t dir);

#endif
