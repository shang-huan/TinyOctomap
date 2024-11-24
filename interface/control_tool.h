#ifndef CONTROL_TOOL_MUL_H_
#define CONTROL_TOOL_MUL_H_
#include "octoMap.h"
#include "auxiliary_tool.h"
#include "circularQueue.h"
#include "Packet.h"
#include "MySystem.h"
#define MAX_MULTIRANGER_UAV_NUM 1

#define MAX_LOOP 5 
#define WINDOW_SIZE 30 

#ifdef HOST
    #define AVOID_DISTANCE TREE_RESOLUTION
    #define STRIDE TREE_RESOLUTION
#else
    #define AVOID_DISTANCE 15
    #define STRIDE 8
#endif

#define DISCIPLINE 0

#define INCOME_INFO_TIMES 1 
#define COST_PRUNE_TIMES 1 
#define PROBABILITY_MEM(octomap) (double)octomap->octoNodeSet->length / NODE_SET_SIZE
#define DIRECTION_AWARD 1.2

#define JUMP_MAX_STEP 5

#define DETECT_DISTANCE 20.0
#define SAFE_DISTANCE 10.0

typedef struct uavControl_t
{
    // CoordinateQueue_t paths;
    coordinateF_t next_point;
    uavRange_t uavRange;

    float direction_weight[6];
    short lastdir;
    
    Queue_t queue;
    short loops[WINDOW_SIZE];
    
    bool flag_jump;
    direction_t Jump_Dir;
    uint8_t Jump_Rest_Step;
}uavControl_t;

void inituavRange(uavRange_t* uavRange);
void inituavControl(uavControl_t* uavControl);

void UpdateMap(octoMap_t *octoMap, mapping_req_payload_t *mappingRequestPayload,uint8_t uav_id);
direction_t GetRandomDir(measure_t *measurement);
void CalCandidates(coordinateF_t *candidates, measure_t *measurement, coordinateF_t *current_F);
bool CalBestCandinates(octoMap_t *octoMap,uavControl_t* uavControl,uavControl_t** uavs);
bool JumpLocalOp(uavControl_t *uavControl,uavControl_t** uavs);
bool UpdateLoops(uavControl_t* uavControl);
bool CalNextPoint(uavControl_t* uavControl,octoMap_t* octoMap,uavControl_t** uavs);
float CalMinDistance(uavControl_t* uavControl,uavControl_t** uavs, coordinateF_t* point);
float CalAvoidWeight(float distance);
#endif /* CONTROL_TOOL_MUL_H_ */