#include "syscall.h"
#include "param.h"
#include "mmu.h"

.extern syscalls

.globl syscallasm_entry
syscallasm_entry:

# TODO IST for NMI's etc so we don't die here.

# switch stacks, mycpu.cur_ustack, mycpu.cur_kstack
swapgs
movq %rsp, %gs:0x10
movq %gs:0x08, %rsp
addq $KSTACKSIZE, %rsp 	# this is SUPER UGLY...
			# it very much works now
			# but if we want nestable stack
			# we'd want to have it point to top from getgo

# see how the frame is equal to mycpu.cur_kstack?
# i wrote that comment a bit ago and have no clue what i meant sry.
#pushq %r11
#pushq %rcx

// we need to construct a trapframe for fork to work
// can this be replaced with something better?

pushq $(SEG_UDATA << 3 | DPL_USER)
#pushq %rsp
pushq %gs:0x10
pushq %r11
pushq $(SEG_UCODE << 3 | DPL_USER)
pushq %rcx
pushq $0
pushq $0
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

# mycpu()->proc->tf = %rsp
# i.e. *((*(gs:0x18))+0) = %rsp
# i.e. *(gs:0x18) = %rsp
movq %gs:0x18, %rcx
movq %rsp, (%rcx)

movq %r10, %rcx # this will be mirrored in libc

cmpq $NSYSCALL, %rax
jae bad_no 

movabs $syscalls, %rbx
call *(%rbx, %rax, 8)

jmp ok

bad_no:

mov $-1, %rax

ok:

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
#popq %rax
addq $0x40, %rsp # low trapframe struct fields, fake cpugen

#popq %rcx
#popq %r11

# switch stacks back
movq %gs:0x10, %rsp
swapgs

sysretq
