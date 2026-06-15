# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

.globl swtch
swtch:
	movq %rdi, %rax # struct context **old
	movq %rsi, %rdx # struct context *new

	# save callee saved regs
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %rsi
	pushq %rdi
	pushq %rbp
	pushq %rbx

	# switch stacks
	movq %rsp, (%rax)
	movq %rdx, %rsp

	# load callee saved regs
	popq %rbx
	popq %rbp
	popq %rdi
	popq %rsi
	popq %r12
	popq %r13
	popq %r14
	popq %r15
	
	ret
