#include <stdlib.h>
#include <stdio.h>

/* Defined in brk.c */
//extern void *__curbrk;
extern uint64_t __curbrk;
extern size_t __max_addr;

void *sbrk(size_t incr)
{
	//char *old_brk = (char *)__curbrk;
	int ret;
	uint64_t old_brk = __curbrk;

	if ((incr < 0) || (__curbrk + incr) > __max_addr) {
		//errno = ENOMEM;
		printf("ERROR: sbrk failed. Ran out of memory ...\n");
		return NULL;
	}

	if(incr == 0){
//		printf("in sbrk: curbrk %lx\n", old_brk);
		return (void *)old_brk;
	}

	ret = brk((void *)(old_brk + incr));

//	printf("[sbrk] ret from brk :0x%lx, old_brk :0x%lx", ret, old_brk);

	if (ret < 0){
		printf("ERROR: sbrk failed!\n");
		return NULL;
	}

//	printf("in sbrk: old_brk is %lx, after incr, brk %lx\n", old_brk, __curbrk);
	return (void *)old_brk;
}
