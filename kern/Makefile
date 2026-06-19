STRING := $(patsubst string/%.c, string/%.o, $(wildcard string/*.c))
OBJS = \
       console.o\
       ioapic.o\
       kalloc.o\
       kbd.o\
       lapic.o\
       main.o\
       mp.o\
       picirq.o\
       proc.o\
       spinlock.o\
       $(STRING)\
       swtch.o\
       syscall.o\
       syscallasm.o\
       sysproc.o\
       trapasm.o\
       trap.o\
       vectors.o\
       vm.o\

# Cross-compiling (e.g., on Mac OS X)
# TOOLPREFIX = i386-jos-elf

# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX = 

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your TOOLPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	elif which qemu-system-i386 > /dev/null; \
	then echo qemu-system-i386; exit; \
	elif which qemu-system-x86_64 > /dev/null; \
	then echo qemu-system-x86_64; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

include ../config.mk
CFLAGS+= -mcmodel=large -fno-omit-frame-pointer
BOOTCFLAGS= --target=x86_64-elf -m32 -fno-pic -static -fno-builtin -fno-strict-aliasing -Oz -Wall -ggdb -MD -mno-sse -fno-omit-frame-pointer -fno-stack-protector

xv6.img: bootblock kernel
	dd if=/dev/zero of=xv6.img count=10000
	dd if=bootblock of=xv6.img conv=notrunc
	dd if=kernel of=xv6.img seek=1 conv=notrunc

bootblock: bootasm.S bootmain.c
	$(CC) $(BOOTCFLAGS) -I. -c bootmain.c
	$(CC) $(BOOTCFLAGS) -I. -c bootasm.S
	$(LD) $(LDFLAGS) -N -e start -T bootblock.ld -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary bootblock.o bootblock

entryother: entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c entryother.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 --image-base 0x0 -o bootblockother.o entryother.o
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
	$(OBJDUMP) -S bootblockother.o > entryother.asm

initcode: initcode.c initrt.S
	$(CC) $(CFLAGS) -nostdinc -c initcode.c
	$(CC) $(CFLAGS) -nostdinc -c initrt.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 --image-base 0x0 -o initcode.out initcode.o initrt.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

kernel: $(OBJS) entry.o  kernel.ld entryother initcode
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel entry.o $(OBJS) -b binary entryother initcode
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

vectors.S: vectors
	./vectors > vectors.S

-include *.d

clean: 
	rm -f *.o *.d *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out kernel kernelmemfs xv6.img xv6memfs.img $(OBJS)
