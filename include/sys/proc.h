#ifndef _PROC_H
#define _PROC_H

#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/tarfs.h>

#define MAX_FILE_NUM		20
#define THREAD_NAME_SIZE	100

typedef struct __attribute__((__packed__))  task_struct{
	struct task_struct *prev;
	struct task_struct *next;
	struct task_struct *last;


	void *kernel_stack;
	uint64_t user_stack;

	/* file array keep track of opened files */
	file file_array[MAX_FILE_NUM];

	/* mm_struct descripe the user process vm structures */
	struct mm_struct *mm;

	/* only increase */
	int pid;
	int ppid;
	char name[THREAD_NAME_SIZE];

	/* points to kernel thread function */
	void (*func)(void);
}task_struct;

typedef struct __attribute__((__packed__))  context_info{
	uint64_t rcx;	
	uint64_t rax;
	uint64_t rflags;	
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rbx;
	uint64_t rbp;
}context_info;

//void __switch_to(void);
task_struct * __switch_to(task_struct *me, task_struct *next, task_struct **last);
void schedule(void);
void init_proc(void);
int kernel_thread(void (*f)(void), char *thread_name);
void exec(char *);
void switch_mm(task_struct *prev, task_struct *next);

#endif
