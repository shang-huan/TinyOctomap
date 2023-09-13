#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "coordinateQueue.h"

void initCoordinateQueue(CoordinateQueue_t *queue) {
    queue->front = 0;
    queue->tail = 0;
    queue->len = 0;
}

bool push_CoordinateQueue(CoordinateQueue_t *queue, coordinate_t data){
    if(isCoordinateQueueFull(queue)){
        printf("CoordinateQueue is full!\n");
        return false;
    }
    queue->data[queue->tail] = data;
    queue->tail = (queue->tail + 1) % MAX_QUEUE_SIZE;
    ++queue->len;
    return true;
}

coordinate_t pop_CoordinateQueue(CoordinateQueue_t *queue){
    if(isCoordinateQueueEmpty(queue)){
        printf("CoordinateQueue is empty!\n");
        coordinate_t data = {-1,-1,-1};
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
