#include "stdlib.h"
#include <stdint.h>
#include "crossSystem_tool.h"
#include "control_tool.h"
#include "auxiliary_tool.h"
#include "octoMap.h"
#include "octoTree.h"
#include "math1.h"
#include "math.h"

void inituavRange(uavRange_t* uavRange){
    uavRange->measurement.data[0] = 0;
    uavRange->measurement.data[1] = 0;
    uavRange->measurement.data[2] = 0;
    uavRange->measurement.data[3] = 0;
    uavRange->measurement.data[4] = 0;
    uavRange->measurement.data[5] = 0;
    uavRange->measurement.roll = 0;
    uavRange->measurement.pitch = 0;
    uavRange->measurement.yaw = 0;
    uavRange->current_point.x = 0;
    uavRange->current_point.y = 0;
    uavRange->current_point.z = 0;
}

void inituavControl(uavControl_t* uavControl){
    uavControl->next_point.x = 0;
    uavControl->next_point.y = 0;
    uavControl->next_point.z = 0;
    inituavRange(&uavControl->uavRange);
    for(int i = 0; i < 6; ++i){
        uavControl->direction_weight[i] = 1;
    }
    uavControl->lastdir = 0;
    initQueue(&uavControl->queue);
    for(int i = 0; i < WINDOW_SIZE; ++i){
        uavControl->loops[i] = 0;
    }
    uavControl->flag_jump = false;
    uavControl->Jump_Dir = FRONT;
    uavControl->Jump_Rest_Step = 0;
}


void UpdateMap(octoMap_t *octoMap, mapping_req_payload_t *mappingRequestPayload,uint8_t uav_id)
{
    for(int i = 0;i<mappingRequestPayload->len;++i){
        octoTreeRayCasting(octoMap->octoTree, octoMap, &mappingRequestPayload->startPoint, &mappingRequestPayload->endPoint[i], uav_id);
    }
}

direction_t GetRandomDir(measure_t *measurement)
{
    // Randomly sample twice to choose the larger
    direction_t dir = intTodirection(Myrand() % 6);
    direction_t maxdir = intTodirection(Myrand() % 6);
    int i = 0;
    // Guaranteed to get a feasible direction
    while ( measurement->data[maxdir] < STRIDE + fmax(AVOID_DISTANCE,BOTTOM) && i < 20)
    {
        if(measurement->data[maxdir] >= STRIDE + AVOID_DISTANCE && maxdir != DOWN){
            break;
        }
        maxdir = intTodirection(Myrand() % 6);
        ++i;
    }
    // Try to get a better and feasible direction
    dir = intTodirection(Myrand() % 6);
    ++i;
    if (i == 20)
        return ERROR_DIR;
    if (measurement->data[dir] > measurement->data[maxdir] && dir != DOWN)
        maxdir = dir;
    return maxdir;
}

void CalCandidates(coordinateF_t *candidates, measure_t *measurement, coordinateF_t *current_F)
{
    float pitch = -1 * measurement->pitch;
    float roll = measurement->roll;
    float yaw = measurement->yaw;
    direction_t dir = UP;
    for (int i = 0; i <= 5; ++i)
    {
        dir = intTodirection(i);
        if (measurement->data[dir] > AVOID_DISTANCE + STRIDE)
        {
            #ifdef HOST
                calPoint_Sim(current_F, dir, STRIDE, &candidates[dir]);
            #else
                cal_PointByLength(STRIDE, pitch, roll, yaw, current_F, dir, &candidates[dir]);
            #endif
        }
        else
        {
            candidates[dir].x = 30000;
            candidates[dir].y = 30000;
            candidates[dir].z = 30000;
        }
    }
}

bool CalBestCandinates(octoMap_t *octoMap,uavControl_t* uavControl,uavControl_t** uavs){
    // printF("CalBestCandinates\n");
    coordinateF_t candinates[6];
    coordinate_t item_point;
    double item_candinateCost = 0, max_candinateCost = 0;
    Cost_C_t item_sum,item_cost;
    CalCandidates(candinates, &uavControl->uavRange.measurement, &uavControl->uavRange.current_point);
    max_candinateCost = 0;
    short dir_next = -1;
    float min_distance = 10;
    // bool flagPrintF = false;
    // if(uavControl->uavRange.current_point.z == 20){
    //     flagPrintF = true;
    //     printF("-------------");
    // }
    for(int i = 0;i<6;++i){
        item_candinateCost = 0;
        item_sum.cost_prune = 0;
        item_sum.income_info = 0;
        if (candinates[i].x == 30000 && candinates[i].y == 30000 && candinates[i].z == 30000)
        {
            continue;
        }
        direction_t dir = UP;
        for (int j = 0; j <= 5; ++j)
        {
            dir = intTodirection(j);
            item_point.x = candinates[i].x;
            item_point.y = candinates[i].y;
            item_point.z = candinates[i].z;
            item_cost = Cost_Sum(octoMap->octoTree, octoMap, &item_point, dir);
            item_sum.cost_prune += item_cost.cost_prune;
            item_sum.income_info += item_cost.income_info;
        }
        if (item_sum.income_info == 0)
        {
            item_sum.income_info = DISCIPLINE;
        }
        // printF("cost_prune:%d,income_info:%d\n",item_sum.cost_prune,item_sum.income_info);
        min_distance = CalMinDistance(uavControl, uavs, &candinates[i]);
        item_candinateCost = (double)uavControl->direction_weight[i] * (PROBABILITY_MEM(octoMap) * item_sum.cost_prune * COST_PRUNE_TIMES +
                                                    (1.0 - PROBABILITY_MEM(octoMap)) * item_sum.income_info * INCOME_INFO_TIMES);
        if(CalAvoidWeight(min_distance) != 1){
            // printF("candinateCost_pre:%f,",item_candinateCost);
            item_candinateCost = CalAvoidWeight(min_distance) * item_candinateCost;
            // printF("candinateCost_re:%f\n",item_candinateCost);
        }
        if (item_candinateCost > max_candinateCost){
            dir_next = i;
            max_candinateCost = item_candinateCost;
        }
        // if(flagPrintF){
        //     printF("candinateCost:%f,dir_next:%d\n",item_candinateCost,dir_next);
        // }
    }
    if(dir_next != -1){
        uavControl->direction_weight[dir_next] = DIRECTION_AWARD;
        uavControl->direction_weight[(uavControl->lastdir)] = 1;
        (uavControl->lastdir) = dir_next;
        uavControl->next_point = candinates[dir_next];
        return true;
    }
    else{
        // printF("no next point\n");
        return false;
    }
}

bool JumpLocalOp(uavControl_t *uavControl,uavControl_t** uavs){
    float length = Myfmin(uavControl->uavRange.measurement.data[uavControl->Jump_Dir],300);
    coordinateF_t item_end_point;
    if(length > STRIDE + AVOID_DISTANCE && uavControl->Jump_Rest_Step > 0){
        if(length < STRIDE + BOTTOM && uavControl->Jump_Dir == DOWN){
            uavControl->flag_jump = false;
            uavControl->Jump_Rest_Step = 0;
            return false;
        }
        #ifdef HOST
            calPoint_Sim(&uavControl->uavRange.current_point, uavControl->Jump_Dir, STRIDE, &item_end_point);
        #else
            cal_PointByLength(STRIDE, -1 * uavControl->uavRange.measurement.pitch, uavControl->uavRange.measurement.roll, uavControl->uavRange.measurement.yaw, &uavControl->uavRange.current_point, uavControl->Jump_Dir, &item_end_point);
        #endif
        // 细节待商榷
        if(CalMinDistance(uavControl, uavs, &item_end_point) < SAFE_DISTANCE){
            uavControl->flag_jump = false;
            return false;
        }
        uavControl->next_point = item_end_point;
        --uavControl->Jump_Rest_Step;
        if(length < STRIDE * 2 + AVOID_DISTANCE){
            uavControl->flag_jump = false;
        }
        return true;
    }
    else{
        uavControl->flag_jump = false;
        uavControl->Jump_Rest_Step = 0;
        return false;
    }
}

bool CalNextPoint(uavControl_t* uavControl,octoMap_t* octoMap,uavControl_t** uavs){
    short index_loop = (((int)uavControl->uavRange.current_point.x + (int)uavControl->uavRange.current_point.y + (int)uavControl->uavRange.current_point.z) / TREE_RESOLUTION) % WINDOW_SIZE;;
    ++uavControl->loops[index_loop];
    if (uavControl->loops[index_loop] < MAX_LOOP)
    {
        push(&uavControl->queue, index_loop);
        if (uavControl->queue.len >= WINDOW_SIZE)
        {
            index_loop = pop(&uavControl->queue);
            --uavControl->loops[index_loop];
        }
        if(!uavControl->flag_jump && !CalBestCandinates(octoMap, uavControl,uavs)){
            uavControl->Jump_Dir = GetRandomDir(&uavControl->uavRange.measurement);
            if(uavControl->Jump_Dir == ERROR_DIR){
                printF("no next Jump_Dir\n\n\n\n\n\n");
                return false;
            }
            uavControl->flag_jump = true;
            uavControl->Jump_Rest_Step = JUMP_MAX_STEP;
        }
    }
    else
    {
        initQueue(&uavControl->queue);
        for (int i = 0; i < WINDOW_SIZE; ++i)
        {
            uavControl->loops[i] = 0;
        }
        uavControl->Jump_Dir = GetRandomDir(&uavControl->uavRange.measurement);
        if(uavControl->Jump_Dir == ERROR_DIR){
            printF("no next Jump_Dir\n\n\n\n\n\n\n");
            return false;
        }
        uavControl->flag_jump = true;
        uavControl->Jump_Rest_Step = JUMP_MAX_STEP;
    }
    if(uavControl->flag_jump){
        if(!JumpLocalOp(uavControl,uavs)){
            --uavControl->loops[index_loop];
            printF("isn't safe\n");
            return CalNextPoint(uavControl, octoMap,uavs);
        }
    }
    return true;
}

float CalMinDistance(uavControl_t* uavControl,uavControl_t** uavs, coordinateF_t* point){
    float min_distance = 30000;
    float distance = 0;
    for(int i = 0;i<MAX_MULTIRANGER_UAV_NUM;++i){
        if(uavs[i] == uavControl){
            continue;
        }
        distance = caldistance(&uavs[i]->uavRange.current_point, point);
        if(distance < min_distance){
            min_distance = distance;
        }
        distance = caldistance(&uavs[i]->next_point, point);
        if(distance < min_distance){
            min_distance = distance;
        }
    }
    return min_distance;
}

float CalAvoidWeight(float distance){
    if(distance > DETECT_DISTANCE){
        return 1;
    }
    else if(distance > SAFE_DISTANCE){
        return (float)(distance-SAFE_DISTANCE)/(DETECT_DISTANCE-SAFE_DISTANCE);
    }
    else{
        return 0;
    }
}