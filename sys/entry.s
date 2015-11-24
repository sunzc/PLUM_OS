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
