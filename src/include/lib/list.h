#ifndef LIST_H
#define LIST_H

typedef struct node_s {
    unsigned long data;
    struct node_s* next;
} node_t;

typedef node_t list_node_t;

node_t* list_create_node(unsigned long data);
node_t* list_append_node(node_t* head, unsigned long data);
node_t* list_remove_node(node_t* head, unsigned long data);
void list_print_list(node_t* head);
node_t* list_delete_list(node_t* head);

// int main() {
//     node_t* head = NULL;

//     head = append_node(head, 10);
//     head = append_node(head, 20);
//     head = append_node(head, 30);

//     printf("Linked List: ");
//     print_list(head);

//     head = remove_node(head, 20);
//     printf("Linked List after removing 20: ");
//     print_list(head);

//     head = delete_list(head);

//     return 0;
// }

#endif // LIST_H
