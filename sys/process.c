#include <sys/sbunix.h>
#include <sys/gdt.h>
#include <sys/proc.h>
#include <sys/tarfs.h>
#include <sys/elf.h>
#include <sys/string.h>

/* task struct are linked in a double-linked list, task_headp points to the head */
task_struct *task_headp = NULL;
task_struct *current = NULL;
task_struct *clear_zombie = NULL;
task_struct *sleep_task_list = NULL;
task_struct *wait_stdin_list = NULL;
task_struct *zombie_list = NULL;
int zombie_count = 0;

extern void *pgd_start;
extern void __ret_to_user(void *user_stack, void *user_entry);

/* terminal or keyboard related */
extern char *stdin_buf;
extern int stdin_buf_size;
extern int stdin_buf_state;

/* current highest pid number */
int pid_count = 0;

static task_struct * select_task_struct(void);
static task_struct * switch_to(task_struct *, task_struct *, task_struct **);
static uint64_t map_pflags_to_vmprot(uint32_t p_flags);

int get_strlist_size(char *argv[]);
int get_strlist_num(char *argv[]);
void copy_strlist(char *des_argv[], char *src_argv[], char *start);

void init_proc() {
	task_headp =(task_struct *) get_zero_page();

	task_headp->prev = NULL;
	task_headp->next = NULL;
	task_headp->last = NULL;

	/* pid keep growing */
	task_headp->pid = pid_count++;
	strncpy(task_headp->name, "init", 4);
	strncpy(task_headp->cwd, "/", 1);

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
	strncpy(tsp->cwd, "/", 1);
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

	return tsp->pid;
}


void schedule() {
	task_struct *me, *next;

	me = current;
	next = select_task_struct();

	while (next->sigterm == 1) {
		exit_st(next, 1);
		next = select_task_struct();
	}

	/* give up, maybe no process to schedule */
	if(next == me)
		return;

	current = next;

	/* switch tss.rsp0 */
	tss.rsp0 = (uint64_t)(next->kernel_stack);

	//__switch_to(me, next, &me->last);
	switch_mm(me, next);
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
	if (find_process_by_pid(current->pid, task_headp) != NULL)
		p = current;
	else	//current not in active state
		p = task_headp;

	if(p->next != NULL)
		return p->next;
	else
		return task_headp;
}

void get_interp(char *interp_name, char *fp) {
	int i = 0;
	while((fp[i + 2] != '\n') && i < NAME_SIZE) { //skip '#!'
		interp_name[i] = fp[i+2];
		i++;
	}

	assert(i < NAME_SIZE);
	interp_name[i] = '\0';
}

#define ARG_LIST_SIZE	10
void get_interp_argv(char *interp_argv[], char *argv[], char *interp_name) {
	int i = 0;

	interp_argv[0] = interp_name;
	while (i < ARG_LIST_SIZE && argv[i] != NULL) {
		interp_argv[i + 1] = argv[i];
		i++;
	}

	assert(i < ARG_LIST_SIZE);
	interp_argv[i + 1] = NULL;
}

/**
 * exec the first user process/
 * 	1. load elf_binary into memory(not really, demand-paging) and build corresponding vm_struct.
 *	2  set_up page tables for the user process.
 *	3. ret_to_user to start execute.
 */

void exec(char *filename, char *argv[], char *envp[]) {
	int i, fd;
	void *fp;

	/* interpreter related */
	char interp_name[NAME_SIZE];
	char *interp_argv[ARG_LIST_SIZE];

	/* future kernel stack */
	void *future_kstack;

	/* tmp kernel buffer to keep old argv, envp */
	void *kbuf,*kargvp, *kenvpp, *kstart;

	/* future user stack */
	void *argvp, *envpp, *argcp;
	int arglist_size, envlist_size, argv_num, envp_num, parameter_size;
	uint64_t pa_start, pv_start;

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
	if (fd < START_FD) {
		printf("execve failed: file %s does not exist!\n",filename);
		exit_st(current, 1);
	}

	/* we don't want to change file->pos, give size 0, we will get the file pointer at start pos 0 */
	fp = tarfs_read(fd, 0);

	/* before we cancell old mm_struct, we need first copy data to kernel space*/
	if((kbuf = get_zero_page()) == NULL)
		panic("[exec] alloc kbuf failed!");

	/* map the user stack , we don't want demand paging or cow here */
	if ((uint64_t)argv < 0x800000000000) { // only map when argv passed from user mode
		vma = search_vma((uint64_t)argv, current->mm);
		if (vma != NULL)
			map_a_page(vma, (uint64_t)argv);
		else
			panic("[exec]ERROR:can't find valid vma for argv!");
	}

	/* Add some crazy thing here, support script start with #!bin/sbush */
	if (strncmp(fp,"#!", 2) == 0) {
		get_interp(interp_name, fp);
		get_interp_argv(interp_argv, argv, interp_name);
		exec(interp_name, interp_argv, envp);
	}

	/* toppest user address */
	current->user_stack = 0x800000000000;

	/* calculate stack */
	arglist_size = get_strlist_size(argv); /* count ending '\0' into size */
	envlist_size = get_strlist_size(envp);
	argv_num = get_strlist_num(argv) + 1; /* reserve space for NULL */
	envp_num = get_strlist_num(envp) + 1;

	parameter_size = arglist_size + envlist_size + (argv_num * 8) + (envp_num * 8) + 8;
	assert(parameter_size  + 0x100 < PG_SIZE);

	/* get envp and argv string list's start address */
	pa_start = (current->user_stack - arglist_size - 8) & (~0x7); /* bottom limit, 8 byte align */
	pv_start = (pa_start - envlist_size - 8) & (~0x7);

	/* get envp and argv address */
	envpp = (void *)(pv_start - envp_num * 8);
	argvp = (void *)((uint64_t)envpp - argv_num * 8);

	kstart = kbuf + (argv_num + envp_num)*8 + 0x30;
	kargvp = kbuf;
	kenvpp = kbuf + argv_num * 8 + 0x10;	//gap
	copy_strlist((char **)kargvp, argv, kstart);
	copy_strlist((char **)kenvpp, envp, kstart + arglist_size + 0x30);


	/* free previous mm_struct TODO give up free */
	//unmap_mm(current);
	current->mm = NULL;

	/* change process name */
	strncpy(current->name, argv[0], strlen(argv[0]));

	/* prepare for loading */
	assert(current->mm == NULL);

	if ((current->mm = (struct mm_struct *)get_zero_page()) == NULL)
		panic("[exec]: ERROR alloc mm_struct failed!");

	/* initialize the mm_struct */
	current->mm->mmap = NULL;
	current->mm->entry = ((Elf64_Ehdr *)fp)->e_entry;
	current->mm->pgd = alloc_pgd();
	current->mm->mm_count = 0;
	current->mm->code_start = 0;
	current->mm->code_end = 0;
	current->mm->data_start = 0;
	current->mm->data_end = 0;
	/* will be adjusted later, should be the lowest userable address above .text .data segments */
	current->mm->user_heap = 0;
	current->mm->brk = 0;

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

	current->mm->brk = current->mm->user_heap;

	/* insert vma for stack */
	vma = (struct vm_area_struct *)get_zero_page();
	if (vma == NULL)
		panic("[exec] ERROR: vma alloc failed!");
	else {
		vma->next = NULL;
		vma->vm_end = current->user_stack;
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

	/* map a page for stack, we are going to prepare stack data for new process */
	map_a_page(vma, (uint64_t)(current->user_stack - PG_SIZE));

	/* copy string and string pointer array at the same time */
	copy_strlist((char **)envpp, (char **)kenvpp, (char *)pv_start);
	copy_strlist((char **)argvp, (char **)kargvp, (char *)pa_start);

	/* copy argc */
	argcp = argvp - 8;
	*((int *)argcp) = argv_num - 1;

	/* adjust user_stack */
	current->user_stack = (uint64_t)argcp;

	/* DEBUG:TODO */
	//printf("[execve]dump user stack:\n");
	//dump_stack((void *)(current->user_stack), parameter_size/8);

/**
 * old way to handle stack, not a real case.
 *
	sp = (void *)(current->user_stack);
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
	current->user_stack = (uint64_t)sp;
*/

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
	__ret_to_user((void *)(current->user_stack), (void *)(current->mm->entry));
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

int get_strlist_size(char *argv[]) {
	int i, size;

	size = 0;
	for (i = 0; *(argv + i) != NULL; i++) {
	//	printf("[get_strlist_size]argv+i:%s\n",*(argv + i));
		size += strlen(*(argv + i)) + 1;
	}

	return size;
}

int get_strlist_num(char *argv[]) {
	int i = 0;
	while(*(argv + i) != NULL)
		i++;

	return i;
}

void copy_strlist(char *des_argv[], char *src_argv[], char *start) {
	int i = 0, len;

	while(*(src_argv + i) != NULL) {
	//	printf("[copy_strlist]src argv+i:%s\n", *(src_argv + i));
		des_argv[i] = start;
		len = strlen(*(src_argv + i));
		strncpy(start, *(src_argv + i), len);
		start += len + 1;
		i++;
	}
}

/* always add to head, assume it's a single linked list */
void add_to_tasklist(task_struct **head, task_struct *ts) {
	ts->next = *head;
	*head = ts;
}

/* return 0 on success, -1 on failed */
int remove_from_tasklist(task_struct **head, task_struct *ts) {
	task_struct *p = *head;
	if (p == ts) {
		/* if p points to ts, just remove it */
		*head = ts->next;
		/* when remove, always clear ->next */
		ts->next = NULL;
		return 0;
	}

	/* traverse list to find ts */
	while(p->next != ts) {
		p = p->next;
	}

	/* if we find it, remove it */
	if (p->next == ts) {
		p->next = ts->next;
		ts->next = NULL;
		return 0;
	} else
		return -1;
}

/* put myself on list, and set sleep time */
void sleep(task_struct **list, uint64_t time) {
	int removed = 0;
	removed = remove_from_tasklist(&task_headp, current);
	if(removed == 0) // succeeded
		add_to_tasklist(list, current);
	else
		panic("[sleep]can't find process on active list!");

	current->sleep_time = time;
	current->state = SLEEP;

	/* yield to other process */
	schedule();
}

/**
 * usually, we should wake up all the process on the waiting list, sometimes
 * for example, stdin buffer ready.
 * but sometimes, we should wake up one process, like the process who sleep for
 * a period of time.
 * flag 0, wake up the whole list
 * flag 1, wake up only one task.
 * NOTE(TODO):
 * 	always remove a task_struct before add it to a new list.
 */
void wakeup(task_struct **list, task_struct *ts, int flag) {
	task_struct *p, *next;

	assert(list != NULL);

	if (flag == 0) {
		p = *list;
		next = p;
		while(next != NULL) {
			next = p->next;
			p->state = ACTIVE;
			p->sleep_time = 0;
			remove_from_tasklist(list, p);
			add_to_tasklist(&task_headp, p);
			p = next;
		}
	} else if (flag == 1) {
		assert(ts!=NULL);
		ts->state = ACTIVE;
		ts->sleep_time = 0;
		remove_from_tasklist(list, ts);
		add_to_tasklist(&task_headp, ts);
	} else
		panic("[wakeup]: unknown flag ");

/* should we schedule immediately TODO, maybe not*/
//	schedule();
}

task_struct *find_process_by_pid(int pid, task_struct *task_list) {
	task_struct *p;

	/* we find process on all possible list */
	p = task_list;
	while(p != NULL && p->pid != pid)
		p = p->next;

	if (p != NULL)
		return p;
	else
		return NULL;
}

/* the reverse process of fork, a lot of reclaim of resources*/
void free_process(task_struct *p) {

	assert(p != NULL && p->state == ZOMBIE);

	/* free kernel stack */
	free_page((uint64_t)VA2PA(p->kernel_stack) >> PG_BITS);

	/* we should deduct file count, but we don't need to here, for we don't cache file data */

	/* free user space */
	assert(p->mm != NULL);
	//unmap_mm(p);

	/* remove myself from zombie list */
	remove_from_tasklist(&zombie_list, p);

	/* disappear */
	free_page((uint64_t)VA2PA(p) >> PG_BITS);
}


void thread_clear() {
	/* we do nothing here, just wait for interrupt */
	task_struct *p, *next;
	while (1) {
		p = zombie_list;
		while(p != NULL) {
			next = p->next;
			if (p->orphan == 1) {
				free_process(p);
				//printf("zombie process pid:%d is freed!\n", p->pid);
			}
			p = next;
		}
		zombie_count = 0;
		sleep(&sleep_task_list, -1);
	}
}
