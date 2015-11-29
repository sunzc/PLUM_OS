#ifndef _MM_H
#define _MM_H

#include <sys/defs.h>
#include <sys/pgtable.h>
#include <sys/tarfs.h>

/**
 * bitmap for physical memory bookkeeping, note: bitmap occupies only 1 page.
 * for 128MB memory. The calculations is done as follows:
 *   	128MB / 4KB = 32K pages, keep in mind that one frames maps to 1 bit in the bitmap
 * so we need 32k bits to store the bit map.
 * 	32K / (4K * 8 bits/Byte) = 1 page
 */
#define PG_BITS 	(12)
#define PG_SIZE		(1 << PG_BITS)	/* 4K */
#define STACK_SIZE	PG_SIZE
#define PHYMEM_SIZE	(1<<27) /* 128M, acutal size is less than it, it's aligned size */
#define BITMAP_SIZE	((PHYMEM_SIZE >> PG_BITS) >> 6) /* one bit per page, one uint64_t contains 64 bits*/

/* vitual address to physical address */
#define VA2PA(addr)	(((void *)(addr)) - 0xffffffff80000000)
#define PA2VA(addr)	(((void *)(addr)) + 0xffffffff80000000)

/* align addr in page size */
#define PG_ALIGN(addr) (((uint64_t)(addr) + PG_SIZE - 1) & (~(PG_SIZE - 1)))

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

typedef struct __attribute__((__packed__)) vm_area_struct {
	struct vm_area_struct *next;

	/* start -- end, prot*/
	uint64_t vm_start;
	uint64_t vm_end; /* first address after vma, why?TODO */
	uint64_t prot;

	/* backing file info */
	uint64_t vm_pgoff;
	uint64_t vm_filesz;
	uint64_t vm_align; 
	file *vm_file;
}vma_struct;

/**
 * mm_struct contains a pointer to process vma list.
 */
typedef struct __attribute__((__packed__)) mm_struct {
	struct vm_area_struct *mmap;

	/* elf e_entry, which records the start address of execution */
	uint64_t entry;

	/* pgd points to the page global directory */
	pgd_t *pgd;

	/* we need to know the follow for user stack, heap, brk management */
	uint64_t code_start, code_end;
	uint64_t data_start, data_end;
	uint64_t user_stack, user_heap;

	/* statistic info */
	int mm_count;
	uint64_t highest_vm_end;
}mm_struct;

void *memset(void *s, int c, size_t n);
void mm_init();
uint64_t get_free_page();
void *get_zero_page();
void free_page(uint64_t pfn);
void map_vma(struct vm_area_struct *, pgd_t *);
struct vm_area_struct *search_vma(uint64_t, mm_struct *);
void map_a_page(struct vm_area_struct *, uint64_t);
pgd_t *alloc_pgd();
void copy_page(void *dest_page, void *src_page);

#endif
