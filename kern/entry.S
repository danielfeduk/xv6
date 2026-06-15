# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.
        
# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"

# Multiboot header.  Data to direct multiboot loader.
.p2align 2
.text
#.globl multiboot_header
#multiboot_header:
#  # define magic 0x1badb002
#  # define flags 0
#  .long magic
#  .long flags
#  .long (-magic-flags)


.section .mb2
mb2_header:
#define MAGIC	0xE85250D6
#define ARCH	0x0
#define HDLEN	(mb2_header_end-mb2_header)
.long MAGIC
.long ARCH
.long HDLEN
.long -(MAGIC+ARCH+HDLEN)
.short 0
.short 0
.long 8
mb2_header_end:

.section .text
# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
_start = V2P_WO(entry)

.code32
# Entering xv6 on boot processor, with paging off.
.globl entry
entry:
 # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4
  # Set page directory
  movl    $(V2P_WO(pgdir32)), %eax
  movl    %eax, %cr3
  # Turn on paging.
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Make it 64 bit
  mov %cr0, %eax
  and $(~CR0_PG), %eax
  mov %eax, %cr0
  
  xor %eax, %eax
  mov $(V2P_WO(initpml4)), %eax
  mov %eax, %cr3
  mov $MSR_EFER, %ecx
  rdmsr
  or $(EFER_LME | EFER_NXE | EFER_SCE), %eax
  wrmsr
  mov %cr4, %eax
  or $(CR4_PAE | CR4_PGE), %eax
  mov %eax, %cr4
  mov %cr0, %eax
  or $(CR0_PG), %eax
  mov %eax, %cr0

  .globl _gdtr64
  mov $V2P_WO(gdtr64), %eax
  lgdt (%eax)
  ljmp $0x08, $V2P_WO(yonder)
  .code64
  yonder:
  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %ss


  # Set up the stack pointer.
  movabs $(stack + KSTACKSIZE), %rsp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  movabs $main, %rax
  jmp *%rax

.section .rodata

.globl initpml4
.globl initpdpt
.globl pgdir32

.align PGSIZE
pgdir32:
.long (0 + PTE_P + PTE_W + PTE_PS)
.zero (511 * 4)

.align PGSIZE
initpml4:
.quad (V2P_WO(initpdpt) + PTE_P + PTE_W)
.zero (512 - 4) * 8
.quad (V2P_WO(initpdpt) + PTE_P + PTE_W) # core iden map, permanent.
.quad 0
.quad (V2P_WO(initpdpt) + PTE_P + PTE_W) # temporarily iden map all kernel space

.align PGSIZE
initpdpt:
.set i, 0
.rept 512
    .quad (i * 0x40000000) + PTE_P + PTE_W + PTE_PS
    .set i, i + 1
.endr

.globl gdtr64
gdtr64:
.word gdtend64 - gdt64 - 1
.quad V2P_WO(gdt64)

gdt64:
.word 0x0000
.word 0x0000
.word 0x0000
.word 0x0000

.word 0xffff
.word 0x0000
.byte 0x00
.byte 0x9a // EXEC
.byte 0xaf
.byte 0x00

.word 0xffff
.word 0x0000
.byte 0x00
.byte 0x92  // DATA
.byte 0xaf
.byte 0x00

gdtend64:

.comm stack, KSTACKSIZE
