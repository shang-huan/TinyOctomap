#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "coordinateQueue.h"
#include "crossSystem_tool.h"
#include <stdint.h>
void initCoordinateQueue(CoordinateQueue_t *queue) {
    queue->front = 0;
    queue->tail = 0;
    queue->len = 0;
}

bool push_CoordinateQueue(CoordinateQueue_t *queue, coordinate_t data){
    if(isCoordinateQueueFull(queue)){
        printF("CoordinateQueue is full!\n");
        return false;
    }
    queue->data[queue->tail] = data;
    queue->tail = (queue->tail + 1) % MAX_QUEUE_SIZE;
    ++queue->len;
    return true;
}

coordinate_t pop_CoordinateQueue(CoordinateQueue_t *queue){
    if(isCoordinateQueueEmpty(queue)){
        printF("CoordinateQueue is empty!\n");
        coordinate_t data = {static_cast<uint16_t>(-1),static_cast<uint16_t>(-1),static_cast<uint16_t>(-1)};
        return data;
    }
    coordinate_t data = queue->data[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    --queue->len;
    return data;
}

bool isCoordinateQueueEmpty(CoordinateQueue_t *queue){
    // return queue->front == queue->tail;
    return queue->len == 0;
}

bool isCoordinateQueueFull(CoordinateQueue_t *queue){
    // return (queue->tail + 1) % MAX_QUEUE_SIZE == queue->front;
    return queue->len == MAX_QUEUE_SIZE;
}
