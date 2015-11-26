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
__syscall_handler:
	pushq %rcx
	pushq %rdx
	pushq %rsi
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11

	movq %rax,%rdi
	movq %rsp,%rsi
	call syscall_handler 
//	sti
	popq %r11 
	popq %r10 
	popq %r9 
	popq %r8 
	popq %rdi
	popq %rsi
	popq %rdx
	popq %rcx
	sysretq	

