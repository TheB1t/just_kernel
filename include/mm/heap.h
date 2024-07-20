#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>

#define HEAP_START          0x90000000
#define HEAP_MIN_SIZE       0x100000

typedef struct {
	uint32_t			startAddr;
	uint32_t			endAddr;
	uint32_t			maxAddr;
	uint32_t			cr3;
	uint16_t			perms;
	struct	list_head	head;
} Heap_t;

typedef	struct {
	uint32_t			prevFoot;
	uint32_t			head;
	struct	list_head	list;
} HeapChunk_t;

Heap_t*     createHeap(uint32_t placementAddr, uint32_t cr3, uint32_t start, uint32_t size, uint32_t max, uint16_t perms);
void*	    alloc(uint32_t size, Heap_t* heap);
void*	    palignedAlloc(uint32_t size, Heap_t* heap);
void        free(void* p, Heap_t* heap);

uint32_t	heap_malloc(Heap_t* heap, uint32_t size, uint8_t align);
void		heap_free(Heap_t* heap, void* addr);