#include "mmu.h"

  # vectors.S sends all traps here.
.globl alltraps
alltraps:

	pushq %rax
	pushq %rcx
	pushq %rdx
	pushq %rbx
	pushq %rbp
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

	# what is swapgs
	# what is xsave

	movq %rsp, %rdi
	call trap

.globl trapret
trapret:

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
	popq %rbp
	popq %rbx
	popq %rdx
	popq %rcx
	popq %rax
	addq $0x10, %rsp # trapno and errcode
	iretq
