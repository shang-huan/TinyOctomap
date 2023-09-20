#ifndef CIRCULARQUEUE_H
#define CIRCULARQUEUE_H
#define MAX_QUEUE_SIZE 150
#include "stdbool.h"

typedef struct{
    short data[MAX_QUEUE_SIZE];
    short front;
    short tail;
    short len;
}Queue_t;

void initQueue(Queue_t *queue);
void push(Queue_t *queue, short data);
short pop(Queue_t *queue);
bool isQueueEmpty(Queue_t *queue);
bool isQueueFull(Queue_t *queue);
#endif