#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[], char* envp[]);

int argc = 1;
uint64_t *pargc;
char **argv;
char **envp;
int res;

void _start(void) {

	__asm__ __volatile__(
	"movq %%rsp, %0\n\t"
	: "=rm" (pargc)
	:
	:);

	pargc += 3;
	argc = *((int*)pargc);
	argv = (char **)((char *)pargc + 8 );
	envp = (char **)((char *)argv + (argc+1)*8);
/*	printf("argc:%d, argv:%lx, envp:%lx\n",tmp, argv, envp);
	pa = argv;
	while(*pa!=NULL)
		printf("%s\n", *pa++);

	pe = envp;
	while(*pe!=NULL)
		printf("%s\n", *pe++);

*/
//	brk((void*)0);
	res = main(argc, argv, envp);


	exit(res);
}
