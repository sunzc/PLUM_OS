#
# entry.s
#

.text
##########
# __ret_to_user
# 	parameter 1: user stack
# 	parameter 2: user entry
#
.global __ret_to_user 
__ret_to_user:
	mov $0x23,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs

	pushq $0x23 #user data segment with bottom bits set for ring3
	pushq %rdi
	pushfq
	pushq $0x1B # user code segment with bottom bits set for ring3
	pushq %rsi
	iretq

##############
# __syscall_handler
# syscall_handler(int syscall_num, struct stack_frame *)
#
.global __syscall_handler
.extern current
__syscall_handler:
#TODO:we should switch stack here
	pushq %r12
	movq current,%r12
	movq %rsp,0x20(%r12)	# store rsp into current->user_stack
	movq 0x18(%r12),%rsp	# load rsp from current->kernel_stack

	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rsi
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rbp

	movq %rax,%rdi
	movq %rsp,%rsi
	call syscall_handler 
//	sti
	popq %rbp
	popq %r15
	popq %r14
	popq %r13
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rdi
	popq %rsi
	popq %rdx
	popq %rcx
	popq %rbx

	movq %rsp,0x18(%r12)	# store rsp into current->kernel_stack
	movq 0x20(%r12),%rsp	# load rsp from current->user_stack
	popq %r12
	sysretq	

.global ret_from_fork
ret_from_fork:
	popq %rbp
	popq %r15
	popq %r14
	popq %r13
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rdi
	popq %rsi
	popq %rdx
	popq %rcx
	popq %rbx

	movq %rsp,0x18(%r12)	# store rsp into current->kernel_stack
	movq 0x20(%r12),%rsp	# load rsp from current->user_stack
	popq %r12
	movq $0x0,%rax
	sysretq
