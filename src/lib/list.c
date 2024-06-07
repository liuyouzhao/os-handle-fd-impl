#include <stdio.h>
#include <stdlib.h>
#include "list.h"

node_t* list_create_node(unsigned long data) {
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    if (!new_node) {
        printf("Memory allocation error\n");
        exit(1);
    }
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

node_t* list_append_node(node_t* head, unsigned long data) {
    node_t* new_node = list_create_node(data);
    if (head == NULL) {
        return new_node;
    }
    node_t* temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = new_node;
    return head;
}

node_t* list_remove_node(node_t* head, unsigned long data) {
    if (head == NULL) {
        return NULL;
    }

    node_t* temp = head;
    node_t* prev = NULL;

    if (temp != NULL && temp->data == data) {
        head = temp->next;
        free(temp);
        return head;
    }

    while (temp != NULL && temp->data != data) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        return head;
    }

    prev->next = temp->next;

    free(temp);
    return head;
}

void list_print_list(node_t* head) {
    node_t* temp = head;
    while (temp != NULL) {
        printf("%lu -> ", temp->data);
        temp = temp->next;
    }
    printf("NULL\n");
}

node_t* list_delete_list(node_t* head) {
    node_t* current = head;
    node_t* next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    return NULL;
}
