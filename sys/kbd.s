/* filename: isr_wrapper_timer.s */
.global _isr_wrapper_kbd
.align 4

_isr_wrapper_kbd:
	pushq %rbp
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rsi
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	cld /*  C code following the sysV ABI requires DF to be clear on function entry */
	call kbdintr
	sti
	popq %r15 
	popq %r14 
	popq %r13 
	popq %r12 
	popq %r11 
	popq %r10 
	popq %r9
	popq %r8
	popq %rdi
	popq %rsi
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	popq %rbp
	iretq
