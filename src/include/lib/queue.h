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
struct node* create_node(int data);

// Function to create an empty queue
struct queue* create_queue();

// Function to add an element to the queue
void enqueue(struct queue* queue, int data);

// Function to remove an element from the queue
int dequeue(struct queue* queue);

// Function to display the queue
void display_queue(struct queue* queue);


#endif // QUEUE_H
