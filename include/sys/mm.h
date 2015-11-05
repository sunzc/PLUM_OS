#ifndef _MM_H
#define _MM_H

#include <sys/defs.h>

/**
 * bitmap for physical memory bookkeeping, note: bitmap occupies only 1 page.
 * for 128MB memory. The calculations is done as follows:
 *   	128MB / 4KB = 32K pages, keep in mind that one frames maps to 1 bit in the bitmap
 * so we need 32k bits to store the bit map.
 * 	32K / (4K * 8 bits/Byte) = 1 page
 */
#define PG_BITS 	(12)
#define PG_SIZE		(1 << PG_BITS)	/* 4K */
#define PHYMEM_SIZE	(1<<27) /* 128M, acutal size is less than it, it's aligned size */
#define BITMAP_SIZE	((PHYMEM_SIZE >> PG_BITS) >> 6) /* one bit per page, one uint64_t contains 64 bits*/

/* vitual address to physical address */
#define VA2PA(addr)	(addr - 0xffffffff00000000)
#define PA2VA(addr)	(addr + 0xffffffff00000000)

/* align addr in page size */
#define PG_ALIGN(addr) (((uint64_t)addr + PG_SIZE - 1) & (~(PG_SIZE - 1)))

/* bookeeping available physical memory block */
#define MAX_PHY_BLOCK	5
typedef struct phymem_block {
	uint64_t base;
	uint64_t length;
}phymem_block;


/* TODO undefined yet, just to show some basic concept */
struct vma {
	void *start_addr;
	void *end_addr;
	struct vma *next;
	int flag;
};

/* TODO vma_list is used to link vma struct in a circle */
struct vma_list {
	struct vma_list *prev;
	struct vma_list *next;
	struct vma *vmap;
}

/**
 * page struct to keep track of physical page, including page reference count,
 * and pointer to vma list, pointer to next free page struct.
 */
struct page {
	struct vma_list *vma_listp;
	struct page *next;
	int ref;
};

void *memset(void *s, int c, size_t n);
void mm_init();

#endif
