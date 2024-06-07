#ifndef QUEUE_H
#define QUEUE_H

// Define the node structure
struct node {
    int data;
    struct node* next;
};

// Define the queue structure
struct queue {
    struct node* front;
    struct node* rear;
};

// Function to create a new node
struct node* queue_create_node(int data);

// Function to create an empty queue
struct queue* queue_create_queue();

// Function to add an element to the queue
void queue_enqueue(struct queue* queue, int data);

// Function to remove an element from the queue
int queue_dequeue(struct queue* queue);

// Function to display the queue
void queue_display_queue(struct queue* queue);


#endif // QUEUE_H
