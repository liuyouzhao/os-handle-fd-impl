#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dentry.h"

static dentry_t* _search_sub_path(const char* subpath, dentry_t* cur_dir) {
    list_node_t* pcur = cur_dir->d_subdirs;
    while(pcur) {
        if(strcmp(PTR_DEN(pcur->data)->d_name, subpath)) {
            return PTR_DEN(pcur->data);
        }
        pcur = pcur->next;
    }
    return NULL;
}

//static void _insert_dentry()

dentry_t* dentry_create(const char* path, dentry_t* root) {
    char *pch = NULL;
    dentry_t* cur_dir_den = root;
    dentry_t* lst_dir_den = root;
    do {
        pch = strtok ((char*)path, "/");
        lst_dir_den = cur_dir_den;
        cur_dir_den = _search_sub_path(pch, cur_dir_den);
        if(!cur_dir_den) {

        }
    } while(pch);
    return cur_dir_den;
}

dentry_t* dentry_delete(const char* path, dentry_t* root) {

}

dentry_t* dentry_find(const char* path, dentry_t* root) {
    char *pch = NULL;
    dentry_t* cur_dir_den = root;
    do {
        pch = strtok ((char*)path, "/");
        cur_dir_den = _search_sub_path(pch, cur_dir_den);
        if(!cur_dir_den) {
            return NULL;
        }
    } while(pch);
    return cur_dir_den;
}
