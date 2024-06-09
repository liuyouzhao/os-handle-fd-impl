#include "queue.h"
#include "defs.h"
#include <stdio.h>
#include <stdlib.h>

__PURE__ queue_node_t* queue_create_node(const long data) {
    queue_node_t* new_node = (queue_node_t*)malloc(sizeof(queue_node_t));
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

__PURE__ queue_t* queue_create_queue() {
    queue_t* queue = (queue_t*)malloc(sizeof(queue_t));
    queue->front = queue->rear = NULL;
    arch_spin_lock_init(&(queue->lock));
    return queue;
}

__ATOMIC__ void queue_enqueue(queue_t* queue, const long data) {
    queue_node_t* new_node = queue_create_node(data);

    arch_spin_lock(&(queue->lock));
    if (queue->rear == NULL) {
        queue->front = queue->rear = new_node;
        arch_spin_unlock(&(queue->lock));
        return;
    }
    queue->rear->next = new_node;
    queue->rear = new_node;
    arch_spin_unlock(&(queue->lock));
}

__ATOMIC__ long queue_dequeue(queue_t* queue) {
    if (queue->front == NULL) {
        return -1;
    }

    arch_spin_lock(&(queue->lock));
    queue_node_t* temp = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    arch_spin_unlock(&(queue->lock));

    int data = temp->data;
    free(temp);

    return data;
}

void queue_clean_queue(queue_t** queue) {
    while(queue_dequeue(*queue) >= 0);
    arch_spin_lock_destroy(&((*queue)->lock));
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
