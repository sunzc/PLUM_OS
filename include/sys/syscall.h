#ifndef __SYS_SYSCALL_H
#define __SYS_SYSCALL_H

#define SYS_exit       60
#define SYS_brk        12
#define SYS_fork       57
#define SYS_getpid     39
#define SYS_getppid   110
#define SYS_execve     59
#define SYS_wait4      61
#define SYS_nanosleep  35
#define SYS_alarm      37
#define SYS_kill       62
#define SYS_getcwd     79
#define SYS_chdir      80
#define SYS_open        2
#define SYS_read        0
#define SYS_write       1
#define SYS_lseek       8
#define SYS_close       3
#define SYS_pipe       22
#define SYS_dup        32
#define SYS_dup2       33
#define SYS_getdents   78
#define SYS_ps	       300
#define SYS_rt_sigaction	13
#define SYS_rt_sigreturn	15

#define IA32_EFER		0xC0000080
#define IA32_STAR		0xC0000081
#define IA32_LSTAR		0xC0000082
#define IA32_FMASK		0xC0000084

void init_syscall(void);
#endif
