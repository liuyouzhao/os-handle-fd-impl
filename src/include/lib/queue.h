#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue_node_s {
    long data;
    struct queue_node_s* next;
} queue_node_t;

typedef struct queue_s {
    queue_node_t* front;
    queue_node_t* rear;
} queue_t;

queue_node_t* queue_create_node(long data);
queue_t* queue_create_queue();
void queue_enqueue(queue_t* queue, long data);
long queue_dequeue(queue_t* queue);
void queue_display_queue(queue_t* queue);
void queue_clean_queue(queue_t** queue);

#endif // QUEUE_H
