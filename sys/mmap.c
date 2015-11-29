#include <sys/sbunix.h>
#include <sys/proc.h>
#include <sys/pgfault.h>

#define DEBUG_PGFAULT

extern task_struct *current;

void page_fault_handler(uint64_t addr, uint64_t ecode, uint64_t eip) {
	vma_struct *vma;

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

		} else if (ecode & PFEC_WR) {	/* vialation in page level, write non-writable page */
			printf("pagefault: vialation found, write non-writable page!\n");
			goto bad_area;
		}

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
