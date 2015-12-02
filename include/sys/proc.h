#ifndef _PROC_H
#define _PROC_H

#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/tarfs.h>

#define MAX_FILE_NUM		20
#define THREAD_NAME_SIZE	100

typedef enum TASK_STATE{RUNNING=1, ACTIVE, SLEEP} TASK_STATE;

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

	/**
	 * process state
	 * process have three states:
	 * 	1. running. ### current points to it
	 * 	2. active.  ### task_headp points to the list, they are ready to run.
	 *	3. sleep.   ### sleep list, a process may wait for some resources.
	 *
	 * In detail, sleep list are separate lists, each resouce may have its own sleep list.
	 * we currently support two kinds of sleep lists:
	 * 	a) wait for a timer interrupt, that means it sleep for a certain time.
	 *	b) wait for std input.
	 */
	TASK_STATE state;
	int sleep_time;

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
void exec(char *filename, char *argv[], char *envp[]);
void switch_mm(task_struct *prev, task_struct *next);
void unmap_mm(task_struct *tsp);
int remove_from_tasklist(task_struct **head, task_struct *ts);
void add_to_tasklist(task_struct **head, task_struct *ts);
void sleep(task_struct **list, uint64_t time);
void wakeup(task_struct **list, task_struct *ts, int flag);

#endif
