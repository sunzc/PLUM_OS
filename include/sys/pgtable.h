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
#define PTE_COW		(1<<9)	/* 9-11 bit are not used by MMU */


#define PMD_BITS		21

#define PMD_T_NUM		64	/* the entries number of pmd */
#define PTE_T_NUM		512	/* PTE entires number per page */

/* self referencing index */
#define SELF_REF_INDEX		0x1fe

#define PGD_MASK	0xffffff7fbfdfe000
#define PUD_MASK	0xffffff7fbfc00000
#define PMD_MASK	0xffffff7f80000000
#define PTE_MASK	0xffffff0000000000

/* get page directory entry use self-referencing trick */
#define get_pgd_entry_trick(addr)	(*(pgd_t *)((((addr>>36) & (((uint64_t)1<<12) - 1)) | PGD_MASK) & (~0x7)))
#define get_pud_entry_trick(addr)	(*(pud_t *)((((addr>>27) & (((uint64_t)1<<(12+9)) - 1)) | PUD_MASK) & (~0x7)))
#define get_pmd_entry_trick(addr)	(*(pmd_t *)((((addr>>18) & (((uint64_t)1<<(12+9+9)) - 1)) | PMD_MASK) & (~0x7)))
#define get_pte_entry_trick(addr)	(*(pte_t *)((((addr>>9)  & (((uint64_t)1<<(12+9+9+9)) - 1)) | PTE_MASK) & (~0x7)))
#define get_pgd_entry_addr_trick(addr)	((((addr>>36) & (((uint64_t)1<<(12)) - 1)) | PGD_MASK) & (~0x7))
#define get_pud_entry_addr_trick(addr)	((((addr>>27) & (((uint64_t)1<<(12+9)) - 1)) | PUD_MASK) & (~0x7))
#define get_pmd_entry_addr_trick(addr)	((((addr>>18) & (((uint64_t)1<<(12+9+9)) - 1)) | PMD_MASK) & (~0x7))
#define get_pte_entry_addr_trick(addr)	((((addr>>9)  & (((uint64_t)1<<(12+9+9+9)) - 1)) | PTE_MASK) & (~0x7))

#define put_pgd_entry_trick(addr, pud, prot) \
	do { *((pgd_t *)(get_pgd_entry_addr_trick(addr))) = (uint64_t)VA2PA(pud) | prot;} while(0)
#define put_pud_entry_trick(addr, pmd, prot) \
	do { *((pud_t *)(get_pud_entry_addr_trick(addr))) = (uint64_t)VA2PA(pmd) | prot;} while(0)
#define put_pmd_entry_trick(addr, pte, prot) \
	do { *((pmd_t *)(get_pmd_entry_addr_trick(addr))) = (uint64_t)VA2PA(pte) | prot;} while(0)
#define put_pte_entry_trick(addr, pg_frame, prot) \
	do { *((pte_t *)(get_pte_entry_addr_trick(addr))) = (uint64_t)VA2PA(pg_frame) | prot;} while(0)

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
