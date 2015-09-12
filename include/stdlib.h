#ifndef _STDLIB_H
#define _STDLIB_H

#include <sys/defs.h>
#include <sys/dirent.h>

extern __thread int errno;

void exit(int status);

// memory
void *malloc(size_t size);
void free(void *ptr);
int brk(void *end_data_segment);

// processes
pid_t fork(void);
pid_t getpid(void);
pid_t getppid(void);
int execve(const char *filename, char *const argv[], char *const envp[]);
pid_t waitpid(pid_t pid, int *status, int options);
unsigned int sleep(unsigned int seconds);

// signals
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
int kill(pid_t pid, int sig);
unsigned int alarm(unsigned int seconds);
#define SIG_DFL ((__sighandler_t)0)
#define SIG_IGN ((__sighandler_t)1)
#define SIGINT     2
#define SIGKILL    9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGALRM   14
#define SIGTERM   15
#define SIGCHLD   17

// paths
char *getcwd(char *buf, size_t size);
int chdir(const char *path);

// files
typedef int64_t ssize_t;
enum { O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2, O_CREAT = 0x40, O_DIRECTORY = 0x10000 };
int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
enum { SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2 };
off_t lseek(int fildes, off_t offset, int whence);
int close(int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);

// directories
void *opendir(const char *name);
struct dirent *readdir(void *dir);
int closedir(void *dir);

#endif
