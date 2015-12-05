#include <sys/sbunix.h>
#include <sys/syscall.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/tarfs.h>

//#define DEBUG_SYSCALL
#define BUF_READY	1
#define BUF_NOT_READY	0

/* file operation related */
#define EOF	0
enum { O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2, O_CREAT = 0x40, O_DIRECTORY = 0x10000 };

// directories
#define NAME_MAX 255
struct  __attribute__((__packed__)) dirent {
  long d_ino;
  off_t d_off;
  unsigned short d_reclen;
  char d_name [NAME_MAX+1];
};


extern task_struct *current;
extern task_struct *clear_zombie;
extern task_struct *task_headp;
extern task_struct *sleep_task_list;
extern task_struct *wait_stdin_list;
extern task_struct *zombie_list;
extern int pid_count;
extern int zombie_count;

extern int stdin_buf_state;
extern int stdin_buf_size;
extern char *stdin_buf;

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

/* file system related */
uint64_t open(sc_frame *);
uint64_t read(sc_frame *);
uint64_t write(sc_frame *);
uint64_t close(sc_frame *);
uint64_t getdents(sc_frame *);
uint64_t getcwd(sc_frame *);
uint64_t chdir(sc_frame *);

/* memory related */
uint64_t brk(sc_frame *);

/* process related */
uint64_t fork (sc_frame *sf);
uint64_t execve(sc_frame *sf);
uint64_t wait(sc_frame *sf);
void exit(sc_frame *sf);
uint64_t kill(sc_frame *sf);
uint64_t getpid(sc_frame *sf);
uint64_t getppid(sc_frame *sf);
uint64_t ps(sc_frame *sf);

/* other helper functions */
uint64_t read_stdin(char *buf, int size);
int copy_from_kernel(char *ubuf, char *kbuf, int size);
void copy_from_user(char *kbuf, char *ubuf, int size);
int belongs_to_dir(const char *dirname, const char *name);

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
	uint64_t res = 0;
	switch(syscall_num) {
		case SYS_open:
			res = open(sf);
			break;
		case SYS_read:
			res = read(sf);
			break;
		case SYS_write:
			res = write(sf);
			break;
		case SYS_close:
			res = close(sf);
			break;
		case SYS_getdents:
			res = getdents(sf);
			break;
		case SYS_getcwd:
			res = getcwd(sf);
			break;
		case SYS_chdir:
			res = chdir(sf);
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
		case SYS_wait4:
			res = wait(sf);
			break;
		case SYS_exit:
			exit(sf);
			break;
		case SYS_kill:
			res = kill(sf);
			break;
		case SYS_getpid:
			res = getpid(sf);
			break;
		case SYS_getppid:
			res = getppid(sf);
			break;
		case SYS_ps:
			res = ps(sf);
			break;
		default:
			res = sys_null(syscall_num);
			break;
	}

	return res;
}

/* page fault may happen, it's buggy TODO hope ubuf doesn't cross page*/
void copy_from_user(char *kbuf, char *ubuf, int size) {
	int i = 0;
	vma_struct *vma;

#ifdef DEBUG_SYSCALL
	printf("ubuf:0x%lx, kbuf:0x%lx\n",ubuf, kbuf);
#endif
	vma = search_vma((uint64_t)ubuf, current->mm);
	if (vma != NULL)
		map_a_page(vma, (uint64_t)ubuf);


	for (i = 0; i < size; i++)
			*(kbuf + i) = *(ubuf + i);
}

/* page fault may happen, it's buggy TODO hope ubuf doesn't cross page*/
int copy_from_kernel(char *ubuf, char *kbuf, int size) {
	int i = 0;
	vma_struct *vma;

#ifdef DEBUG_SYSCALL
	printf("ubuf:0x%lx, kbuf:0x%lx, current:0x%lx, mm:0x%lx\n",ubuf, kbuf, current, current->mm);
#endif
	vma = search_vma((uint64_t)ubuf, current->mm);
	if (vma != NULL)
		map_a_page(vma, (uint64_t)ubuf);
	else {
		printf("[copy_from_kernel]wrong parameter! ubuf:0x%lx\n");
		return -1;
	}


	for (i = 0; i < size; i++)
			*(ubuf + i) = *(kbuf + i);
	return 0;
}

uint64_t read_stdin(char *buf, int size) {
	uint64_t readsz;
	int i = 0;

	//printf("read_stdin:buf:0x%lx, size : 0x%lx\n", (uint64_t)buf, size);
	if (size >= stdin_buf_size) {
		if (copy_from_kernel(buf, stdin_buf, stdin_buf_size) != 0)
			return -1;
		stdin_buf_size = 0;
		readsz = stdin_buf_size;

		/* only when buffer is empty, set it not ready */
		stdin_buf_state = BUF_NOT_READY;
	} else {
		if (copy_from_kernel(buf, stdin_buf, size) != 0)
			return -1;
		for (i = size; i < stdin_buf_size; i++)
			stdin_buf[i-size] = stdin_buf[i];

		stdin_buf_size -= size;
		readsz = size;
	}

	return readsz;
}

uint64_t open(sc_frame *sf) {
	int fd;
	char *path;
	uint64_t flag;
	uint64_t mode;

	path = (char *)(sf->rdi);
	flag = sf->rsi;
	mode = sf->rdx;

	/* only allow read access */
	assert(flag == O_RDONLY);

	/* only allow default mode */
	assert(mode == 0);

	fd = tarfs_open(path, "r");

#ifdef DEBUG_SYSCALL
	printf("[open] path:%s, fd:%d\n",path, fd);
#endif

	return (uint64_t)fd;
}

/* fd 0: stdin, 1: stdout, 2: stderr */
uint64_t read(sc_frame *sf) {
	int fd;
	char *buf;
	void *p;
	uint64_t size, readsz;

	fd = (int)(sf->rdi);
	buf = (char *)(sf->rsi);
	size = sf->rdx;
	readsz = 0;

	/* can't read stdout and stderr */
	assert(fd != 1 && fd != 2);

	/* undefined error: size == 0 */
	if (size == 0)
		return -1;

	if (fd == 0) { /* read from stdin */
		/* do we need lock here to guarantee ATOMICALITY TODO */
		while(stdin_buf_state != BUF_READY) {
			sleep(&wait_stdin_list, 0);
		}

		readsz = read_stdin(buf, size);
	} else {
		assert(current->file_array[fd].type == FT_FILE);

		/* check if we are at filetail */
		if (current->file_array[fd].pos == current->file_array[fd].size)	// EOF
			return EOF;

		if (current->file_array[fd].pos + size <= current->file_array[fd].size)
			readsz = size;
		else
			readsz = current->file_array[fd].size - current->file_array[fd].pos;

		/* copy data from kernel to user buf */
		p = current->file_array[fd].start_addr + current->file_array[fd].pos;
/*		printf("[read] pos : 0x%lx\n",current->file_array[fd].pos);
		printf("[read] start_addr : 0x%lx\n",current->file_array[fd].start_addr);
		int i;
		for(i = 0; i < readsz; i++)
			printf("%c",*((char *)p+i));
		printf("\n");
		printf("[read] filename:%s, content:%s, readsz:%d\n",current->file_array[fd].name, (char *)p, readsz);
*/
		if (copy_from_kernel(buf, p, readsz) != 0)
			return -1;

		/* move read pointer */
		current->file_array[fd].pos += readsz;
	}

	return readsz;
}

/* only support write to stdout TODO */
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

/* close file */
uint64_t close(sc_frame *sf) {
	int fd = (int)(sf->rdi);

	/* does not allow close stdin, stdout, stderr */
	assert(fd != 0 && fd != 1 && fd != 2);

	tarfs_close(fd);
	return 0;
}

/* return 0 on success */
uint64_t chdir(sc_frame *sf) {
	char *path = (char *)(sf->rdi);
	char cwd[NAME_SIZE];
	int fd;
	int len;

	copy_from_user(cwd, path, NAME_SIZE);

	len = strlen(cwd);
	// support cd .. but not cd ../../
	if (len >= 3 && cwd[len - 1] == '.' && cwd[len-2] == '.' && cwd[len-3] == '/') {
		len = len - 3;
		//skip duplicate '//'
		while(cwd[len] == '/')
			len--;

		// go back one directory
		while(len >= 0 && cwd[len] != '/')
			len--;

		if(len >= 0)
			cwd[len + 1] = '\0';
		else {
			cwd[0] = '/';
			cwd[1] = '\0';
		}
	}

#ifdef DEBUG_SYSCALL
	printf("[chdir] cwd after deal with ..:%s\n", cwd);
#endif
	/* validate whether it's a legal path, no access control here */
	if ((fd = tarfs_open(cwd,"r")) > 0) {
		len = strlen(current->file_array[fd].name);
		strncpy(current->cwd, current->file_array[fd].name, len);
		tarfs_close(fd);
		return 0;
	} else
		return -1;
}

uint64_t getcwd(sc_frame *sf) {
	char *path = (char *)(sf->rdi);
	int size = (int)(sf->rsi);
	int len;

	len = strlen(current->cwd);
	assert(len <= size);
	/* when copy string, we should copy the ending '\0' */
	if (copy_from_kernel(path, current->cwd, len + 1) != 0)
		return -1;

	return (uint64_t)len;
}

/**
 * this is a comparator function, test whether name is in
 * the format of dirname/bla, and bla should not contain '/'.
 * which means it belongs to dir/
 * we should always handle the special root directory, which is faked by us.
 * 0: means true, 1 means not true;
 */
int belongs_to_dir(const char *dirname, const char *name) {
	int i;
	int len, len1;

#ifdef DEBUG_SYSCALL
	printf("[belongs_to_dir] dirname:%s, name:%s\n",dirname, name);
#endif

	len = strlen(dirname);
	len1 = strlen(name);

	if ((len == 0) || (len1 == 0))
		return 1;

	if (strcmp(dirname, "/") == 0) { // special case, faked root '/'
		for(i = 0; i < len1; i++)
			if (name[i] == '/' && name[i+1] !='\0') // not end with /, like bin/, but contains /, that's not what we want
				return 1;
			else if(name[i] == '/' && name[i+1] == '\0')
				return 0;
			else // name[i] != '/'
				continue;
		return 0;	// does not contains / at all, that's a file under root '/'
	}

	// given bin/, we want bin/bla, bin/bla/, but not bin/bla/blabla
	if (strncmp(dirname, name, len) == 0) {
		for(i = len; i < len1; i++)
			if (name[i] == '/' && name[i+1] == '\0')	// case :bin/bla/
				return 0;
			else if(name[i] == '/' && name[i+1] != '\0')	// case :bin/bla/blabla
				return 1;
			else
				continue;
		return 0;	// case: bin/bla
	} else
		return 1;	//case: other/bla, not start with bin/
}

/* read dents */
uint64_t getdents(sc_frame *sf) {
	int fd = (int)(sf->rdi);
	char *dp = (char *)(sf->rsi);
	int buf_size = (int)(sf->rdx);

	int type;
	void *start, *res;
	int readsz;
	char dirname[FILE_NAME_SIZE] = {0};
	int len, len1;

	void *kbuf = get_zero_page();

	/**
	 * we don't support real r/w filesystem, only tarfs here.
	 * However, tarfs has no idea of directory, neither dirents, so we have to 
	 * fake it.
	 * when the file type is directory, the pos means something else, it points to the
	 * header of a file header in tarfs.
	 * Remember, we judge whether a file belongs to a directory by its name dir, dir/filename
	 */

	/* assert it's a legal directory */
	assert(fd >= START_FD && fd < MAX_FILE_NUM);
	assert(current->file_array[fd].type == FT_DIR);

	len = strlen(current->file_array[fd].name);
	strncpy(dirname, current->file_array[fd].name, len);

#ifdef DEBUG_SYSCALL
	printf("[getdents]dirname:%s, original name:%s\n",dirname, current->file_array[fd].name);
#endif
	/* start traverse the tarfs, and find file that with name beginning with dirname/ */
	start = current->file_array[fd].start_addr + current->file_array[fd].pos;
	res = traverse_tarfs(dirname, start, belongs_to_dir, &type);
	if (res != NULL) {
		if (type == FT_DIR)
			current->file_array[fd].pos = res - current->file_array[fd].start_addr + TARFS_HEADER;
		else if(type == FT_FILE)
			current->file_array[fd].pos = res - current->file_array[fd].start_addr + ROUND_TO_512(get_filesz(res)) + TARFS_HEADER;
		else {
			printf("[getdents] unknown filetype!\n");
			return -1;
		}
	} else	// no more dirent found
		return 0;

	/* fill in the dirents */
	readsz = sizeof(struct dirent);
	assert(readsz <= buf_size);

	/**
	 * d_ino should contains the inode info, but we use it to contain the file type info instead
	 * for we don't inode.
	 * NOTE user process will check if it's 0, so it should not be 0.
	 */
	((struct dirent *)kbuf)->d_ino = type + 1;
	((struct dirent *)kbuf)->d_off = readsz;
	((struct dirent *)kbuf)->d_reclen = readsz;
	len1 = strlen(((struct posix_header_ustar *)res)->name);
	assert(len1 < NAME_MAX + 1); 
	if (strcmp(dirname, "/") == 0) {
		/**
		 * that means we should copy the entire name, note: tarfs filename doesn't start with '/'
		 * otherwise we need to remove prefix, possible dirname /, res->name bin/
		 */
		strncpy(((struct dirent *)kbuf)->d_name, ((struct posix_header_ustar *)res)->name, len1);
	} else {
		/* possible dirname /bin, tarfs filename bin/hello , get hello out of it*/
		strncpy(((struct dirent *)kbuf)->d_name, ((struct posix_header_ustar *)res)->name + len, len1 - len);
	}

	if (copy_from_kernel(dp, kbuf, readsz) != 0)
		return -1;

#ifdef DEBUG_SYSCALL
	printf("res->name:%s\n", ((struct posix_header_ustar *)res)->name);
	printf("d_name:%s\n",((struct dirent *)kbuf)->d_name);
#endif

	/* reclaim kernel page */
	free_page((uint64_t)VA2PA(kbuf) >> PG_BITS);

	return (uint64_t)readsz;
}

uint64_t sys_null(int num) {
	int res = 0;
	printf("syscall num = %d, not supportted yet!\n", num);

	exit_st(current, 1);
	return res;
}

/**
 * change user process program break to addr.
 */
#define HEAP_LIMIT	0x7ffffff00000
uint64_t brk(sc_frame *sf) {
	uint64_t new_break, heap_start,cur_break;
	vma_struct *vma;

	new_break = sf->rdi;
	heap_start = current->mm->user_heap;
	cur_break = current->mm->brk;

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
		if (cur_break == heap_start) {	//haven't alloc vma for heap yet.
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
		} else {	// extend heap
			vma = search_vma(cur_break - 1, current->mm);
			assert((vma != NULL) && ((vma->prot & PTE_RW) == 1) && (vma->vm_file == NULL));
			/* ok, it's the writable data vma we want to extend */
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
	strncpy(tsp->cwd, current->cwd, strlen(current->cwd));

	/* copy mm_struct */
	if ((tsp->mm = (mm_struct *)get_zero_page()) == NULL)
		panic("[fork]: ERROR alloc mm_struct failed!");

	/* copy the mm_struct */
	tsp->mm->mmap = NULL;
	tsp->mm->entry = current->mm->entry;
	tsp->mm->pgd = alloc_pgd();
	tsp->mm->mm_count = current->mm->mm_count;
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
	tsp->mm->brk = current->mm->brk;

	/* copy vma and mark cow */
	copy_mm(tsp->mm, current->mm);

	/* flush tlb here, or we can't catch exception for COW TODO*/
	flush_tlb();

	/* change process state */
	tsp->state = ACTIVE;

	/* insert new task struct at the head of the list */
	add_to_tasklist(&task_headp, tsp);

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

/* wait for child process's state change, like terminate */
uint64_t wait(sc_frame *sf) {
	int pid = (int)(sf->rdi);

//	ignore status, options
//	int options = (int)(sf->rdx);
//	int *status = (int *)(sf->rsi);

	// status comes from user space
/*	vma_struct *vma;

	printf("[wait]status addr:0x%lx\n",status);
	vma = search_vma((uint64_t)status, current->mm);
	if (vma != NULL)
		map_a_page(vma, (uint64_t)status);
	else
		panic("[wait]ERROR status is an invalid address!\n");

	*status =  waitpid(pid);
*/
	waitpid(pid);

	return 0;
}

uint64_t waitpid(int pid) {
	task_struct *child;
	int status;

	/* ignore status and options */
	child = find_process_by_pid(pid, task_headp);
	if (child == NULL)
		child = find_process_by_pid(pid, sleep_task_list);
	if (child == NULL)
		child = find_process_by_pid(pid, wait_stdin_list);
	if (child == NULL) {
		printf("can't find child process with pid :%d\n", pid);
		return -1;
	}

	// we are not waiting for timer, so set time: -1, set child, hope it will wait me up
	child->waited = 1;
	sleep(&sleep_task_list,  -1);

	status = child->exit_status;

	free_process(child);

	return status;
}

/* father will free me */
void exit(sc_frame *sf) {
	int status = (int)(sf->rdi);
	exit_st(current, status);
}

void exit_st(task_struct *ts, int status) {
	task_struct *father;

	ts->exit_status = status;
	ts->state = ZOMBIE;
	remove_from_tasklist(&task_headp, ts);
	add_to_tasklist(&zombie_list, ts);

	if (ts->waited == 1) {
		father = find_process_by_pid(ts->ppid, sleep_task_list);

		// we need notify init to adopt this orphon, but now we don't support it
		// first mark meself, so that if init want to do it, he can do it
		if (father == NULL)
			ts->orphan = 1;
		 else
			wakeup(&sleep_task_list, father, 1);
	} else	// no one wait for me , so mark myself orphan, let init adopt us.
		ts->orphan = 1;

	if (ts->orphan == 1) {
		zombie_count++;
		if(zombie_count == 50)
			wakeup(&sleep_task_list, clear_zombie, 1);
	}

	/* once schedule away, never come back, GOODBYE! */
	schedule();
}

/* support kill to terminate process */
uint64_t kill(sc_frame *sf) {
	int pid = (int)(sf->rdi);
	int sig = (int)(sf->rsi);

	return kill_by_pid(pid, sig);
}

uint64_t kill_by_pid(int pid, int sig) {
	task_struct *tsp;

	tsp = find_process_by_pid(pid, task_headp);
	if (tsp == NULL)
		tsp = find_process_by_pid(pid, sleep_task_list);
	if (tsp == NULL)
		tsp = find_process_by_pid(pid, wait_stdin_list);
	if (tsp == NULL) {
		printf("[kill_by_pid] can't find child process with pid :%d\n", pid);
		return -1;
	}

	// ignore all sig but terminate
	if (sig == SIGKILL) {
		tsp->sigterm = 1;
	}

	return 0;
}

uint64_t getpid(sc_frame *sf) {
	return (uint64_t)(current->pid);
}

uint64_t getppid(sc_frame *sf) {
	return (uint64_t)(current->ppid);
}

uint64_t ps(sc_frame *sf) {
	task_struct *p;

	printf("pid\tppid\tuser\tstate\tprocess_name\n");
	p = task_headp;
	while(p != NULL) {
		printf("%d\t%d\t%s\t%s\t%s\n", p->pid, p->ppid, (p->mm == NULL) ? "root" : "zhisun", (p->pid == current->pid)? "RUNING" : "ACTIVE", p->name);
		p = p->next;
	}

	p = sleep_task_list;
	while(p != NULL) {
		printf("%d\t%d\t%s\tSLEEP\t%s\n", p->pid, p->ppid, (p->mm == NULL) ? "root" : "zhisun", p->name);
		p = p->next;
	}

	p = wait_stdin_list;
	while(p != NULL) {
		printf("%d\t%d\t%s\tSLEEP\t%s\n", p->pid, p->ppid, (p->mm == NULL) ? "root" : "zhisun", p->name);
		p = p->next;
	}

	p = zombie_list;
	while(p != NULL) {
		printf("%d\t%d\t%s\tZOMBIE\t%s\n", p->pid, p->ppid, (p->mm == NULL) ? "root" : "zhisun", p->name);
		p = p->next;
	}

	return 0;
}
