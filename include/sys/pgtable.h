#ifndef _PGTABLE_H
#define _PGTABLE_H

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

#endif
