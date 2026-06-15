
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

struct gatedesc64 idt[256];
extern u64 vectors[];
struct spinlock tickslock;
u32 ticks;

void
tvinit(void)
{
	for (int i = 0; i < 256; i++) {
		SETGATE64(idt[i], 0, 0, SEG_KCODE << 3, vectors[i], 0);
	}
	initlock(&tickslock, "time");
}

void
idtinit(void)
{
	lidt(idt, sizeof(idt));
}

void
trap(struct trapframe *tf)
{
	switch (tf->trapno) {
	case T_IRQ0 + IRQ_TIMER:
		++ticks;
		lapiceoi();
		break;
	case T_IRQ0 + IRQ_KBD:
		kbdintr();
		lapiceoi();
		break;
	default:
		cprintf("unexpected trap %d from cpu %d rip %p (cr2=0x%p)\n", tf->trapno, cpuid(), tf->rip, rcr2());
		panic("trap");
	}

	// if process killed and in userspace, exit.
	if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER) {
		exit();
	}

	if (myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER && !(ticks % 1)) {
		// cprintf("timeryield\n");
		yield();
	}

	// process could have been killed when yielded
	if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER) {
		exit();
	}

	return;
}
