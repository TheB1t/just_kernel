#pragma once

#include <klibc/stdlib.h>

#define VECTOR_INIT_SIZE    4
#define PTR_SIZE            sizeof(void*)

typedef struct {
    void**      items;
    uint32_t    max_items;
    uint32_t    items_count;
} vector_t;

void    vector_init(vector_t *v);
void*   vector_get(vector_t *v, uint32_t index);
void    vector_resize(vector_t *v, uint32_t new_size);
void    vector_add(vector_t *v, void *new_item);
void    vector_delete(vector_t *v, uint32_t index);
void**  vector_items(vector_t *v);
void    vector_uninit(vector_t *v);