#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <stdarg.h>

void handle_signal(int signal) {
	printf("I am handle signal function!\n");
}

int main(int argc, char* argv[], char* envp[]){
	void (*res)(int);
	res = signal(SIGINT, handle_signal);
	if(res == SIGERR){
		printf("signal failed!\n");
		exit(0);
		return 0;
	}
	for(;;){
		sleep(3);	
	}
	exit(0);
	return 0;

}


