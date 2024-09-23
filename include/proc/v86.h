#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>

#define v86_num_calls(list)         list_size(LIST_GET_HEAD(list))
#define v86_call_next(list)         entry_v86_call(LIST_GET_HEAD(list)->next)
#define entry_v86_call(iter)        list_entry(iter, v86_call_t, list)

#define add_v86_call(list, call)    list_add_tail(LIST_GET_LIST(call), LIST_GET_HEAD(list))
#define del_v86_call(call)          list_del(LIST_GET_LIST(call))

typedef struct {
    uint16_t    di;
    uint16_t    si;
    uint16_t    bp;
    uint16_t    sp;
    uint16_t    bx;
    uint16_t    dx;
    uint16_t    cx;
    uint16_t    ax;
} v86_ctx_t;

typedef struct {
    uint16_t     off;
    uint16_t     seg;
} ivt_entry_t;

typedef enum {
    V86_CALL_TYPE_COMPLETE,
    V86_CALL_TYPE_GENERIC_INT,
} v86_call_type_t;

typedef struct {
    v86_ctx_t           ctx;
    v86_call_type_t     type;
    uint8_t             int_num;
    struct list_head    list;
} v86_call_t;

typedef struct {
    struct list_head    head;
} v86_call_list_t;

void    v86_init();
void    v86_call(v86_call_t* call);
void*   v86_farptr2linear(uint32_t fp);