#include <stdlib.h>
#include <syscall.h>
#include <stdio.h>

#define MAX_HEAP (1<<30) /* 1GB */
#define DATA_START (1<<29) /* 512MB */

/* sbrk expects this */
//void * __curbrk = (void *)DATA_START;
uint64_t __curbrk;
const size_t __max_addr = (size_t)MAX_HEAP;

int brk(void *addr)
{
	uint64_t ret;

	ret = syscall_1(SYS_brk, (uint64_t)addr);
	
//	printf("in brk: ret is %lx, * addr is %lx\n", ret, ((uint64_t )addr));
	
	__curbrk = ret;

	if(ret == (uint64_t)addr)
		return 0;
	else
		return -1;
}
