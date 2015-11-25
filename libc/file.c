#include<stdlib.h>
#include<syscall.h>

typedef uint64_t mode_t;
int open(const char *pathname, int flags){
	mode_t mode = 0;
	return (int) syscall_3(SYS_open, (uint64_t) pathname, (uint64_t) flags, (uint64_t) mode);
}

ssize_t read(int fd, void *buf, size_t count){
	return (ssize_t) syscall_3(SYS_read, (uint64_t) fd, (uint64_t) buf, (uint64_t)count);
}

ssize_t write(int fd, const void *buf, size_t count){
	return (ssize_t) syscall_3(SYS_write, (uint64_t) fd, (uint64_t) buf, (uint64_t)count);
}

off_t lseek(int fildes, off_t offset, int whence){
	return (off_t) syscall_3(SYS_write, (uint64_t) fildes, (uint64_t) offset, (uint64_t)whence);
}

int close(int fd){
	return (int) syscall_1(SYS_close, (uint64_t) fd);
}

int dup(int oldfd){
	return (int) syscall_1(SYS_dup, (uint64_t) oldfd);
}
int dup2(int oldfd, int newfd){
	return (int) syscall_2(SYS_dup, (uint64_t) oldfd, (uint64_t) newfd);
}
