#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sbush.h>

int main(int argc, char* argv[], char* envp[]) {
	char *fullpath = NULL;

	if (argc == 1)
		fullpath = getcwd(NULL, MAX_BUFFER_SIZE - 1);
	else if(argc == 2)
		fullpath = get_fullpath(argv[1], envp);
	else{
		printf("ERROR: too many arguments!\n");
		return -1;
	}

	list_file(fullpath);

	return 0;
}
