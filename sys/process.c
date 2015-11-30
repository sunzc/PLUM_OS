#include <sys/sbunix.h>
#include <sys/gdt.h>
#include <sys/proc.h>
#include <sys/tarfs.h>
#include <sys/elf.h>
#include <sys/string.h>

/* task struct are linked in a double-linked list, task_headp points to the head */
task_struct *task_headp = NULL;
task_struct *current = NULL;
extern void *pgd_start;
extern void __ret_to_user(void *user_stack, void *user_entry);

/* current highest pid number */
int pid_count = 0;

static task_struct * select_task_struct(void);
static task_struct * switch_to(task_struct *, task_struct *, task_struct **);
static uint64_t map_pflags_to_vmprot(uint32_t p_flags);

void init_proc() {
	task_headp =(task_struct *) get_zero_page();

	task_headp->prev = NULL;
	task_headp->next = NULL;
	task_headp->last = NULL;

	/* pid keep growing */
	task_headp->pid = pid_count++;
	strncpy(task_headp->name, "init", 4);

	/* mm_struct describe vm structure of user process, kernel thread 's mm is NULL */
	task_headp->mm = NULL;

	/* this very first process's kernel_stack will be changed in switch_to */
	task_headp->func = NULL;
	task_headp->kernel_stack = NULL; 

	/* current points the running process's task_struct */
	current = task_headp;
}

int kernel_thread(void (*f)(void), char *thread_name) {
	task_struct *tsp;

	if ((tsp = (task_struct *)get_zero_page()) == NULL)
		return -1;

	/* initialize the task struct */
	tsp->pid = pid_count++;
	if (thread_name != NULL)
		strncpy(tsp->name, thread_name, strlen(thread_name));
	else
		strncpy(tsp->name, "anonimous process", 20);
	tsp->func = f;
	if ((tsp->kernel_stack = get_zero_page()) == NULL)
		return -1;

	/* prepare context info */
	*(uint64_t *)(tsp->kernel_stack + STACK_SIZE - 8) = (uint64_t)f;
	((context_info *)(tsp->kernel_stack + STACK_SIZE - 8 - sizeof(context_info)))->rcx = (uint64_t)&(tsp->last);
	((context_info *)(tsp->kernel_stack + STACK_SIZE - 8 - sizeof(context_info)))->rax = (uint64_t)tsp;
	tsp->kernel_stack = tsp->kernel_stack + STACK_SIZE - 8 - sizeof(context_info);

	/* insert new task struct at the head of the list */
	tsp->next = task_headp;
	task_headp->prev = tsp;
	tsp->prev = NULL;
	task_headp = tsp;

	return 0;
}


void schedule() {
	task_struct *me, *next;

	me = current;
	next = select_task_struct();
	assert(next != me);

	current = next;

	//__switch_to(me, next, &me->last);
	switch_to(me, next, &me->last);
 	//__asm volatile("sti"::);

}

static task_struct * switch_to(task_struct *me, task_struct *next, task_struct **last) {
	__asm__(
		"movq %0, %%rax;"
		"movq %1, %%rbx;"
		"movq %2, %%rcx;"
		"callq __switch_to;"
	:
	:"m"(me),"m"(next),"m"(last)
	:"%rax","%rbx","%rcx"
	);		

	return me;
}

static task_struct * select_task_struct(void) {
	task_struct *p;

	/* choose next task in a round-robin way */
	p = current;
	if(p->next != NULL)
		return p->next;
	else
		return task_headp;
}

/**
 * exec the first user process/
 * 	1. load elf_binary into memory(not really, demand-paging) and build corresponding vm_struct.
 *	2  set_up page tables for the user process.
 *	3. ret_to_user to start execute.
 */

void exec(char *filename) {
	int i, fd;
	void *fp;

	/* future kernel stack */
	void *future_kstack, *sp;
	int argc;
	char *argv, *envp;
	

	/* vma struct */
	struct vm_area_struct *vma;

	/* Elf related part, mostly the program header */
	void *elf_header, *ph;
	uint64_t phoff;
	int phnum, phentsize;

	/* fields that inside a program header */
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;

	fd = tarfs_open(filename, "r");

	/* we don't want to change file->pos, give size 0, we will get the file pointer at start pos 0 */
	fp = tarfs_read(fd, 0);

	/* prepare for loading */
	assert(current->mm == NULL);

	if ((current->mm = (struct mm_struct *)get_zero_page()) == NULL)
		panic("[exec]: ERROR alloc mm_struct failed!");

	/* initialize the mm_struct */
	current->mm->mmap = NULL;
	current->mm->entry = ((Elf64_Ehdr *)fp)->e_entry;
	current->mm->pgd = alloc_pgd();
	current->mm->mm_count = 0;
	current->mm->highest_vm_end = 0;
	current->mm->code_start = 0;
	current->mm->code_end = 0;
	current->mm->data_start = 0;
	current->mm->data_end = 0;
	/* toppest user address */
	current->mm->user_stack = 0x7ffffffffffc;
	/* will be adjusted later, should be the lowest userable address above .text .data segments */
	current->mm->user_heap = 0;

	/* Even we do not have a context switch, we have to do switch_mm to go to new address space */
	switch_mm(NULL, current);

	/* generally, we read one segment from elf_binary, and setup the related vma for it! */

	/* parse elf header */
	elf_header = fp;
	phoff = ((Elf64_Ehdr *)elf_header)->e_phoff;
	phentsize = ((Elf64_Ehdr *)elf_header)->e_phentsize;
	phnum = ((Elf64_Ehdr *)elf_header)->e_phnum;

	/* locate program header */
	ph = fp + phoff;

	/* handle each program header */
	for (i = 0; i < phnum; i++) {
		/* read program header */
		p_type	= ((Elf64_Phdr *)ph)->p_type;
		p_flags	= ((Elf64_Phdr *)ph)->p_flags;
		p_offset= ((Elf64_Phdr *)ph)->p_offset;
		p_vaddr = ((Elf64_Phdr *)ph)->p_vaddr;
		p_filesz= ((Elf64_Phdr *)ph)->p_filesz;
		p_memsz	= ((Elf64_Phdr *)ph)->p_memsz;
		p_align	= ((Elf64_Phdr *)ph)->p_align;

		/* skip none loadable segment, do not consider PT_INTERP, for we don't support dynamic loader */
		if (p_type != PT_LOAD)
			continue;

		/**
		 * We assume the 1st program header is the only one text segment, 
		 * the 2rd program header is the only one data segment(if exists).
		 * Our assumation based on the following:
		 *	1. it's a elf file
		 *	2. it's a statically linked executable.
		 */

		/* we have already checked p_type, only PT_LOAD can go through here */
		if (i == 0 && p_flags == (PF_R + PF_X)) {
			/* 1st seg should be text seg, p_flags equals (PF_R + PF_X = 5) */
			current->mm->code_start = p_vaddr;
			current->mm->code_end = p_vaddr + p_memsz;
		}

		if (i == 1 && p_flags == (PF_R + PF_W)) {
			/* 2rd seg should be code seg, p_flags equals (PF_R + PF_W = 6) */
			current->mm->data_start = p_vaddr;
			current->mm->data_end = p_vaddr + p_memsz;
		}

		vma = (struct vm_area_struct *)get_zero_page();
		if (vma == NULL)
			panic("[exec] ERROR: vma alloc failed!");
		else {
			vma->next = NULL;
			vma->vm_start = p_vaddr;
			vma->vm_end = p_vaddr + p_memsz;
			vma->prot = map_pflags_to_vmprot(p_flags);
			vma->vm_pgoff = p_offset;
			vma->vm_filesz = p_filesz;
			vma->vm_align = p_align;
			vma->vm_file = (file *)&(current->file_array) + fd;
		}

		insert_vma(vma, current->mm);

		ph += phentsize;
	}

	/**
	 * Adjust user heap:
	 * 	if elf has data segment, heap should be right after data_end, 
	 * 	otherwise, user has no data segment, heap should be after code_end
	 */
	if (current->mm->data_end != 0)
		current->mm->user_heap = PG_ALIGN(current->mm->data_end);
	else
		current->mm->user_heap = PG_ALIGN(current->mm->code_end);

	/**
	 * we haven't setup mappings for stack and heap. they are different from code & data segment
	 * they don't have back file.
	 */

	/* insert vma for heap */
/*	vma = (struct vm_area_struct *)get_zero_page();
	if (vma == NULL)
		panic("[exec] ERROR: vma alloc failed!");
	else {
		vma->next = NULL;
		vma->vm_start = current->mm->user_heap;
		vma->vm_end = vma->vm_start + PG_SIZE;
		vma->prot = PTE_RW;
		vma->vm_pgoff = 0;
		vma->vm_filesz = 0;
		vma->vm_align = PG_SIZE;
		vma->vm_file = NULL;
	}
*/
	/* insert current vma at the tail, assume segment are put in increasing order */
//	insert_vma(vma, current->mm);

	/* insert vma for stack */
	vma = (struct vm_area_struct *)get_zero_page();
	if (vma == NULL)
		panic("[exec] ERROR: vma alloc failed!");
	else {
		vma->next = NULL;
		vma->vm_end = (current->mm->user_stack & ~(PG_SIZE - 1)) + PG_SIZE;
		vma->vm_start = vma->vm_end - (0x10 * PG_SIZE); /* stack limit 0x10 * 4K */
		vma->prot = PTE_RW;
		vma->vm_pgoff = 0;
		vma->vm_filesz = 0;
		vma->vm_align = PG_SIZE;
		vma->vm_file = NULL;
	}

	/* insert current vma at the tail, assume segment are put in increasing order */
	insert_vma(vma, current->mm);

	/**
	 *  fake the user stack, make it look like this:
	 *     ________
	 *  0 |        |
	 *  c |        |
	 *  8 |  envp  |
	 *  4 |        |
	 *  0 |  NULL  |
	 *  c |        |
	 *  8 |  argv  |
	 *  4 |        |
	 *  0 |  argc  |
	 *     --------
	 */

	sp = (void *)(current->mm->user_stack);
	sp = (void *)((uint64_t)sp & ~0xf);
	map_a_page(vma, (uint64_t)sp);
	argv = sp - 0x10;
	sp = sp - 0x10;
	*argv = 't';
	*(argv + 1) = 'e';
	*(argv + 2) = 's';
	*(argv + 3) = 't';
	*(argv + 4) = '\0';
	envp = NULL; 
	argc = 1;
	sp = sp - 0x20;
	*(int *)sp = argc;
	*(uint64_t *)(sp + 8) = (uint64_t)argv;
	*(uint64_t *)(sp + 16) = (uint64_t)0;
	*(uint64_t *)(sp + 24) = (uint64_t)envp;
	current->mm->user_stack = (uint64_t)sp;

	/**
	 * Before we return to user, we should set tss.rsp0 to a given kernel stack,
	 * that's the stack when we fall back to kernel, we should use. Now we want use
	 * the current stack, but when we fall back to kernel, we should be at the top
	 * of the stack, pretending it's the first time we fall down to kernel.
	 */
	if ((future_kstack = get_zero_page()) == NULL)
		panic("[exec]ERROR: alloc future_kstack failed!");

	future_kstack = future_kstack + PG_SIZE;
	tss.rsp0 = (uint64_t)future_kstack;

	/* set kernel stack, so that when syscall/interrupt happen, we will use that stack */
	current->kernel_stack = future_kstack;

	/* everything is ready, now we return to user */
	__ret_to_user((void *)(current->mm->user_stack), (void *)(current->mm->entry));
}

static uint64_t map_pflags_to_vmprot(uint32_t p_flags) {
	uint64_t prot = 0;

	/* refer to ELF Segment Permissions documentation */
	switch(p_flags) {
		case 0:	/* should deny all access, but don't know how to express it TODO */
			break;
		case 1:
		case 4:
		case 5:
			/* read and execute only */
			break;
		case 2:
		case 3:
		case 6:
		case 7:
			/* read, write and execute */
			prot |= PTE_RW;
	}

	return prot;
}
