#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>

typedef struct {
    struct	list_head   head;
} plist_root_t;

typedef struct {
    void*               ptr;
    struct	list_head   list;
} plist_node_t;

void            plist_init(plist_root_t* root);
uint32_t        plist_size(plist_root_t* root);
void*           plist_get(struct list_head* iter);
void            plist_add(plist_root_t* root, plist_node_t* node);
void            plist_del(plist_node_t* node);
plist_node_t*   plist_alloc_node(void* ptr);
void            plist_free_node(plist_node_t* node);