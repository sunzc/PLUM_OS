/* filename: switch_to.s */
.global __switch_to
.align 4

/**
 * __switch_to(me, next, &last)
 * rax: me
 * rbx: next
 * rcx: &last
 */
__switch_to:
	pushq %rbx
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
	pushq %rflags
	pushq %rax /* ptr to me on mystack */
	pushq %rcx /* ptr to local last (&last) */

	movq %rsp, %rax(24) /* save my stack ptr */
	movq %rbx(24), %rsp /* swtich to next stack */

	popq %rcx /* get next's ptr to &last */
	movq %rax, (%rcx) /* store rax in &last */
	popq %rax /* update me (rax) to new stack */
	popq %rflags
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
	popq %rbx
	retq
