# Initial process execs /init.
# This code runs in user space.

#include "syscall.h"
#include "traps.h"


.globl start
start: 
mov $SYS_fork, %rax
syscall
cmp $0, %rax
jnz loorp
mov $message, %rdi
mov $SYS_debugprnt, %rax
syscall

mov $1000, %rdi
mov $SYS_sleep, %rax
syscall

#mov $1, %rdi
#mov $SYS_kill, %rax
#syscall
mov $SYS_exit, %rax
syscall
loorp: 
mov $SYS_wait, %rax
syscall
mov %rax, %rdi
mov $SYS_debugprnt2, %rax
syscall

/*
mov $0x1000, %rsp
mov $SYS_fork, %rax
syscall
mov %rax, %rdi
pushq %rax
mov $SYS_debugprnt2, %rax
syscall
popq %rax
cmp $0, %rax
jnz else
mov $SYS_debugprnt, %rax
mov $message, %rdi
syscall
jmp start
else:
mov $SYS_getpid, %rax
syscall
cmp $1, %rax
je loop
mov $SYS_exit, %rax
syscall
loop: jmp loop
/*
mov $SYS_fork, %rax
syscall
cmp $-1, %rax
je panic
pause
jmp start
panic:
mov $message, %rdi
mov $SYS_debugprnt, %rax
syscall
halt:
pause
jmp halt
*/
/*
xor %rax, %rax
loop:
	pause
	inc %rax
	cmpq $0x100000, %rax
	jae out
	jmp loop
out:
mov $message, %rdi
mov $SYS_debugprnt, %rax
syscall
#jmp start

mov $0, %rdi
mov $SYS_kill, %rax
syscall
*/

# fork
/*
mov $SYS_fork, %rax
syscall
##if !rax && rbx<4, inc rbx, jump top.
##else spin
cmp $0, %rax
jnz spin
#cmp $6, %rbx
#jg spin
inc %rbx
jmp start
spin:
jmp spin
*/
#mov $message, %rdi
#mov $SYS_debugprnt, %rax
#syscall

#mov $SYS_exit, %rax
#syscall

hang:
jmp hang
  pushq $argv
  pushq $init
  pushq $0  // where caller pc would be
  movabs $0xfffffe8000000000, %rax
  movq $7, (%rax)
  #movl $SYS_exec, %eax
  int $T_SYSCALL

zero:
mov $SYS_exit, %rax
syscall

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

message:
.asciz "Hiii"
message2:
.asciz "Hii2"
