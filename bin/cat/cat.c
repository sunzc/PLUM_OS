#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sbush.h>

int main(int argc, char* argv[], char* envp[]){
	char *fullpath, *buf;
	int fd;

	if (argc == 1)
		return 0;
	else if (argc == 2) {
		fullpath = get_fullpath(argv[1], envp);

		fd = open(fullpath,O_RDONLY);

		if (fd > 0) {
			buf = (char *) malloc(PAGE_SIZE * sizeof(char));
			if (!buf)
				return -1;
			else{
				while((read(fd, buf, PAGE_SIZE)) > 0)
					printf("%s", buf);
			}
		}
	} else
		printf("Too many arguments for cat!\n");

	return 0;
}
