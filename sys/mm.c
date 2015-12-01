#include <sys/sbunix.h>
#include <sys/proc.h>

extern task_struct *current;

/* available physical memory block */
phymem_block pmb_array[MAX_PHY_BLOCK];
uint32_t pmb_count = 0;

/* kernel image base and end, should always reside in memory */
extern void *_physbase;
extern void *_physfree;

/* bootloader image base and end, can be freed after set up kernel page tables */
extern void *_loaderbase;
extern void *_loaderend;

/* global page table start address */
void *pgd_start;
void *pgtable_end;

/* bitmap start address */
uint64_t *bitmap;
void *bitmap_end;

/* page struct array start address */
struct page *page_array;
void *page_array_end;

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

	/* bitmap starts at _physfree, take up 4k memory, one page */
	bitmap = (uint64_t *)(_physfree);

	/* initialize array to zero */
	for (i = 0; i < BITMAP_SIZE; i++)
		bitmap[i] = 0;

	/* mark bootloader image busy, discard half usable page*/
	pfn_start = (uint64_t)_loaderbase >> PG_BITS;
	pfn_end = ((uint64_t)_loaderend >> PG_BITS);
	printf("boot: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("boot map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark memory hole busy forever */
	pfn_start = (uint64_t)(pmb_array[0].base + pmb_array[0].length) >> PG_BITS;
	pfn_end = ((uint64_t)pmb_array[1].base >> PG_BITS);
	printf("hole: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("hole map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark kernel image busy forever */
	pfn_start = (uint64_t)_physbase >> PG_BITS;
	pfn_end = ((uint64_t)_physfree >> PG_BITS);
	printf("kernel: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("kernel map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark physical memory padding area busy forever */
	pfn_start = (uint64_t)(pmb_array[pmb_count - 1].base + pmb_array[pmb_count - 1].length) >> PG_BITS;
	pfn_end = ((uint64_t)PHYMEM_SIZE >> PG_BITS);
	printf("padding: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("padding map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark bitmap itself busy forever */
	pfn_start = (uint64_t)_physfree >> PG_BITS;
	pfn_end = pfn_start + ((BITMAP_SIZE * sizeof(uint64_t)) >> PG_BITS);
	bitmap_end = (void *) (pfn_end << PG_BITS);
	printf("bitmap: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("bitmap map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark initial pgtables area busy forever */
	pgd_start = (void *)PG_ALIGN((uint64_t)_physfree + BITMAP_SIZE * sizeof(uint64_t));
	pfn_start = (uint64_t)pgd_start >> PG_BITS;
	pfn_end = ((uint64_t)pgd_start >> PG_BITS) + (1 + 1 + 1 + 64);
	pgtable_end = (void *)(pfn_end << PG_BITS);
	printf("pgtable: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("pgtable map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);

	/* mark page array area busy forever */
	page_array = (struct page *) pgtable_end;
	pfn_start = (uint64_t)page_array >> PG_BITS;
	pfn_end = pfn_start + (PG_ALIGN(PAGE_ARRAY_SIZE * sizeof(struct page)) >> PG_BITS);
	page_array_end = (void *)(pfn_end << PG_BITS);
	printf("page array: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("page array map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++)
		bitmap[i/64] |= 1 << (i%64);
}

void free_initmem() {
	uint64_t i, pfn_start, pfn_end;

	/* free bootloader mem after set up kernel own page table */
	pfn_start = (uint64_t)_loaderbase >> PG_BITS;
	pfn_end = ((uint64_t)_loaderend >> PG_BITS);
	printf("free initmem: pfn_start: %lu pfn_end : %lu\n", pfn_start, pfn_end);
	printf("free initmem map: pfn_start: 0x%lx pfn_end : 0x%lx\n", (uint64_t)bitmap + pfn_start/8, (uint64_t)bitmap + pfn_end/8);
	for (i = pfn_start; i < pfn_end; i++) {
		bitmap[i/64] &= ~(1 << (i%64));
		(page_array + i)->ref = 0;
	}
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
	//*pgd = (uint64_t)pud | PGD_P | PGD_RW | PGD_US;
	*(pgd + 0x1ff) = (uint64_t)pud | PGD_P | PGD_RW;

	/* self-refering trick here, for the 0x1ff has already been used by kernel, we use 0x1fe */
	*(pgd + 0x1fe) = (uint64_t)pgd | PGD_P | PGD_RW;
	
	/* for pud, the 1st entry and the 0x1fc entry is filled */
	//*pud = (uint64_t)pmd | PUD_P | PUD_RW | PUD_US; 
	*(pud + 0x1fe) = (uint64_t)pmd | PUD_P | PUD_RW;

	/* for pmd, the 1st to 63rd entries are all filled */
	for (i = 0; i < PMD_T_NUM; i++)
		*(pmd + i) = ((uint64_t)pte + (i << PG_BITS))| PMD_P | PMD_RW;

	/* for pte, we need to fill each page with concret physical pages */
	for (i = 0; i < PTE_T_NUM; i++) {
		ptep = (pte_t *)((uint64_t)pte + (i << PG_BITS));
		for (j = 0; j < 512; j++)
			*(ptep + j) = ((i << PMD_BITS) + (j << PG_BITS)) | PTE_P | PTE_RW;
	}
}

static void setup_page_array() {
	int i, j;

	for (i = 0; i < BITMAP_SIZE; i++) {
		for (j = 0; j < 64; j++) {
			if (bitmap[i] & (1<<j))
				(page_array + (i * 64 + j))->ref= 1;
			else
				(page_array + (i * 64 + j))->ref= 0;
		}
	}
}

/* here we assume pfn is the physical address page frame */
void add_page_ref(uint64_t pfn) {
	assert((uint64_t)(page_array + pfn) < (uint64_t)page_array_end);
	(page_array + pfn)->ref += 1;
}

/* here we assume pfn is the physical address page frame */
int get_page_ref(uint64_t pfn) {
	assert((uint64_t)(page_array + pfn) < (uint64_t)page_array_end);
	return (page_array + pfn)->ref;
}

/* here we assume pfn is the physical address page frame */
void deduct_page_ref(uint64_t pfn) {
	assert((uint64_t)(page_array + pfn) < (uint64_t)page_array_end);
	(page_array + pfn)->ref -= 1;
}

uint64_t get_free_page() {
	uint64_t bf_pfn_start = 0;
	uint64_t bf_pfn_end = 159; 
	uint64_t bkf_pfn_start = 256;
	uint64_t bkf_pfn_end = 512;
	uint64_t normal_pfn_start = 671;
	uint64_t normal_pfn_end = 32766;

	uint64_t pfn = 0;

	/* search boot area for free pages */
	for (pfn = bf_pfn_start; pfn < bf_pfn_end; pfn++) {
		if ((page_array + pfn)->ref == 0) {
			/* mark page_array 1 */
			(page_array + pfn)->ref = 1;

			/* mark bitmap busy */
			bitmap[pfn/64] |= 1 << (pfn%64);

			return pfn;
		}
	}

	/* search before kernel image area for free pages */
	for (pfn = bkf_pfn_start; pfn < bkf_pfn_end; pfn++) {
		if ((page_array + pfn)->ref == 0) {
			/* mark page_array 1 */
			(page_array + pfn)->ref = 1;

			/* mark bitmap busy */
			bitmap[pfn/64] |= 1 << (pfn%64);

			return pfn;
		}
	}

	/* search boot area for free pages */
	for (pfn = normal_pfn_start; pfn < normal_pfn_end; pfn++) {
		if ((page_array + pfn)->ref == 0) {
			/* mark page_array 1 */
			(page_array + pfn)->ref = 1;

			/* mark bitmap busy */
			bitmap[pfn/64] |= 1 << (pfn%64);

			return pfn;
		}
	}

	panic("[get_free_page] ERROR : out of memory!");
	return -1;
}

void *get_zero_page() {
	uint64_t pfn;
	void *page;

	if ((pfn = get_free_page()) == -1)
		return NULL;
	

	page = (void *)PA2VA(pfn<<PG_BITS);

	memset(page, 0, PG_SIZE);

	return page;
}

void free_page(uint64_t pfn) {
	/* page ref count - 1 */
	(page_array + pfn)->ref -= 1;

	/* if page ref equals 0, free it in bitmap */
	if ((page_array + pfn)->ref == 0)
		bitmap[pfn/64] &= ~(1 << (pfn%64));
}

/* we use too much physical address for convenience, now we need to give up it! */
static void move_vaddr() {
	pgd_start = (void *)PA2VA((char *)pgd_start);
	pgtable_end = (void *)PA2VA((char *)pgtable_end);

	/* bitmap start address */
	bitmap = (uint64_t *)PA2VA((char *)bitmap);
	bitmap_end = (void *)PA2VA((char *)bitmap_end);

	/* page struct array start address */
	page_array = (struct page *)PA2VA((char *)page_array);
	page_array_end = (void *)PA2VA((char *)page_array_end);
	
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
	setup_page_array();
	free_initmem();
	move_vaddr();

	/* switch to kernel own page tables */
	__asm__ __volatile__(
	"movq %0, %%cr3\n\t"
	:
	: "b" (VA2PA(pgd_start))
	:);

}

void switch_mm(task_struct *prev, task_struct *next) {
	/* switch to a user process's address space */
	if (next->mm == NULL)
		return;

	if (prev != next) {
		__asm__ __volatile__(
		"movq %0, %%cr3\n\t"
		:
		: "b" (VA2PA(next->mm->pgd))
		:);
	}
}

void flush_tlb() {
	__asm__ __volatile__(
	"movq %0, %%cr3\n\t"
	:
	: "b" (VA2PA(current->mm->pgd))
	:);
}

/* copy page byte by byte */
void copy_page(void *dest_page, void *src_page) {
	int i;

	for (i = 0; i < PG_SIZE; i++)
		*(char *)(dest_page + i) = *(char *)(src_page + i);
}

/**
 * alloc_pgd return a pgd pointer, which is a copy of the kernel page global directory.
 * Note:
 *	pgd stores the virtual address, only change to physical address when use.
 */
pgd_t *alloc_pgd() {
	void *p;

	if ((p = get_zero_page()) == NULL)
		return NULL;

	copy_page(p, pgd_start);

	/*self referencing trick, 0x1fe*/
	*((uint64_t *)p + 0x1fe) = ((uint64_t)VA2PA(p)) | PGD_P | PGD_RW;

	return (pgd_t *)(p); 
}

/* now we set up page tables for user process, the original mapping for low address should be cancelled first */
void map_vma(struct vm_area_struct *vma, pgd_t *pgd) {
	uint64_t i;
	int pgd_off, pud_off, pmd_off, pte_off;
	void *pud, *pmd, *pte, *pg_frame;
	char *usrp, *fp;

	uint64_t start_addr = vma->vm_start;
	uint64_t end_addr = vma->vm_end;
	uint64_t prot = vma->prot;

	uint64_t pfn_start = start_addr >> PG_BITS;
	uint64_t pfn_end = (end_addr - 1) >> PG_BITS;

	for(i = pfn_start; i <= pfn_end; i++) {

		/* alloc pud */
		pgd_off = get_pgd_off(i);
		if (get_pgd_entry(pgd, pgd_off) == 0) {
			if ((pud = get_zero_page()) == NULL)
				panic("[map_vma]ERROR: alloc pud error!");

			put_pgd_entry(pgd, pgd_off, pud, PGD_P | PGD_RW | PGD_US); 
		} else
			pud = (void *)((uint64_t)PA2VA(get_pgd_entry(pgd, pgd_off)) & ~(PG_SIZE - 1));

		/* alloc pmd */
		pud_off = get_pud_off(i);
		if (get_pud_entry(pud, pud_off) == 0) {
			if ((pmd = get_zero_page()) == NULL)
				panic("[map_vma]ERROR: alloc pmd error!");
			put_pud_entry(pud, pud_off, pmd, PUD_P | PUD_RW | PUD_US); 
		} else
			pmd = (void *)((uint64_t)PA2VA(get_pud_entry(pud, pud_off)) & ~(PG_SIZE - 1));

		/* alloc pte */
		pmd_off = get_pmd_off(i);		
		if (get_pmd_entry(pmd, pmd_off) == 0) {
			if ((pte = get_zero_page()) == NULL)
				panic("[map_vma]ERROR: alloc pte error!");
			put_pmd_entry(pmd, pmd_off, pte, PMD_P | PMD_RW | PMD_US); 
		} else
			pte = (void *)((uint64_t)PA2VA(get_pmd_entry(pmd, pmd_off)) & ~(PG_SIZE - 1));

		/* alloc page */
		pte_off = get_pte_off(i);
		if (get_pte_entry(pte, pte_off) == 0) {
			if ((pg_frame = get_zero_page()) == NULL)
				panic("[map_vma]ERROR: alloc page frame error!");
			/* in case this segment is readonly, so set it rw first and change PROT after load data */
			put_pte_entry(pte, pte_off, pg_frame, PTE_P | PTE_RW | PTE_US);
		}
	}

	/**
	 * load file to user memory
	 * suppose here the pgd is the current pgd, after setup page tables we can access those user
	 *  memory afterwards.
	 */

	/* if this vma is for stack or heap, that means it doesn't have back file, we are done! */
	if (vma->vm_file == NULL)
		return;

	/* locate file pointer to vm_pgoff of vm_file */
	fp = (char *) (vma->vm_file->start_addr + vma->vm_pgoff);

	/* locate user memory pointer to vm_start */
	usrp = (char *) (vma->vm_start);

	/* copy data from file to user memory */
	for (i = 0; i < vma->vm_filesz; i++)
		*(usrp + i) = *(fp + i);

	/* change page entry PROT permentally */
	for (i = pfn_start; i <= pfn_end; i++) {
		pgd_off = get_pgd_off(i);
		pud = (void *)((uint64_t)PA2VA(get_pgd_entry(pgd, pgd_off)) & ~(PG_SIZE - 1));
		pud_off = get_pud_off(i);
		pmd = (void *)((uint64_t)PA2VA(get_pud_entry(pud, pud_off)) & ~(PG_SIZE - 1));
		pmd_off = get_pmd_off(i);		
		pte = (void *)((uint64_t)PA2VA(get_pmd_entry(pmd, pmd_off)) & ~(PG_SIZE - 1));
		pte_off = get_pte_off(i);
		pg_frame = (void *)((uint64_t)PA2VA(get_pte_entry(pte, pte_off)) & ~(PG_SIZE - 1));
		put_pte_entry(pte, pte_off, pg_frame, PTE_P | prot | PTE_US);
	}
}

vma_struct *search_vma (uint64_t addr, mm_struct *mm) {
	vma_struct *vma;

	vma = mm->mmap;
	while (vma != NULL) {
		if (vma->vm_start <= addr && vma->vm_end > addr)
			break;
		else
			vma = vma->next;
	}

	return vma;
}

void insert_vma (vma_struct *vma, mm_struct *mm) {
	vma_struct *vmap;

	if (mm->mmap == NULL)
		mm->mmap = vma;
	else {
		vmap = mm->mmap;
		while(vmap->next != NULL)
			vmap  = vmap->next;
		vmap->next = vma;
	}
}

void map_a_page (vma_struct *vma, uint64_t addr) {
	pgd_t *pgd;
	uint64_t pfn, vm_inner_off, prot;

	int i;
	int pgd_off, pud_off, pmd_off, pte_off;
	void *pud, *pmd, *pte, *pg_frame;
	char *usrp, *fp;

	pgd = current->mm->pgd;
	prot = vma->prot;
	pfn = addr >> PG_BITS;

	/* alloc pud */
	pgd_off = get_pgd_off(pfn);
	if (get_pgd_entry(pgd, pgd_off) == 0) {
		if ((pud = get_zero_page()) == NULL)
			panic("[map_vma]ERROR: alloc pud error!");

		put_pgd_entry(pgd, pgd_off, pud, PGD_P | PGD_RW | PGD_US);
	} else
		pud = (void *)((uint64_t)PA2VA(get_pgd_entry(pgd, pgd_off)) & ~(PG_SIZE - 1));

	/* alloc pmd */
	pud_off = get_pud_off(pfn);
	if (get_pud_entry(pud, pud_off) == 0) {
		if ((pmd = get_zero_page()) == NULL)
			panic("[map_vma]ERROR: alloc pmd error!");
		put_pud_entry(pud, pud_off, pmd, PUD_P | PUD_RW | PUD_US);
	} else
		pmd = (void *)((uint64_t)PA2VA(get_pud_entry(pud, pud_off)) & ~(PG_SIZE - 1));

	/* alloc pte */
	pmd_off = get_pmd_off(pfn);
	if (get_pmd_entry(pmd, pmd_off) == 0) {
		if ((pte = get_zero_page()) == NULL)
			panic("[map_vma]ERROR: alloc pte error!");
		put_pmd_entry(pmd, pmd_off, pte, PMD_P | PMD_RW | PMD_US);
	} else
		pte = (void *)((uint64_t)PA2VA(get_pmd_entry(pmd, pmd_off)) & ~(PG_SIZE - 1));

	/* alloc page */
	pte_off = get_pte_off(pfn);
	if (get_pte_entry(pte, pte_off) == 0) {
		if ((pg_frame = get_zero_page()) == NULL)
			panic("[map_vma]ERROR: alloc page frame error!");
		/* in case this segment is readonly, so set it rw first and change PROT after load data */
		put_pte_entry(pte, pte_off, pg_frame, PTE_P | PTE_RW | PTE_US);
	}

	/**
	 * load file to user memory
	 * suppose here the pgd is the current pgd, after setup page tables we can access those user
	 * memory afterwards.
	 */

	/* if this vma is for stack or heap, that means it doesn't have back file, we are done! */
	if (vma->vm_file == NULL)
		return;

	/* locate file pointer to vm_pgoff + vm_off of vm_file */
	vm_inner_off = (pfn << PG_BITS) - vma->vm_start;
	fp = (char *) (vma->vm_file->start_addr + vma->vm_pgoff + vm_inner_off);

	/* locate user memory pointer to vm_start */
	usrp = (char *) (pfn << PG_BITS);

	/* copy data from file to user memory */
	for (i = 0; i < PG_SIZE; i++)
		*(usrp + i) = *(fp + i);

	/* change page entry PROT permentally */
	pgd_off = get_pgd_off(pfn);
	pud = (void *)((uint64_t)PA2VA(get_pgd_entry(pgd, pgd_off)) & ~(PG_SIZE - 1));
	pud_off = get_pud_off(pfn);
	pmd = (void *)((uint64_t)PA2VA(get_pud_entry(pud, pud_off)) & ~(PG_SIZE - 1));
	pmd_off = get_pmd_off(pfn);
	pte = (void *)((uint64_t)PA2VA(get_pmd_entry(pmd, pmd_off)) & ~(PG_SIZE - 1));
	pte_off = get_pte_off(pfn);
	pg_frame = (void *)((uint64_t)PA2VA(get_pte_entry(pte, pte_off)) & ~(PG_SIZE - 1));
	put_pte_entry(pte, pte_off, pg_frame, PTE_P | prot | PTE_US);

	return;
}
