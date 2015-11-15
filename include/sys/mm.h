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
#define VA2PA(addr)	(addr - 0xffffffff80000000)
#define PA2VA(addr)	(addr + 0xffffffff80000000)

/* align addr in page size */
#define PG_ALIGN(addr) (((uint64_t)addr + PG_SIZE - 1) & (~(PG_SIZE - 1)))

/* bookeeping available physical memory block */
#define MAX_PHY_BLOCK	5
typedef struct phymem_block {
	uint64_t base;
	uint64_t length;
}phymem_block;

/**
 * page struct to keep page ref info. bitmap is used to manage free page. 
 */
#define PAGE_ARRAY_SIZE (PHYMEM_SIZE>>PG_BITS)
struct page {
	int ref;
};

void *memset(void *s, int c, size_t n);
void mm_init();
uint64_t get_free_page();
void *get_zero_page();
void free_page(uint64_t pfn);

#endif
