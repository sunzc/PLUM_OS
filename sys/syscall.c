#include <sys/sbunix.h>
#include <sys/syscall.h>
#include <sys/proc.h>
#include <sys/string.h>

//#define DEBUG_SYSCALL

extern task_struct *current;
extern task_struct *task_headp;
extern int pid_count;

const uint32_t CPUID_FLAG_MSR = 1 << 5;

typedef struct sc_frame {
	uint64_t rbp;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
}sc_frame;

extern void __syscall_handler(void);
extern void ret_from_fork(void);

uint64_t syscall_handler(int syscall_num, sc_frame *sf);
uint64_t sys_null(int num);
uint64_t read(sc_frame *);
uint64_t write(sc_frame *);
uint64_t brk(sc_frame *);
uint64_t fork (sc_frame *sf);
uint64_t execve(sc_frame *sf);

void copy_mm(mm_struct *new, mm_struct *old);
void copy_vma_deep(vma_struct *new_vmap, pgd_t *new_pgd, vma_struct *old_vmap, pgd_t *old_pgd);

void cpuid(int code, uint32_t *a, uint32_t *d) {
	__asm volatile("cpuid":"=a"(*a), "=d"(*d):"a"(code):"ecx","ebx");
}

uint32_t cpu_has_msr() {
	uint32_t a, d;	//eax, edx
	cpuid(1, &a, &d);
	return d & CPUID_FLAG_MSR;
}

void cpu_get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi) {
	__asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void cpu_set_msr(uint32_t msr, uint32_t lo, uint32_t hi) {
	__asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void init_syscall() {
	uint32_t lo, hi, msr;
	
	/* first test if the processor support syscall/sysret or not */
	msr = cpu_has_msr();
	if (!msr)
		panic("cpu doesn't support MSR!");

	/* enable syscall by setting EFER :SysCallEnable bit */
	cpu_get_msr(IA32_EFER, &lo, &hi);

	lo = lo | 1;
	cpu_set_msr(IA32_EFER, lo, hi);

	/* set MSR:IA32_LSTAR to be the address of syscall_handler*/
	lo = (uint32_t) (((uint64_t)__syscall_handler) & ((((uint64_t) 1) << 32) - 1));
	hi = (uint32_t) ((((uint64_t)__syscall_handler)& ~((((uint64_t) 1) << 32) - 1)) >> 32);

#ifdef DEBUG_SYSCALL
	printf("[init_syscall] set MSR:IA32_LSTAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif

	cpu_set_msr(IA32_LSTAR, lo, hi);


#ifdef DEBUG_SYSCALL
	lo = 0;
	hi = 0;
	cpu_get_msr(IA32_LSTAR, &lo, &hi);
	printf("[init_syscall] get MSR:IA32_LSTAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif

	/**
	 * set MSR:IA32_STAR, so that:
	 * for syscall:
	 * 	CS.Selector = gdt[1], equals IA32_STAR[47:32]
	 * 	SS.Selector = gdt[2], equals IA32_STAR[47:32] + 8
	 * for sysret:
	 * 	CS.Selector = gdt[8], equals IA32_STAR[63:48] + 16
	 * 	SS.Selector = gdt[7], equals IA32_STAR[63:48] + 8
	 * which means 
	 *  IA32_STAR[47:32] = 0x8(KERNEL MODE);
	 *  IA32_STAR[63:48] = 0x30 + 0x3(USER MODE);
	 */

	lo = 0;	// IA32_STAR[0:31] : reserved
	hi = (0x33 << 16) | 0x8;	// IA32_STAR[32:63]:SYSRET CS&SS| SYSCALL CS&SS

#ifdef DEBUG_SYSCALL
	printf("[init_syscall] set MSR:IA32_STAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif

	cpu_set_msr(IA32_STAR, lo, hi);

#ifdef DEBUG_SYSCALL
	lo = 0;
	hi = 0;
	cpu_get_msr(IA32_STAR, &lo, &hi);
	printf("[init_syscall] get MSR:IA32_STAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif
}

uint64_t syscall_handler(int syscall_num, sc_frame *sf) {
	uint64_t res;
	switch(syscall_num) {
		case SYS_read: 
			res = read(sf);
			break;
		case SYS_write: 
			res = write(sf);
			break;
		case SYS_brk:
			res = brk(sf);
			break;
		case SYS_fork:
			res = fork(sf);
			break;
		case SYS_execve:
			res = execve(sf);
			break;
		default:
			res = sys_null(syscall_num);
			break;
	}

	return res;
}


uint64_t read(sc_frame *sf) {
	uint64_t res = 0;
	printf("syscall read!\n");

	return res;
}

uint64_t write(sc_frame *sf) {
	uint64_t res = 0;
	char *buf = (char *)(sf->rsi);

#ifdef DEBUG_SYSCALL	
	uint32_t fd = (uint32_t)(sf->rdi);
	uint64_t size = sf->rdx;
	printf("[debug]syscall write! fd:%d, buf:0x%lx, size:0x%lx\n", fd, buf, size);
#endif
	printf("%c",*buf);
	return res;
}

uint64_t sys_null(int num) {
	int res = 0;
	printf("syscall num = %d, not supportted yet!\n", num);

	while(1) {
		printf("I come into unsupported syscall, so yield. call schedule\n");
		schedule();
	}
	return res;
}

/**
 * change user process program break to addr.
 */
#define HEAP_LIMIT	0x7ffffff00000
uint64_t brk(sc_frame *sf) {
	uint64_t new_break, cur_break;
	vma_struct *vma;

	new_break = sf->rdi;
	cur_break = current->mm->user_heap;

#ifdef DEBUG_SYSCALL
	printf("[brk] new_break : 0x%lx, cur_break: 0x%lx\n", new_break, cur_break);
#endif

	/**
	 * TODO: heap memory and data are writable area, but they should use differnt vma
	 * because ,heap memory have no backup file, but data segment have back file and
	 * initial value.
	 * current impl is buggy, we should explicitly setup vma for heap and should not
	 * rely on search_vma return NULL to judge whether heap vma is setup or not.
	 */
	if (new_break == 0)
		return cur_break;
	else if (new_break > cur_break && new_break < HEAP_LIMIT) {	/* add or extend vma */
		vma = search_vma(cur_break - 1, current->mm);
		if (vma == NULL || (vma->prot & PTE_RW) == 0 || vma->vm_file != NULL) {
			/**
			 * no vma alloced yet, alloc one.
			 * it's tricky here, when vma->vm_file!=NULL, it's data seg
			 */
			if ((vma = get_zero_page()) == NULL)
				panic("[brk]ERROR: alloc vma failed!");

			vma->vm_start = cur_break;
			vma->vm_end = new_break;
			vma->vm_file = NULL;
			vma->prot = PTE_RW;
			vma->vm_align = PG_SIZE;

			insert_vma(vma, current->mm);

		} else if (vma->prot & PTE_RW) {	/* ok, it's the writable data vma we want to extend */
			if (vma->vm_end < new_break)
				vma->vm_end = new_break;
		}

		return new_break;

	} else if (new_break > HEAP_LIMIT)
		panic("ERROR: reach heap limit!");
	else
		panic("ERROR: do not support shrink heap memory!");

	return -1;
}

void copy_sc_frame(sc_frame *new, sc_frame *old) {
	new->rbp = old->rbp;
	new->r15 = old->r15;
	new->r14 = old->r14;
	new->r13 = old->r13;
	new->r11 = old->r11;
	new->r10 = old->r10;
	new->r9  = old->r9;
	new->r8  = old->r8;
	new->rdi = old->rdi;
	new->rsi = old->rsi;
	new->rdx = old->rdx;
	new->rcx = old->rcx;
	new->rbx = old->rbx;
}

void copy_file_array(task_struct *new, task_struct *old) {
	int i;

	for (i = START_FD; i < MAX_FILE_NUM; i++) {
		if (old->file_array[i].free == 0)
			continue;

		strncpy(new->file_array[i].mode, old->file_array[i].mode, 16);
		new->file_array[i].free = old->file_array[i].free;
		new->file_array[i].start_addr = old->file_array[i].start_addr;
		new->file_array[i].size = old->file_array[i].size;
		new->file_array[i].pos = old->file_array[i].pos;
	}
}

uint64_t fork (sc_frame *sf) {
	task_struct *tsp;
	sc_frame *nsf;

	if ((tsp = (task_struct *)get_zero_page()) == NULL)
		return -1;

	/* initialize the task struct */
	tsp->pid = pid_count++;
	tsp->ppid = current->pid;
	strncpy(tsp->name, "first child process through fork", 40);
	tsp->func = NULL;

	if ((tsp->kernel_stack = get_zero_page()) == NULL)
		return -1;

	/**
	 * prepare context info
	 * we should first prepare for context switch stack frame, and then
	 * prepare for ret_from_fork, that is ret from syscall
	 * stack look should looks like this:
	 *     ___________
	 *    |           |
	 *    |  sc_frame |
	 *    |___________|
	 *    | ret_f_fork|
	 *    |___________|
	 *    |           |
	 *    | contxt_inf|
	 *    |___________|
	 *
	 */
	nsf = (sc_frame *)(tsp->kernel_stack + STACK_SIZE - sizeof(sc_frame));
	copy_sc_frame(nsf, sf);
	*(uint64_t *)(tsp->kernel_stack + STACK_SIZE - sizeof(sc_frame) - 8) = (uint64_t)ret_from_fork;
	((context_info *)(tsp->kernel_stack + STACK_SIZE - sizeof(sc_frame) - 8 - sizeof(context_info)))->rcx \
		 = (uint64_t)&(tsp->last);
	((context_info *)(tsp->kernel_stack + STACK_SIZE - sizeof(sc_frame) - 8 - sizeof(context_info)))->rax \
		= (uint64_t)tsp;
	((context_info *)(tsp->kernel_stack + STACK_SIZE - sizeof(sc_frame) - 8 - sizeof(context_info)))->r12 \
		= (uint64_t)(tsp);
	tsp->kernel_stack = tsp->kernel_stack + STACK_SIZE - sizeof(sc_frame) - 8 - sizeof(context_info);

	/* copy father opened file to child */
	copy_file_array(tsp, current);

	/* copy mm_struct */
	if ((tsp->mm = (mm_struct *)get_zero_page()) == NULL)
		panic("[fork]: ERROR alloc mm_struct failed!");

	/* copy the mm_struct */
	tsp->mm->mmap = NULL;
	tsp->mm->entry = current->mm->entry;
	tsp->mm->pgd = alloc_pgd();
	tsp->mm->mm_count = current->mm->mm_count;
	tsp->mm->highest_vm_end = current->mm->mm_count;
	tsp->mm->code_start = current->mm->code_start;
	tsp->mm->code_end = current->mm->code_end;
	tsp->mm->data_start = current->mm->data_start;
	tsp->mm->data_end = current->mm->data_end;
	/* toppest user address */
	tsp->user_stack = current->user_stack;

#ifdef DEBUG_SYSCALL
	printf("[fork] user_stack:0x%lx\n", current->user_stack);
	dump_stack((void *)(current->user_stack), 32);
#endif

	/* will be adjusted later, should be the lowest userable address above .text .data segments */
	tsp->mm->user_heap = current->mm->user_heap;

	/* copy vma and mark cow */
	copy_mm(tsp->mm, current->mm);

	/* flush tlb here, or we can't catch exception for COW */

	/* insert new task struct at the head of the list */
	tsp->next = task_headp;
	task_headp->prev = tsp;
	tsp->prev = NULL;
	task_headp = tsp;

	return tsp->pid;
}


void copy_vma(vma_struct *new, vma_struct *old) {
	new->vm_start = old->vm_start;
	new->vm_end =old->vm_end;
	new->prot = old->prot;
	new->vm_pgoff = old->vm_pgoff;
	new->vm_filesz =old->vm_filesz;
	new->vm_file = old->vm_file;
}

/**
 * first copy vma struct, and see
 * 	1. if vma is readonly, just copy all page tables.
 *	2. if vma is writable, mark mapped page COW and readonly.
 */
void copy_mm(mm_struct *new, mm_struct *old) {
	vma_struct *new_vmap, *old_vmap;

	old_vmap = old->mmap;
	while(old_vmap != NULL) {
		if ((new_vmap = (vma_struct *)get_zero_page()) == NULL)
			panic("[copy_vma]ERROR: alloc new->mmap failed!");

		copy_vma(new_vmap, old_vmap);
		copy_vma_deep(new_vmap, new->pgd, old_vmap, old->pgd);

		insert_vma(new_vmap, new);

		old_vmap = old_vmap->next;
	}
}

void copy_vma_deep(vma_struct *new_vmap, pgd_t *new_pgd, vma_struct *old_vmap, pgd_t *old_pgd) {
	uint64_t i;
	int pgd_off, pud_off, pmd_off, pte_off;
	void *pud, *pmd, *pte, *pg_frame;
	void *npud, *npmd, *npte;

	uint64_t start_addr = old_vmap->vm_start;
	uint64_t end_addr = old_vmap->vm_end;

	uint64_t pfn_start = start_addr >> PG_BITS;
	uint64_t pfn_end = (end_addr - 1) >> PG_BITS;

	for(i = pfn_start; i <= pfn_end; i++) {
		/* get pud */
		pgd_off = get_pgd_off(i);
		if (get_pgd_entry(old_pgd, pgd_off) == 0)
			continue;
		else {
			pud = (void *)((uint64_t)PA2VA(get_pgd_entry(old_pgd, pgd_off)) & ~(PG_SIZE - 1));

			/* alloc page directory for new pgd */
			if (get_pgd_entry(new_pgd, pgd_off) == 0) {
				if ((npud = get_zero_page()) == NULL)
					panic("[copy_vma_deep]ERROR: alloc npud error!");

				put_pgd_entry(new_pgd, pgd_off, npud, PGD_P | PGD_RW | PGD_US);
			} else
				npud = (void *)((uint64_t)PA2VA(get_pgd_entry(new_pgd, pgd_off)) & ~(PG_SIZE - 1));

		}

		/* get pmd */
		pud_off = get_pud_off(i);
		if (get_pud_entry(pud, pud_off) == 0)
			continue;
		else {
			pmd = (void *)((uint64_t)PA2VA(get_pud_entry(pud, pud_off)) & ~(PG_SIZE - 1));

			/* alloc page directory for new pgd */
			if (get_pud_entry(npud, pud_off) == 0) {
				if ((npmd = get_zero_page()) == NULL)
					panic("[map_vma]ERROR: alloc npmd error!");

				put_pud_entry(npud, pud_off, npmd, PUD_P | PUD_RW | PUD_US);
			} else
				npmd = (void *)((uint64_t)PA2VA(get_pud_entry(npud, pud_off)) & ~(PG_SIZE - 1));
		}

		/* get pte */
		pmd_off = get_pmd_off(i);
		if (get_pmd_entry(pmd, pmd_off) == 0)
			continue;
		else {
			pte = (void *)((uint64_t)PA2VA(get_pmd_entry(pmd, pmd_off)) & ~(PG_SIZE - 1));

			/* alloc page directory for new pmd */
			if (get_pmd_entry(npmd, pmd_off) == 0) {
				if ((npte = get_zero_page()) == NULL)
					panic("[map_vma_deep]ERROR: alloc npte error!");

				put_pmd_entry(npmd, pmd_off, npte, PMD_P | PMD_RW | PMD_US);
			} else
				npte = (void *)((uint64_t)PA2VA(get_pmd_entry(npmd, pmd_off)) & ~(PG_SIZE - 1));
		}

		/* get page frame */
		pte_off = get_pte_off(i);
		if (get_pte_entry(pte, pte_off) == 0)
			continue;
		else {
			pg_frame = (void *)((uint64_t)PA2VA(get_pte_entry(pte, pte_off)) & ~(PG_SIZE - 1));

			if (old_vmap->prot & PTE_RW) {	/* writable: mark both old and new pte entry COW and RDONLY */
				put_pte_entry(pte, pte_off, pg_frame, PTE_P | PTE_COW | PTE_US);
				put_pte_entry(npte, pte_off, pg_frame, PTE_P | PTE_COW | PTE_US);
			} else	/* readonly: copy old pte entry to new pte entry */
				put_pte_entry(npte, pte_off, pg_frame, PTE_P | PTE_US);

			/* add page reference */
			add_page_ref(((uint64_t)VA2PA(pg_frame))>>PG_BITS);
		}
	}
}

/**
 * the SYS_execve has three parameters:
 * 	char *filename,
 *	char *argv[]
 * 	char *envp[]
 *
 * Note:
 * 	execve never return, if return, something go wrong!
 */
uint64_t execve(sc_frame *sf) {
	char *filename = (char *)(sf->rdi);
	char **argv = (char **)(sf->rsi);
	char **envp = (char **)(sf->rdx);

	exec(filename, argv, envp);

	return 0;

}
