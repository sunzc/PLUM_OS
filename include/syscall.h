#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <sys/defs.h>
#include <sys/syscall.h>

#define INTERNAL_SYSCALL0(name)					\
  ({								\
    unsigned long resultvar;					\
    __asm volatile (						\
    "syscall\n\t"						\
    : "=a" (resultvar)						\
    : "0" (name) : "memory", "cc", "r11", "cx");		\
    (long) resultvar; })


static __inline int64_t syscall_0(uint64_t n) {
	uint64_t _resultvar = INTERNAL_SYSCALL0 (n);
	return (int64_t) _resultvar; 
}

#define INTERNAL_SYSCALL1(name, a1)				\
  ({								\
    unsigned long resultvar;					\
    long int __arg1 = (long) (a1);				\
    register long int _a1 __asm ("rdi") = __arg1;		\
    __asm volatile (						\
    "syscall\n\t"						\
    : "=a" (resultvar)						\
    : "0" (name) , "r" (_a1) : "memory", "cc", "r11", "cx");	\
    (long) resultvar; })

static __inline int64_t syscall_1(uint64_t n, uint64_t a1) {
	uint64_t _resultvar = INTERNAL_SYSCALL1 (n, a1);
	return (int64_t) _resultvar; 
}

#define INTERNAL_SYSCALL2(name, a1, a2)				\
  ({								\
    unsigned long resultvar;					\
    long int __arg1 = (long) (a1);				\
    long int __arg2 = (long) (a2);				\
    register long int _a1 __asm ("rdi") = __arg1;		\
    register long int _a2 __asm ("rsi") = __arg2;		\
    __asm volatile (						\
    "syscall\n\t"						\
    : "=a" (resultvar)						\
    : "0" (name) , "r" (_a1), "r" (_a2): "memory", "cc", "r11", "cx");	\
    (long) resultvar; })

static __inline int64_t syscall_2(uint64_t n, uint64_t a1, uint64_t a2) {
	uint64_t _resultvar = INTERNAL_SYSCALL2 (n, a1, a2);
	return (int64_t) _resultvar; 
}

#define INTERNAL_SYSCALL3(name, a1, a2, a3)			\
  ({								\
    unsigned long resultvar;					\
    long int __arg1 = (long) (a1);				\
    long int __arg2 = (long) (a2);				\
    long int __arg3 = (long) (a3);				\
    register long int _a1 __asm ("rdi") = __arg1;		\
    register long int _a2 __asm ("rsi") = __arg2;		\
    register long int _a3 __asm ("rdx") = __arg3;		\
    __asm volatile (						\
    "syscall\n\t"						\
    : "=a" (resultvar)						\
    : "0" (name) , "r" (_a1), "r" (_a2), "r" (_a3) : "memory", "cc", "r11", "cx");	\
    (long) resultvar; })

static __inline int64_t syscall_3(uint64_t n, uint64_t a1, uint64_t a2, uint64_t a3) {
	uint64_t _resultvar = INTERNAL_SYSCALL3 (n, a1, a2, a3);
	return (int64_t) _resultvar; 
}

#define INTERNAL_SYSCALL4(name, a1, a2, a3, a4)			\
  ({								\
    unsigned long resultvar;					\
    long int __arg1 = (long) (a1);				\
    long int __arg2 = (long) (a2);				\
    long int __arg3 = (long) (a3);				\
    long int __arg4 = (long) (a4);				\
    register long int _a1 __asm ("rdi") = __arg1;		\
    register long int _a2 __asm ("rsi") = __arg2;		\
    register long int _a3 __asm ("rdx") = __arg3;		\
    register long int _a4 __asm ("r10") = __arg4;		\
    __asm volatile (						\
    "syscall\n\t"						\
    : "=a" (resultvar)						\
    : "0" (name) , "r" (_a1), "r" (_a2), "r" (_a3), "r" (_a4) : "memory", "cc", "r11", "cx");	\
    (long) resultvar; })

static __inline int64_t syscall_4(uint64_t n, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
	uint64_t _resultvar = INTERNAL_SYSCALL4 (n, a1, a2, a3, a4);
	return (int64_t) _resultvar; 
}

#define INTERNAL_SYSCALL5(name, a1, a2, a3, a4, a5)			\
  ({								\
    unsigned long resultvar;					\
    long int __arg1 = (long) (a1);				\
    long int __arg2 = (long) (a2);				\
    long int __arg3 = (long) (a3);				\
    long int __arg4 = (long) (a4);				\
    long int __arg5 = (long) (a5);				\
    register long int _a1 __asm ("rdi") = __arg1;		\
    register long int _a2 __asm ("rsi") = __arg2;		\
    register long int _a3 __asm ("rdx") = __arg3;		\
    register long int _a4 __asm ("r10") = __arg4;		\
    register long int _a5 __asm ("r8") = __arg5;		\
    __asm volatile (						\
    "syscall\n\t"						\
    : "=a" (resultvar)						\
    : "0" (name) , "r" (_a1), "r" (_a2), "r" (_a3), "r" (_a4), "r" (_a5) : "memory", "cc", "r11", "cx");	\
    (long) resultvar; })

static __inline int64_t syscall_5(uint64_t n, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
	uint64_t _resultvar = INTERNAL_SYSCALL5 (n, a1, a2, a3, a4, a5);
	return (int64_t) _resultvar; 
}

#endif
