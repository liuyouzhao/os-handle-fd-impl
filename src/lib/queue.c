#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

queue_node_t* queue_create_node(long data) {
    queue_node_t* new_node = (queue_node_t*)malloc(sizeof(queue_node_t));
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

queue_t* queue_create_queue() {
    queue_t* queue = (queue_t*)malloc(sizeof(queue_t));
    queue->front = queue->rear = NULL;
    return queue;
}

void queue_enqueue(queue_t* queue, long data) {
    queue_node_t* new_node = queue_create_node(data);
    if (queue->rear == NULL) {
        queue->front = queue->rear = new_node;
        return;
    }
    queue->rear->next = new_node;
    queue->rear = new_node;
}

long queue_dequeue(queue_t* queue) {
    if (queue->front == NULL) {
        printf("Queue is empty\n");
        return -1;
    }
    queue_node_t* temp = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    int data = temp->data;
    free(temp);
    return data;
}

void queue_clean_queue(queue_t** queue) {
    while(queue_dequeue(*queue) >= 0);
    *queue = NULL;
}

void queue_display_queue(queue_t* queue) {
    queue_node_t* temp = queue->front;
    while (temp != NULL) {
        printf("%ld ", temp->data);
        temp = temp->next;
    }
    printf("\n");
}
