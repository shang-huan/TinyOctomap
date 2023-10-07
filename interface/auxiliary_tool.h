#ifndef AUXILIARYTOOL_H
#define AUXILIARYTOOL_H
#include <stdint.h>
#include "MySystem.h"
#include "octoTree.h"
#include "octoMap.h"


#define BOTTOM 20
#define TOP 60
#define WIDTH_X TREE_CENTER_X * 2
#define WIDTH_Y TREE_CENTER_Y * 2
#define WIDTH_Z TREE_CENTER_Z * 2
#define SENSOR_TH 300
// CostParameter
typedef enum direction_t{
    UP = 0,
    DOWN = 1,
    LEFT =  2,
    RIGHT = 3,
    FRONT = 4,
    BACK = 5,
    ERROR_DIR = 6
}direction_t;

typedef struct
{
    float x;
    float y;
    float z;
} coordinateF_t; // 浮点数坐标

typedef struct
{
    int data[6];
    float roll;
    float pitch;
    float yaw;
} measure_t; // 测距数据

typedef struct uavRange_t
{
    measure_t measurement;
    coordinateF_t current_point;
}uavRange_t;

typedef struct
{
    double cost_prune;    // cost of the node
    octoNode_t *node; // the node 
    double p_not_occupied; // the probability of the node is not occupied
} costParameter_t;

// Cost_C
typedef struct
{   
    double cost_prune;
    double income_info;
}Cost_C_t;
double caldistance(coordinateF_t* A,coordinateF_t* B);
void determine_threshold(coordinateF_t *point);
direction_t intTodirection(int dir);
// 坐标转换
coordinateF_t rot(float roll, float pitch, float yaw, coordinateF_t* origin, coordinateF_t* point);
void dot(float A[][3], float B[][1]);
// 终点坐标计算
bool calPoint_Sim(coordinateF_t* Start_Point,direction_t direction,int length,coordinateF_t* End_Point); // 仿真程序计算点坐标
bool cal_Point(measure_t* measurement,coordinateF_t* start_point,direction_t dir,coordinateF_t* res);   // 真实计算点坐标
bool cal_PointByLength(float length,float pitch,float roll,float yaw,coordinateF_t* start_point,direction_t dir,coordinateF_t *res); // 固定长度计算点坐标
// 收益计算
octoNode_t *findTargetParent(octoNode_t *octoNode, octoMap_t *octoMap, coordinate_t *point, coordinate_t origin, uint16_t* width, uint8_t* maxDepth); // 找到目标节点
costParameter_t Cost(coordinate_t *Point,octoTree_t *octoTree, octoMap_t *octoMap, octoNode_t *LastoctoNode); // 计算收益
Cost_C_t Cost_Sum(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t *start,direction_t dir); // 计算收益和
#endif