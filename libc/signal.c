#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>

/**
  * a lot of borrowing and grabbing from glibc/ to finally offer a simple but workable signal mechanism.
  */
typedef unsigned long int sigset_t;
/* Using the hidden attribute here does not change the code but it
 * helps to avoid warnings.  */
extern void restore_rt (void) __asm__ ("__restore_rt");


/* We do not globally define the SA_RESTORER flag so do it here.  */
#define SA_RESTORER 0x04000000
/* NOTE: Please think twice before making any changes to the bits of
   code below.  GDB needs some intimate knowledge about it to
   recognize them as signal trampolines, and make backtraces through
   signal handlers work right.  Important are both the names
   (__restore_rt) and the exact instruction sequence.
   If you ever feel the need to make any changes, please notify the
   appropriate GDB maintainer.  */


struct kernel_sigaction {
	__sighandler_t k_sa_handler;
	unsigned long sa_flags;
	void (*sa_restorer) (void);
	sigset_t sa_mask;
};

sighandler_t signal(int signum, sighandler_t handler){
	int result;

	struct kernel_sigaction sa, osa;
	sa.k_sa_handler = handler;
	sa.sa_mask = 0;
	sa.sa_flags = SA_RESTORER;
	sa.sa_restorer = &restore_rt;

	result = (int) syscall_4(SYS_rt_sigaction, (uint64_t) signum, (uint64_t) &sa, (uint64_t) &osa, (uint64_t)sizeof(sigset_t));

	if(result >= 0)
		return osa.k_sa_handler;
	else
		return SIGERR;
}

int kill(pid_t pid, int sig){
	return (int)syscall_2(SYS_kill, (uint64_t) pid, (uint64_t) sig);
}

int get_sig(char *str) {
	if (str[0] == '-')
		return atoi(str + 1);
	else {
		printf("ERROR: wrong format of signal number parameter!\n Please user kill -9 pid\n");
		exit(1);

		return -1;
	}
}

unsigned int alarm(unsigned int seconds){
	return (uint32_t)syscall_1(SYS_alarm, (uint64_t) seconds);
}

#define RESTORE(name, syscall) RESTORE2 (name, syscall)
# define RESTORE2(name, syscall) \
__asm__						\
  (						\
   ".text\n" \
   ".align 16\n"				\
   "__" #name ":\n"				\
   "	movq $" #syscall ", %rax\n"		\
   "	syscall\n"				\
   );

RESTORE (restore_rt, SYS_rt_sigreturn)
