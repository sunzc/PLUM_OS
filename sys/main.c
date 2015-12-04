#include <sys/sbunix.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <sys/timer.h>
#include <sys/tarfs.h>
#include <sys/proc.h>
#include <sys/syscall.h>

extern phymem_block pmb_array[MAX_PHY_BLOCK];
extern uint32_t pmb_count;
extern task_struct *current;

void *_physbase;
void *_physfree;
void *_loaderbase;
void *_loaderend;

void thread_A(void);
void thread_B(void);
void run_init_process(void);
void thread_idle(void);

void start(uint32_t* modulep, void* physbase, void* physfree)
{
	struct smap_t {
		uint64_t base, length;
		uint32_t type;
	}__attribute__((packed)) *smap;
	while(modulep[0] != 0x9001) modulep += modulep[1]+2;
	for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
		if (smap->type == 1 /* memory */ && smap->length != 0) {
			pmb_array[pmb_count].base = smap->base;
			pmb_array[pmb_count].length = smap->length;
			pmb_count++;
			printf("Available Physical Memory [%x-%x]\n", smap->base, smap->base + smap->length);
		}
	}
	printf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);

	// kernel starts here

	_physbase = physbase;
	_physfree = physfree;
	_loaderbase = (void *)pmb_array[0].base;
	_loaderend = (void *)(pmb_array[0].base + pmb_array[0].length);

	picinit();
	timerinit();
	kbdinit();
	init_idt();
	mm_init();
	/* keep interrupt closing to test context switch */
	init_proc();
	//kernel_thread(thread_A);
	//kernel_thread(thread_B);
	init_tarfs();
//	test_tarfs();
	init_syscall();
/*	while(1) {
		printf("I'm the init process\n");
		schedule();
	}
*/
//	__asm volatile("sti"::);
	kernel_thread(thread_idle, "idle");

	int kinit_pid;
	kinit_pid = kernel_thread(run_init_process, "init_process");

	while(1) {
		//printf("[main]nothing to do, call schedule!\n");
		waitpid(kinit_pid);
		printf("haha, I will never die!\n");
		kinit_pid = kernel_thread(run_init_process, "init_process");
		schedule();
	}
}

void run_init_process() {
	printf("process name: %s\n", current->name);
//	exec("bin/sbush");
//	char *arg0 = "bin/ls";
	char *arg0 = "bin/sbush";
//	char *arg0 = "bin/testfork";
//	char *arg1 = "bin/";
	char *env0 = "HOME=/";
	char *env1 = "USER=zhisun";
	char *env2 = "PATH=/bin/:/lib/";
	char *argv[3], *envp[4];

	argv[0] = arg0;
//	argv[1] = arg1;
//	argv[2] = NULL;
	argv[1] = NULL;
	envp[0] = env0;
	envp[1] = env1;
	envp[2] = env2;
	envp[3] = NULL;

//	exec("bin/testfork", argv, envp);
	exec("bin/sbush", argv, envp);
//	exec("bin/ls", argv, envp);
}

void thread_A() {
	while(1) {
		printf("I'm thread_A\n");
		schedule();
	}
}
void thread_idle() {
	/* we do nothing here, just wait for interrupt */
	while(1);
}

void thread_B() {
	while(1) {
		printf("I'm thread_B\n");
		schedule();
	}
}

#define INITIAL_STACK_SIZE 4096
char stack[INITIAL_STACK_SIZE];
uint32_t* loader_stack;
extern char kernmem, physbase;
struct tss_t tss;

void boot(void)
{
	// note: function changes rsp, local stack variables can't be practically used
	register char *s, *v;
	__asm__(
		"movq %%rsp, %0;"
		"movq %1, %%rsp;"
		:"=g"(loader_stack)
		:"r"(&stack[INITIAL_STACK_SIZE])
	);
	reload_gdt();
	setup_tss();
	start(
		(uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase),
		&physbase,
		(void*)(uint64_t)loader_stack[4]
	);
	s = "!!!!! start() returned !!!!!";
	for(v = (char*)0xffffffff800b8000; *s; ++s, v += 2) *v = *s;
	while(1);
}
