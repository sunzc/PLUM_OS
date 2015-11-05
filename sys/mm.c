#include <sys/sbunix.h>
#include <sys/mm.h>

/* available physical memory block */
phymem_block pmb_array[MAX_PHY_BLOCK];
uint32_t pmb_count = 0;

/* kernel image base and end, should always reside in memory */
extern void *_physbase;
extern void *_physfree;

/* bootloader image base and end, can be freed after set up kernel page tables */
extern void *_loaderbase;
extern void *_loaderend;

void *pgd_start;
uint64_t *bitmap;

void *memset(void *s, int c, size_t n) {
	int i;
	char *p = (char *)s;

	for (i = 0; i < n; i++)
		*(p + i) = (char)c;

	return s;
}

/**
 * The physical memory layout looks like below
 *
 *                    __________   0X8000000
 *                   |          |
 *                   |  padding |
 *                   |__________|  0X7FFE000
 *                   |          |
 *                   |          |
 *                   |          |
 *                   |  free    |
 *                   |  memory  |
 *                   |          |
 *                   |          |
 *                   |__________|  0X02XXXXX physfree
 *                   |          |
 *                   |  kernel  |
 *                   |  image   |
 *                   |          |
 *                   |__________|  0X0200000 physbase
 *                   |          |
 *                   |  free    |
 *                   |  memory  |
 *                   |__________|  0X0100000
 *                   |          |
 *                   .          .
 *                   .          .  memory hole
 *                   .          .
 *                   |__________|  0x009FC00
 *                   |          |  
 *                   |  booter  |
 *                   |__________|  0x0000000
 *
 *
 */
static void setup_bitmap() {
	uint64_t i, pfn_start, pfn_end;

	/* bitmap starts at _physfree, take up 32K memory */
	bitmap = (uint64_t *)_physfree;

	/* initialize array to zero */
	for (i = 0; i < BITMAP_SIZE; i++)
		bitmap[i] = 0;

	/* mark bootloader image busy, discard half usable page*/
	pfn_start = (uint64_t)_loaderbase >> PG_BITS;
	pfn_end = ((uint64_t)_loaderend >> PG_BITS);
	printf("boot: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark memory hole busy forever */
	pfn_start = (uint64_t)(pmb_array[0].base + pmb_array[0].length) >> PG_BITS;
	pfn_end = ((uint64_t)pmb_array[1].base >> PG_BITS);
	printf("hole: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark kernel image busy forever */
	pfn_start = (uint64_t)_physbase >> PG_BITS;
	pfn_end = ((uint64_t)_physfree >> PG_BITS);
	printf("kernel: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark physical memory padding area busy forever */
	pfn_start = (uint64_t)(pmb_array[pmb_count - 1].base + pmb_array[pmb_count - 1].length) >> PG_BITS;
	pfn_end = ((uint64_t)PHYMEM_SIZE >> PG_BITS);
	printf("padding: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark bitmap itself busy forever */
	pfn_start = (uint64_t)_physfree >> PG_BITS;
	pfn_end = pfn_start + ((BITMAP_SIZE * sizeof(uint64_t)) >> PG_BITS);
	printf("bitmap: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark initial pgtables area busy forever */
	pgd_start = (void *)PG_ALIGN((uint64_t)_physfree + BITMAP_SIZE * sizeof(uint64_t));
	pfn_start = (uint64_t)pgd_start >> PG_BITS;
	pfn_end = ((uint64_t)pgd_start >> PG_BITS) + (1 + 1 + 1 + 64);
	printf("pgtable: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

}

void free_initmem() {
	uint64_t i, pfn_start, pfn_end;

	/* free bootloader mem after set up kernel own page table */
	pfn_start = (uint64_t)_loaderbase >> PG_BITS;
	pfn_end = ((uint64_t)_loaderend >> PG_BITS) - 1;
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] &= ~(1 << (i%64));
}

/**
 * Now, it's time to set up the kernel's own page tables, after that, we can free the bootloader's
 * page table, and use our own one.
 *
 * The initial page table should map all the physical memory to kernel space, that is 
 *  map physical address:
 *	0x 0000 0000 0000 -- 0x 0000 07ff ffff
 *  to virtual address:
 * 	0x ffff 0000 0000 -- 0x ffff 07ff ffff
 * and to make it smoothly, we should also
 * map physical address:
 * 	0x 0000 0000 0000 -- 0x 0000 07ff ffff
 * to virtual addresss:
 * 	0x 0000 0000 0000 -- 0x 0000 07ff ffff
 *
 * Keep in mind, the x86-64 MMU support 48bit virtual address, 4 level page tables
 * 
 *	 ________________________________________
 * 	|   9   |   9  |  9   |  9   |    12     |
 *       ----------------------------------------
 *      47    3938   3029   2019   1211          0
 */

typedef uint64_t pgd_t;
typedef uint64_t pud_t;
typedef uint64_t pmd_t;
typedef uint64_t pte_t;

#define PGD_P		(1)
#define PGD_RW		(1<<1)
#define PGD_US		(1<<2)

#define PUD_P		(1)
#define PUD_RW		(1<<1)
#define PUD_US		(1<<2)

#define PMD_P		(1)
#define PMD_RW		(1<<1)
#define PMD_US		(1<<2)

#define PTE_P		(1)
#define PTE_RW		(1<<1)
#define PTE_US		(1<<2)

#define PMD_BITS		21

#define PMD_T_NUM		64	/* the entries number of pmd */
#define PTE_T_NUM		512	/* PTE entires number per page */

static void setup_initial_pgtables() {
	int i, j;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte, *ptep;

	/* bitmap starts at _physfree, take up 4K memory, pgd_start follows bitmap area */
	pgd_start = (void *)PG_ALIGN((uint64_t)_physfree + BITMAP_SIZE * sizeof(uint64_t));
	printf("pgd_start:0x%lx\n", pgd_start);

	pgd = (pgd_t *)pgd_start;
	pud = (pud_t *)((uint64_t)pgd + PG_SIZE);
	pmd = (pmd_t *)((uint64_t)pud + PG_SIZE);
	pte = (pte_t *)((uint64_t)pmd + PG_SIZE);

	/* for pgd, only the 1st entry and the last entry is filled, they point to the pud, which is pgd + 1 page */
	*pgd = (uint64_t)pud | PGD_P | PGD_RW | PGD_US;
	*(pgd + 0x1ff) = (uint64_t)pud | PGD_P | PGD_RW | PGD_US;
	
	/* for pud, the 1st entry and the 0x1fc entry is filled */
	*pud = (uint64_t)pmd | PUD_P | PUD_RW | PUD_US; 
	*(pud + 0x1fe) = (uint64_t)pmd | PUD_P | PUD_RW | PUD_US;

	/* for pmd, the 1st to 63rd entries are all filled */
	for (i = 0; i < PMD_T_NUM; i++)
		*(pmd + i) = ((uint64_t)pte + (i << PG_BITS))| PMD_P | PMD_RW | PMD_US;

	/* for pte, we need to fill each page with concret physical pages */
	for (i = 0; i < PTE_T_NUM; i++) {
		ptep = (pte_t *)((uint64_t)pte + (i << PG_BITS));
		for (j = 0; j < 512; j++)
			*(ptep + j) = ((i << PMD_BITS) + (j << PG_BITS)) | PTE_P | PTE_RW | PTE_US;
	}
}

void mm_init() {
	int i;

	printf("mm_init()\n");

	for(i = 0; i < pmb_count; i++) {
		printf("base: 0x%lx\n", pmb_array[i].base);
		printf("length: 0x%lx\n", pmb_array[i].length);
	}

	setup_bitmap();
	setup_initial_pgtables();

	__asm__ __volatile__(
	"movq %0, %%cr3\n\t"
	:
	: "b" (pgd_start)
	:);

}
