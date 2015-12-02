#include <stdio.h>
#include <stdlib.h>
#include <sbush.h>

int main(int argc, char *argv[], char *envp[]) {
	char *cmd;

	cmd =(char*) malloc(MAX_CMD_SIZE * sizeof(char));
	while(1){
		printf("sbush$ ");
		myfgets(cmd, MAX_CMD_SIZE, 0);
		printf("I get cmd:%s\n", cmd);
	}

	return 0;
}
