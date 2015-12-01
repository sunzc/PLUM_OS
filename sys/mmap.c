#include <sys/sbunix.h>
#include <sys/proc.h>
#include <sys/pgfault.h>

#define DEBUG_PGFAULT

extern task_struct *current;

void page_fault_handler(uint64_t addr, uint64_t ecode, uint64_t eip) {
	vma_struct *vma;
	pte_t pte;
	void *pg_frame;

#ifdef DEBUG_PGFAULT
	printf("[page_fault_handler] addr: 0x%lx, ecode: 0x%lx, eip: 0x%lx\n", addr, ecode, eip);
#endif

	if (ecode & PFEC_US) {	/* page fault in user mode, worth attention */
		if (!(ecode & PFEC_P)) {	/* PFEC_P:0, page not present */

			vma = search_vma(addr, current->mm);
			if (vma != NULL) {	/* this page has a corresponding vma, we need to map it */
				map_a_page(vma, addr);
				return;
			} else {
				printf("pagefault: no vma matched, page not accessable!\n");
				goto bad_area;
			}

		} else if (ecode & PFEC_RW) {	/* vialation in page level, write non-writable page */
			printf("pagefault: vialation found, write non-writable page!\n");
			/* handle copy-on-write here */

			/* assert addr has been mapped */
			printf("pte_addr:0x%lx\n",get_pte_entry_addr_trick(addr));
			assert(get_pte_entry_trick(addr) != 0);
			pte = get_pte_entry_trick(addr);
			printf("[pgfault handler]pte:0x%lx\n", pte);
			if (pte & PTE_COW) {
				if (get_page_ref(pte>>PG_BITS) > 1) {	/* it's shared, ref > 1 */
					/**
					 * handle cow here:
					 * 	1. alloc a new page
					 *	2. copy data from old page
					 *	3. mark this page writable
					 *	4. clear PTE_COW
					 *	5. fill new entry
					 *	6. deduct old page ref
					 */
					if ((pg_frame = get_zero_page()) == NULL)
						panic("[page_fault_handler]ERROR: alloc pg_frame failed!");

					copy_page(pg_frame, (void *)PA2VA(pte & (~(PG_SIZE - 1))));
					put_pte_entry_trick(addr, pg_frame, PTE_P | PTE_RW | PTE_US);
					deduct_page_ref(pte>>PG_BITS);
				} else {
					/* if page ref == 1, means we have handled cow before. just clear COW */
					put_pte_entry_trick(addr, PA2VA(pte & (~(PG_SIZE - 1))), PTE_P | PTE_RW | PTE_US);
				}

				/* flush tlb here */
				flush_tlb();
				return;
			} else /* real vialation happen, can't handle it !*/
				goto bad_area;
		} else
			printf("[page_fault_handler] user level pagefault, present, no vialation. It's strange, we should not reach here!\n");
	} else {
		/**
		 * page fault happened in kernel mode is usually a real fault, except for some special
		 * case, like copy data from kernel to user. we leave it for furture. By now, we just
		 * panic.
		 */
		printf("ERROR: page fault in kernel mode!");
		goto bad_area;
	}

bad_area:
	printf("pagefault info addr: 0x%lx, ecode: 0x%lx, eip: 0x%lx\n", addr, ecode, eip);
	panic("ERROR: unable to handle this pagefault!");
	return;	
}
