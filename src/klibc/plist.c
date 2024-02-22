#include <klibc/plist.h>

void plist_init(plist_root_t* root) {
    INIT_LIST_HEAD(LIST_GET_HEAD(root));
}

uint32_t plist_size(plist_root_t* root) {
    uint32_t size = 0;
    struct list_head* iter;

    list_for_each(iter, LIST_GET_HEAD(root)) {
        size++;
    }

    return size;
}

void* plist_get(struct list_head* iter) {
    return list_entry(iter, plist_node_t, list)->ptr;
}

void plist_add(plist_root_t* root, plist_node_t* node) {
    list_add_tail(LIST_GET_LIST(node), LIST_GET_HEAD(root));
}

void plist_del(plist_node_t* node) {
    list_del(LIST_GET_LIST(node));
}

plist_node_t* plist_alloc_node(void* ptr) {
    plist_node_t* node = (plist_node_t*)kmalloc(sizeof(plist_node_t));
    node->ptr = ptr;
    LIST_GET_LIST(node)->prev = (void*)0;
    LIST_GET_LIST(node)->next = (void*)0;
    return node;
}

void plist_free_node(plist_node_t* node) {
    plist_del(node);
    kfree(node);
}