#include "syscall.h"
.globl start
.globl main
start:
xor %rbp, %rbp

call main
mov %rax, %rdi # not quite relevant just yet
mov $SYS_exit, %rax
syscall
hang: jmp hang # hopefully dead code

.globl prnt
prnt:
mov %rcx, %r10
mov $SYS_debugprnt, %rax
syscall
ret
