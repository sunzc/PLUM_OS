#include <stdlib.h>
#include <syscall.h>

typedef long time_t;

struct timespec{
	time_t tv_sec;
	long tv_nsec;
};

pid_t fork(){
	return (pid_t)syscall_0(SYS_fork);
}

pid_t getpid(void){
	return (pid_t) syscall_0(SYS_getpid);
}

pid_t getppid(void){
	return (pid_t) syscall_0(SYS_getppid);
}

int execve(const char *filename, char *const argv[], char *const envp[]){
	return (int) syscall_3(SYS_execve, (uint64_t) filename, (uint64_t) argv, (uint64_t) envp);
}

pid_t waitpid(pid_t pid, int *status, int options){
	return (pid_t) syscall_4(SYS_wait4, (uint64_t) pid, (uint64_t) status, (uint64_t) options, (uint64_t) NULL);
}

unsigned int sleep(unsigned int seconds){
	unsigned int res;
	struct timespec ts = {.tv_sec = (long int) seconds, .tv_nsec = 0};
	res = syscall_2(SYS_nanosleep, (uint64_t) &ts, (uint64_t) &ts);
	if(res) res = (unsigned int) ts.tv_sec + (ts.tv_nsec >= 500000000L);
	return res;
}
