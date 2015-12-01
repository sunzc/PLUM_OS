#include <stdio.h>
#include <stdlib.h>

int g = 1;
int main(int argc, char* argv[], char* envp[]) {
	int pid;

	printf("hello world!\n");
	printf("test fork! g :%d\n", g);
	pid = fork();
	if (pid == 0) {//child
		g++;
		printf("I'm in child!, global g:%d\n", g);
	} else {
		g++;
		printf("I'm in father, child pid = %d, g++ = %d\n", pid, g);
	}

	printf("I'm in both father and child, g:%d\n",g);
	return 0;
}
