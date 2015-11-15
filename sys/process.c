#include <sys/sbunix.h>
#include <sys/mm.h>
#include <sys/proc.h>

/* task struct are linked in a double-linked list, task_headp points to the head */
task_struct *task_headp = NULL;
task_struct *current = NULL;
int pid_count = 0;

void init_proc() {
	task_headp =(task_struct *) get_zero_page();

	task_headp->prev = NULL;
	task_headp->next = NULL;

	task_headp->pid = pid_count++;
	task_headp->func = NULL;
	task_headp->kernel_stack = NULL; 

	/* current points the running process's task_struct */
	current = task_headp;
}

int kernel_thread(void (*f)(void)) {
	task_struct *tsp;

	tsp = (task_struct *)get_zero_page();

	/* initialize the task struct */
	tsp->pid = pid_count++;
	tsp->func = f;
	tsp->kernel_stack = get_zero_page();

	/* prepare context info */
	*(uint64_t)(tsp->kernel_stack + STACK_SIZE - 8) = f;
	(context_info *)(tsp->kernel_stack + STACK_SIZE - 8 - sizeof(context_info))->rcx = &(tsp->last);
	(context_info *)(tsp->kernel_stack + STACK_SIZE - 8 - sizeof(context_info))->rax = tsp;
	tsp->kernel_stack = tsp->kernel_stack + STACK_SIZE - 8 - sizeof(context_info);

	/* insert new task struct at the head of the list */
	tsp->next = task_headp;
	task_headp->prev = tsp;
	tsp->prev = NULL;
	task_headp = tsp;
}


void schedule() {
	task_struct *me, *next;

	next = select_task_struct();
	assert(next != me);

	current = next;

	__switch_to(me, next, &me->last);
}

task_struct * select_task_struct(void) {
	task_struct *p;

	/* choose next task in a round-robin way */
	p = current;
	if(p->next != NULL)
		return p->next;
	else
		return task_headp;
}
