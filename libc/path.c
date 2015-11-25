#include<stdlib.h>
#include<syscall.h>
char *getcwd(char *buf, size_t size){
	int ret;
	char *path;

	if (buf == NULL){
		path = (char*)malloc(256);
	}else
		path = buf;
	
	ret = syscall_2(SYS_getcwd, (uint64_t) path, (uint64_t) size);
	if(ret >= 0)
		return path;
	else
		return NULL;
}

int chdir(const char *path){
	return (int) syscall_1(SYS_chdir, (uint64_t) path);
}

