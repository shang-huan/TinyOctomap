#ifndef COORDINATEQUEUE_H
#define COORDINATEQUEUE_H
#include "stdint.h"
#define MAX_QUEUE_SIZE 150
#include "stdbool.h"

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
} coordinate_t;

typedef struct{
    coordinate_t data[MAX_QUEUE_SIZE];
    short front;
    short tail;
    short len;
}CoordinateQueue_t;

void initCoordinateQueue(CoordinateQueue_t *queue);
bool push_CoordinateQueue(CoordinateQueue_t *queue, coordinate_t data);
coordinate_t pop_CoordinateQueue(CoordinateQueue_t *queue);
bool isCoordinateQueueEmpty(CoordinateQueue_t *queue);
bool isCoordinateQueueFull(CoordinateQueue_t *queue);
#endif