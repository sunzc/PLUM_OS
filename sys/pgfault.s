/** filename: isr_wrapper_timer.s 
 *	page_fault_handler(pgfault_addr, error_code, eip)
 *
 */
.global _page_fault_handler
.align 4

_page_fault_handler:
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

	movq %cr2,%rdi		#/* cr2 contain the address that cause page fault */
	movq 0x70(%rsp),%rsi	#/* 0x70(%rsp) points to the error code pushed on stack before %rax */
	movq 0x78(%rsp),%rdx		#/* exception ip for debug */
	call page_fault_handler

//	sti
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
	addq $0x8,%rsp		# pop error code
	iretq
