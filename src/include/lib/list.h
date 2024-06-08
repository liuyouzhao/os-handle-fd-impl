#ifndef LIST_H
#define LIST_H

typedef struct list_node_s {
    unsigned long data;
    struct list_node_s* next;
} list_node_t;

list_node_t* list_create_node(unsigned long data);
list_node_t* list_append_node(list_node_t* head, unsigned long data);
list_node_t* list_remove_node(list_node_t* head, unsigned long data);
void list_dump_list(list_node_t* head, int (*dump_hook)(unsigned long, unsigned long));
list_node_t* list_delete_list(list_node_t* head);

#endif // LIST_H
