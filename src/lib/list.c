#include <stdio.h>
#include <stdlib.h>
#include "list.h"

list_node_t* list_create_node(unsigned long data) {
    list_node_t* new_node = (list_node_t*)malloc(sizeof(list_node_t));
    if (!new_node) {
        printf("Memory allocation error\n");
        exit(1);
    }
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

list_node_t* list_append_node(list_node_t* head, unsigned long data) {
    list_node_t* new_node = list_create_node(data);
    if (head == NULL) {
        return new_node;
    }
    list_node_t* temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = new_node;
    return head;
}

list_node_t* list_remove_node(list_node_t* head, unsigned long data) {
    if (head == NULL) {
        return NULL;
    }

    list_node_t* temp = head;
    list_node_t* prev = NULL;

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

void list_dump_list(list_node_t* head, int (*dump_hook)(unsigned long, unsigned long)) {
    list_node_t* temp = head;
    unsigned long idx = 0;
    while (temp != NULL) {
        if(!dump_hook) {
            printf("%lu -> ", temp->data);
        }
        else {
            if(dump_hook(idx ++, temp->data)) {
                return;
            }
        }
        temp = temp->next;
    }
}

list_node_t* list_delete_list(list_node_t* head) {
    list_node_t* current = head;
    list_node_t* next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    return NULL;
}
