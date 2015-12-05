#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *envp[]) {
	int pid, sig;

	if (argc != 3) {
		printf("ERROR: wrong format of kill!");
		printf("use: kill -9 pid");
	}

	/* get option '-9' */
	sig = get_sig(argv[1]);
	pid = atoi(argv[2]);
	
	kill(pid, sig);
}
