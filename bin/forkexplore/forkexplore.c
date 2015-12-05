#include <stdio.h>
#include <stdlib.h>
#include <sys/defs.h>


int main (int argc, char* argv[], char* envp[]) {

    pid_t pid = fork();

    int count = 100;

    while (((count--) > 0) && pid > 0){

        printf("I am parent process %d\n", getpid());

        pid = fork();

    }

    if (pid == 0) {

        printf("I am user process %d\n", getpid());

    }

}
