#include <sys/sbunix.h>
#include <sys/mm.h>
#include <sys/proc.h>
#include <sys/tarfs.h>
#include <sys/elf.h>

/* task struct are linked in a double-linked list, task_headp points to the head */
task_struct *task_headp = NULL;
task_struct *current = NULL;
extern tarfs_file* tarfs_file_array;

/* current highest pid number */
int pid_count = 0;

static task_struct * select_task_struct(void);
static task_struct * switch_to(task_struct *, task_struct *, task_struct **);

void init_proc() {
	task_headp =(task_struct *) get_zero_page();

	task_headp->prev = NULL;
	task_headp->next = NULL;
	task_headp->last = NULL;

	/* pid keep growing */
	task_headp->pid = pid_count++;

	/* mm_struct describe vm structure of user process, kernel thread 's mm is NULL */
	task_headp->mm = NULL;

	/* this very first process's kernel_stack will be changed in switch_to */
	task_headp->func = NULL;
	task_headp->kernel_stack = NULL; 

	/* current points the running process's task_struct */
	current = task_headp;
}

int kernel_thread(void (*f)(void)) {
	task_struct *tsp;

	if ((tsp = (task_struct *)get_zero_page()) == NULL)
		return -1;

	/* initialize the task struct */
	tsp->pid = pid_count++;
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
 * 	1. load elf_binary into memory and build corresponding vm_struct.
 *	2  set_up page tables for the user process.
 *	3. ret_to_user to start execute.
 *
 *	Demand paging seems to be easier, but need to handle page fault, that's bad.
 *	Let's avoid demand paging(loading) first, and map all the pages during this.
 */

void exec(char *filename) {
	int i, fd;
	void *fp;

	/* vma struct */
	struct vm_area_struct *vma, *vmap;

	/* Elf related part, mostly the program header */
	void *elf_header, *ph;
	uint64_t phoff;
	int phnum, phentsize;

	/* fields that inside a program header */
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
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
	current->mm->pgd = alloc_pgd();
	current->mm->mm_count = 0;
	current->mm->highest_vm_end = 0;

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
		/* skip none loadable segment, do not consider PT_INTERP, for we don't support dynamic loader */
		if (((Elf64_Phdr *)ph)->p_type != PT_LOAD)
			continue;

		p_flags	= ((Elf64_Phdr *)ph)->p_flags;
		p_offset= ((Elf64_Phdr *)ph)->p_offset;
		p_vaddr = ((Elf64_Phdr *)ph)->p_vaddr;
		p_paddr = ((Elf64_Phdr *)ph)->p_paddr;
		p_filesz= ((Elf64_Phdr *)ph)->p_filesz;
		p_memsz	= ((Elf64_Phdr *)ph)->p_memsz;
		p_align	= ((Elf64_Phdr *)ph)->p_align;

		vma = (struct vm_area_struct *)get_zero_page();
		if (vma == NULL)
			panic("[exec] ERROR: vma alloc failed!");
		else {
			vma->next = NULL;
			vma->vm_start = p_vaddr;
			vma->vm_end = p_vaddr + p_memsz;
			vma->prot = (uint64_t)p_flags;
			vma->vm_pgoff = p_offset;
			vma->vm_file = &tarfs_file_array[fd];
		}

		if (current->mm->mmap == NULL)
			current->mm->mmap = vma;
		else {
			/* insert current vma at the tail, assume segment are put in increasing order */
			vmap = current->mm->mmap;
			while(vmap->next != NULL)
				vmap  = vmap->next;
			vmap->next = vma;
		}

		/* map current vma into memory */
		map_vma(vma, current->mm->pgd);

		ph += phentsize;		
	}
}
