#ifndef _PGTABLE_H
#define _PGTABLE_H

typedef uint64_t pgd_t;
typedef uint64_t pud_t;
typedef uint64_t pmd_t;
typedef uint64_t pte_t;

#define PGD_P		(1)
#define PGD_RW		(1<<1)
#define PGD_US		(1<<2)
#define PGD_NX		(1<<63)

#define PUD_P		(1)
#define PUD_RW		(1<<1)
#define PUD_US		(1<<2)
#define PUD_NX		(1<<63)

#define PMD_P		(1)
#define PMD_RW		(1<<1)
#define PMD_US		(1<<2)
#define PMD_NX		(1<<63)

#define PTE_P		(1)
#define PTE_RW		(1<<1)
#define PTE_US		(1<<2)
#define PTE_NX		(1<<63)


#define PMD_BITS		21

#define PMD_T_NUM		64	/* the entries number of pmd */
#define PTE_T_NUM		512	/* PTE entires number per page */

/* get pxd_offset from pfn */
#define get_pgd_off(pfn)	((int)((pfn<<28)>>(27+28)))
#define get_pud_off(pfn)	((int)((pfn<<(9+28))>>(27+28)))
#define get_pmd_off(pfn)	((int)((pfn<<(18+28))>>(27+28)))
#define get_pte_off(pfn)	((int)((pfn<<(27+28))>>(27+28)))

/* operate on pgtable entries */
#define put_pgd_entry(pgd, pgd_off, pud, prot)	\
	do { *((pgd_t *)pgd + pgd_off) = (uint64_t)VA2PA(pud) | prot; } while(0)
#define put_pud_entry(pud, pud_off, pmd, prot)	\
	do { *((pud_t *)pud + pud_off) = (uint64_t)VA2PA(pmd) | prot; } while(0)
#define put_pmd_entry(pmd, pmd_off, pte, prot)	\
	do { *((pmd_t *)pmd + pmd_off) = (uint64_t)VA2PA(pte) | prot; } while(0)
#define put_pte_entry(pte, pte_off, pg_frame, prot)	\
	do { *((pte_t *)pte + pte_off) = (uint64_t)VA2PA(pg_frame) | prot; } while(0)

#define get_pgd_entry(pgd, pgd_off) (*((pgd_t *)pgd + pgd_off))
#define get_pud_entry(pud, pud_off) (*((pud_t *)pud + pud_off))
#define get_pmd_entry(pmd, pmd_off) (*((pmd_t *)pmd + pmd_off))
#define get_pte_entry(pte, pte_off) (*((pte_t *)pte + pte_off))

#endif
