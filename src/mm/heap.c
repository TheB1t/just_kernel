#include <mm/heap.h>
#include <mm/vmm.h>
#include <mm/pmm.h>

typedef uint32_t size_t;

#define SIZE_T_SIZE         (sizeof(size_t))
#define TWO_SIZE_T_SIZES    (SIZE_T_SIZE<<1)
#define SIZE_T_ZERO         ((size_t)0)
#define SIZE_T_ONE          ((size_t)1)
#define SIZE_T_TWO          ((size_t)2)
#define SIZE_T_FOUR         ((size_t)4)

#define PINUSE_BIT          (SIZE_T_ONE)
#define CINUSE_BIT          (SIZE_T_TWO)
#define RESERV_BIT          (SIZE_T_FOUR)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)
#define FLAG_BITS           (INUSE_BITS|RESERV_BIT)

#define HEAPCHUNK_SIZE      (sizeof(HeapChunk_t))
#define KMALLOC_ALIGNMENT   ((size_t)(2 * sizeof(void*)))

#define KMALLOC_ALIGN_MASK  (KMALLOC_ALIGNMENT - SIZE_T_ONE)
#define MIN_CHUNK_SIZE      ((HEAPCHUNK_SIZE + KMALLOC_ALIGN_MASK) & ~KMALLOC_ALIGN_MASK)
#define CHUNK_OVERHEAD      (SIZE_T_SIZE)

#define MIN_REQUEST         (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - SIZE_T_ONE)

#define chunksize(ptr)      ((ptr)->head & ~(FLAG_BITS))

#define get_head(heap)      (&(heap)->head)

#define chunk2mem(ptr)      ((void*)((char*)(ptr) + TWO_SIZE_T_SIZES))
#define mem2chunk(ptr)      ((HeapChunk_t*)((char*)(ptr) - TWO_SIZE_T_SIZES))

#define pinuse(p)           ((p)->head & PINUSE_BIT)
#define cinuse(p)           ((p)->head & CINUSE_BIT)
#define ok_address(p, h)    (((size_t)(p) >= (h)->startAddr) && ((size_t)(p) < (h)->endAddr))
#define ok_inuse(p)         (((p)->head & INUSE_BITS) != PINUSE_BIT)

#define pad_request(req)    (((req) + CHUNK_OVERHEAD + KMALLOC_ALIGN_MASK) & ~KMALLOC_ALIGN_MASK)

#define request2size(req)   (((req) < MIN_REQUEST) ? MIN_CHUNK_SIZE : pad_request(req))

#define chunk_plus_offset(p, s)     ((HeapChunk_t*)((char*)(p) + (s)))
#define chunk_minus_offset(p, s)    ((HeapChunk_t*)((char*)(p) - (s)))

#define set_pinuse(p)       ((p)->head |= PINUSE_BIT)
#define clear_pinuse(p)     ((p)->head &= ~PINUSE_BIT)

#define set_inuse(p,s) \
    ((p)->head = (((p)->head & PINUSE_BIT)|CINUSE_BIT|(s)), \
    (chunk_plus_offset(p, s))->head |= PINUSE_BIT)

#define set_size_and_pinuse(p, s) \
    ((p)->head = ((s)|PINUSE_BIT))

#define set_foot(p, s)      (chunk_plus_offset(p, s)->prevFoot = (s))

uint32_t heap_malloc(Heap_t* heap, uint32_t size, uint8_t align) {
    if (heap) {
        uint32_t addr;
        if (align)
            addr = (uint32_t)palignedAlloc(size, heap);
        else
            addr = (uint32_t)alloc(size, heap);

        return addr;
    }

    return 0;
}

void heap_free(Heap_t* heap, void* addr) {
	if (heap)
		free(addr, heap);
}

static HeapChunk_t* findSmallestChunk(size_t size, Heap_t* heap) {
	HeapChunk_t* tmp = 0;
	struct list_head* iter;
	list_for_each(iter, get_head(heap)) {
		tmp = list_entry(iter, HeapChunk_t, list);
		if (chunksize(tmp) >= size)
			break;
	}

	if (iter == get_head(heap))
		return 0;

	return tmp;
}

static void insertChunk(HeapChunk_t* chunk, Heap_t* heap) {
	if (list_empty(get_head(heap)))
		list_add_tail(&(chunk->list), get_head(heap));
	else {
		struct list_head* iter;
		size_t csize = chunksize(chunk);
		list_for_each(iter, get_head(heap)) {
			HeapChunk_t* tmp = list_entry(iter, HeapChunk_t, list);
			if (chunksize(tmp) >= csize)
				break;
		}

		list_add_tail(&(chunk->list), iter);
	}
}

static void removeChunk(HeapChunk_t* chunk) {
	__list_del_entry(&(chunk->list));
}

Heap_t* createHeap(uint32_t placementAddr, uint32_t cr3, uint32_t start, uint32_t size, uint32_t max, uint16_t perms) {
	Heap_t* heap = (Heap_t*)placementAddr;

	uint32_t end = start + size;
	assert(start % PAGE_SIZE == 0);
	assert(end % PAGE_SIZE == 0);

	heap->startAddr		= start;
	heap->endAddr		= end;
	heap->maxAddr		= max;
	heap->perms			= perms;
	heap->cr3			= cr3;
	INIT_LIST_HEAD(get_head(heap));

	void* phys = pmm_alloc(size);
	vmm_map_pages(phys, (void*)start, (void*)heap->cr3, size / PAGE_SIZE, heap->perms | VMM_PRESENT);

	HeapChunk_t* hole = (HeapChunk_t*)start;
	hole->prevFoot = 0;
	set_size_and_pinuse(hole, size);
	insertChunk(hole, heap);

	return heap;
}

static void expand(size_t newSize, Heap_t* heap) {
	assert(newSize > heap->endAddr - heap->startAddr);

	if (newSize % PAGE_SIZE) {
		newSize &= -PAGE_SIZE;
		newSize += PAGE_SIZE;
	}

	assert(heap->startAddr + newSize <= heap->maxAddr);

	uint32_t oldEnd = heap->endAddr;
    heap->endAddr = heap->startAddr + newSize;

    size_t size = heap->endAddr - oldEnd;
    void* phys = pmm_alloc(size);

    vmm_map(phys, (void*)oldEnd, size / PAGE_SIZE, heap->perms | VMM_PRESENT);
}

static size_t contract(size_t newSize, Heap_t* heap) {
	assert(newSize < heap->endAddr - heap->startAddr);

	if (newSize % PAGE_SIZE) {
		newSize &= -PAGE_SIZE;
		newSize += PAGE_SIZE;
	}

	if (newSize < HEAP_MIN_SIZE)
		newSize = HEAP_MIN_SIZE;

	uint32_t oldEnd = heap->endAddr;
    heap->endAddr = heap->startAddr + newSize;

    size_t size = oldEnd - heap->endAddr;
    if (!is_mapped((void*)heap->endAddr, (void*)heap->cr3))
        ser_printf("[heap] trying to free unmapped memory! addr: 0x%08x\n", heap->endAddr);

    vmm_unmap_pages((void*)heap->endAddr, (void*)heap->cr3, size / PAGE_SIZE);

    void* phys = virt_to_phys((void*)heap->endAddr, (void*)heap->cr3);
    if ((uint32_t)phys != 0xFFFFFFFF) {
        pmm_unalloc(phys, size);
	}

	return newSize;
}

void* alloc(size_t size, Heap_t* heap) {
	size_t nb = request2size(size);

	HeapChunk_t* hole = findSmallestChunk(nb, heap);
	if (hole == 0) {
		size_t oldSize = heap->endAddr - heap->startAddr;
		size_t oldEndAddr = heap->endAddr;
		expand(oldSize + nb, heap);

		struct list_head* tmp = 0;
		if (!list_empty(get_head(heap))) {
			struct list_head* iter;
			list_for_each(iter, get_head(heap)) {
				if (iter > tmp)
					tmp = iter;
			}
		}

		HeapChunk_t* topChunk = 0;
		size_t csize = 0;

		if (tmp) {
			topChunk = list_entry(tmp, HeapChunk_t, list);
			if (!pinuse(topChunk) && cinuse(topChunk))
				goto _Lassert;

			size_t oldCsize = chunksize(topChunk);
			if ((size_t)chunk_plus_offset(topChunk, oldCsize) != oldEndAddr)
				topChunk = 0;
			else {
				removeChunk(topChunk);
				csize = heap->endAddr - (size_t)topChunk;
			}
		}

		if (topChunk == 0) {
			topChunk = (HeapChunk_t*)oldEndAddr;
			csize = heap->endAddr - oldEndAddr;
		}

		set_size_and_pinuse(topChunk, csize);
		insertChunk(topChunk, heap);

		return alloc(size, heap);
	}

	assert(chunksize(hole) >= nb);

	removeChunk(hole);

	if (chunksize(hole) >= nb + MIN_CHUNK_SIZE) {
		size_t nsize = chunksize(hole) - nb;

		HeapChunk_t* new = chunk_plus_offset(hole, nb);
		new->head = nsize;

		insertChunk(new, heap);
	} else
		nb = chunksize(hole);

	set_inuse(hole, nb);

	return chunk2mem(hole);

_Lassert:
	panic("(alloc) Memory Corrupt");
	__builtin_unreachable();
}

void free(void* ptr, Heap_t* heap) {
	if (ptr == 0)
		return;

	if (!ok_address(ptr, heap))
		goto _Lassert;

	HeapChunk_t* block = mem2chunk(ptr);

	if (!ok_inuse(block))
		goto _Lassert;

	size_t size = chunksize(block);

	if (!pinuse(block)) {
		size_t prevsize = block->prevFoot;
		HeapChunk_t* prev = chunk_minus_offset(block, prevsize);

		if (!ok_address(prev, heap))
			goto _Lassert;

		size += prevsize;
		block = prev;

		removeChunk(block);
	}

	HeapChunk_t* next = chunk_plus_offset(block, size);
	if (!ok_address(next, heap)) {
		if (!pinuse(next))
			goto _Lassert2;

		if (cinuse(next))
			clear_pinuse(next);
		else {
			size_t nextsize = chunksize(next);

			if ((size_t)chunk_plus_offset(next, nextsize) > heap->endAddr)
				goto _Lassert2;

			removeChunk(next);

			size += nextsize;
		}
	}

	if ((size_t)chunk_plus_offset(block, size) == heap->endAddr) {
		size_t oldSize = heap->endAddr - heap->startAddr;
		size_t newSize = (size_t)block - heap->startAddr;

		if (newSize % PAGE_SIZE)
			newSize += MIN_CHUNK_SIZE;

		newSize = contract(newSize, heap);

		if (size > oldSize - newSize)
			size -= oldSize - newSize;
		else {
			size = 0;
			block = 0;
		}
	} else
		set_foot(block, size);

	if (block && size) {
		set_size_and_pinuse(block, size);
		insertChunk(block, heap);
	}

	return;

_Lassert2:
	insertChunk(block, heap);
_Lassert:
	panic("(free) Memory Corrupt");
}

void* palignedAlloc(size_t size, Heap_t* heap) {
	size_t nb = request2size(size);
	size_t req = nb + PAGE_SIZE + MIN_CHUNK_SIZE - CHUNK_OVERHEAD;

	void* mem = alloc(req, heap);
	if (mem) {
		HeapChunk_t* chunk = mem2chunk(mem);
		if ((size_t)mem % PAGE_SIZE) {
			char* br = (char*)mem2chunk(((size_t)mem & -PAGE_SIZE) + PAGE_SIZE);
			char* pos = ((size_t)(br - (char*)(chunk)) >= MIN_CHUNK_SIZE) ? br : br + PAGE_SIZE;

			HeapChunk_t* newChunk = (HeapChunk_t*)pos;
			size_t leadsize = pos - (char*)(chunk);
			size_t newSize = chunksize(chunk) - leadsize;

			set_inuse(newChunk, newSize);

			set_size_and_pinuse(chunk, leadsize);
			newChunk->prevFoot = leadsize;

			insertChunk(chunk, heap);

			chunk = newChunk;
		}

		size_t csize = chunksize(chunk);
		if (csize > nb + MIN_CHUNK_SIZE) {
			size_t remainderSize = csize - nb;
			HeapChunk_t* remainder = chunk_plus_offset(chunk, nb);
			set_inuse(chunk, nb);
			set_inuse(remainder, remainderSize);
			free(chunk2mem(remainder), heap);
		}

		mem = chunk2mem(chunk);
		assert(chunksize(chunk) >= nb);
		assert(((size_t)mem % PAGE_SIZE) == 0);
		assert(cinuse(chunk));
	}

	return mem;
}