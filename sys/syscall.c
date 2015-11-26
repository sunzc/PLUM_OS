#include <sys/sbunix.h>
#include <sys/syscall.h>

//#define DEBUG_SYSCALL

const uint32_t CPUID_FLAG_MSR = 1 << 5;

typedef struct sc_frame {
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
}sc_frame;

extern void __syscall_handler(void);
uint64_t syscall_handler(int syscall_num, sc_frame *sf);
uint64_t sys_null(int num);
uint64_t read(sc_frame *);
uint64_t write(sc_frame *);

void cpuid(int code, uint32_t *a, uint32_t *d) {
	__asm volatile("cpuid":"=a"(*a), "=d"(*d):"a"(code):"ecx","ebx");
}

uint32_t cpu_has_msr() {
	uint32_t a, d;	//eax, edx
	cpuid(1, &a, &d);
	return d & CPUID_FLAG_MSR;
}

void cpu_get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi) {
	__asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void cpu_set_msr(uint32_t msr, uint32_t lo, uint32_t hi) {
	__asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void init_syscall() {
	uint32_t lo, hi, msr;
	

	printf("[init_syscall]\n");

	/* first test if the processor support syscall/sysret or not */
	msr = cpu_has_msr();
	printf("[init_syscall]msr:%d\n", msr);
	if (!msr)
		panic("cpu doesn't support MSR!");

	/* enable syscall by setting EFER :SysCallEnable bit */
	cpu_get_msr(IA32_EFER, &lo, &hi);
	printf("[init_syscall] get MSR:IA32_EFER lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);

	lo = lo | 1;
	cpu_set_msr(IA32_EFER, lo, hi);

	/* set MSR:IA32_LSTAR to be the address of syscall_handler*/
	lo = (uint32_t) (((uint64_t)__syscall_handler) & ((((uint64_t) 1) << 32) - 1));
	hi = (uint32_t) ((((uint64_t)__syscall_handler)& ~((((uint64_t) 1) << 32) - 1)) >> 32);

#ifdef DEBUG_SYSCALL
	printf("[init_syscall] set MSR:IA32_LSTAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif

	cpu_set_msr(IA32_LSTAR, lo, hi);


#ifdef DEBUG_SYSCALL
	lo = 0;
	hi = 0;
	cpu_get_msr(IA32_LSTAR, &lo, &hi);
	printf("[init_syscall] get MSR:IA32_LSTAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif

	/**
	 * set MSR:IA32_STAR, so that:
	 * for syscall:
	 * 	CS.Selector = gdt[1], equals IA32_STAR[47:32]
	 * 	SS.Selector = gdt[2], equals IA32_STAR[47:32] + 8
	 * for sysret:
	 * 	CS.Selector = gdt[8], equals IA32_STAR[63:48] + 16
	 * 	SS.Selector = gdt[7], equals IA32_STAR[63:48] + 8
	 * which means 
	 *  IA32_STAR[47:32] = 0x8(KERNEL MODE);
	 *  IA32_STAR[63:48] = 0x30 + 0x3(USER MODE);
	 */

	lo = 0;	// IA32_STAR[0:31] : reserved
	hi = (0x33 << 16) | 0x8;	// IA32_STAR[32:63]:SYSRET CS&SS| SYSCALL CS&SS

#ifdef DEBUG_SYSCALL
	printf("[init_syscall] set MSR:IA32_STAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif

	cpu_set_msr(IA32_STAR, lo, hi);

#ifdef DEBUG_SYSCALL
	lo = 0;
	hi = 0;
	cpu_get_msr(IA32_STAR, &lo, &hi);
	printf("[init_syscall] get MSR:IA32_STAR lo:0x%x hi:0x%x, lstar:0x%x%x\n",lo,hi,hi,lo);
#endif
}

uint64_t syscall_handler(int syscall_num, sc_frame *sf) {
	uint64_t res;
	switch(syscall_num) {
		case SYS_read: 
			res = read(sf);
			break;
		case SYS_write: 
			res = write(sf);
			break;
		default:
			res = sys_null(syscall_num);
			break;
	}

	return res;
}


uint64_t read(sc_frame *sf) {
	int res = 0;
	printf("syscall read!\n");

	return res;
}

uint64_t write(sc_frame *sf) {
	int res = 0;
	char *buf = (char *)(sf->rsi);

#ifdef DEBUG_SYSCALL	
	uint32_t fd = (uint32_t)(sf->rdi);
	uint64_t size = sf->rdx;
	printf("[debug]syscall write! fd:%d, buf:0x%lx, size:0x%lx\n", fd, buf, size);
#endif
	printf("%c",*buf);
	return res;
}

uint64_t sys_null(int num) {
	int res = 0;
	printf("syscall num = %d, not supportted yet!\n", num);

	return res;
}
