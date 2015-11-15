#ifndef _MM_H
#define _MM_H

#include <sys/defs.h>

typedef struct task_struct{
	struct task_struct *prev;
	struct task_struct *next;
	struct task_struct *last;
	void *kernel_stack;
	int pid;
	void (*func)(void);
}task_struct;

typedef struct context_info{
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
}context_info;

void __switch_to(task_struct *me, task_struct *next, task_struct **last);
#endif