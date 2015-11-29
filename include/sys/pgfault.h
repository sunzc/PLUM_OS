#ifndef _PGFAULT_H
#define _PGFAULT_H
#include <sys/defs.h>

/* define Page-Fault Error Code */
#define PFEC_P		(1)	/* 0 : non-present page, 1 : page-level protection violation */
#define PFEC_WR		(1<<1)	/* 0 : read, 1 : write*/
#define PFEC_US		(1<<2)	/* 0 : CPL(addr) < 3, 1 : CPL(addr) == 3 */
#define PFEC_RSVD	(1<<3)	/* reserved bit set to 1 in paging-structure entry */
#define PFEC_ID		(1<<4)	/* 1 : fault by instruction fetch */

void page_fault_handler(uint64_t pgfault_addr, uint64_t error_code, uint64_t eip);

#endif
