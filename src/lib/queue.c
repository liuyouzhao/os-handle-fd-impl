#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

struct node* queue_create_node(int data) {
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

struct queue* queue_create_queue() {
    struct queue* queue = (struct queue*)malloc(sizeof(struct queue));
    queue->front = queue->rear = NULL;
    return queue;
}

void queue_enqueue(struct queue* queue, int data) {
    struct node* new_node = queue_create_node(data);
    if (queue->rear == NULL) {
        queue->front = queue->rear = new_node;
        return;
    }
    queue->rear->next = new_node;
    queue->rear = new_node;
}

int queue_dequeue(struct queue* queue) {
    if (queue->front == NULL) {
        printf("Queue is empty\n");
        return -1;
    }
    struct node* temp = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    int data = temp->data;
    free(temp);
    return data;
}

void queue_display_queue(struct queue* queue) {
    struct node* temp = queue->front;
    while (temp != NULL) {
        printf("%d ", temp->data);
        temp = temp->next;
    }
    printf("\n");
}
