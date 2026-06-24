
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
		if(cpuid() == 0) {
			acquire(&tickslock);
			++ticks;
			wakeup(&ticks);
			release(&tickslock);
		}
		lapiceoi();
		break;
	case T_IRQ0 + IRQ_KBD:
		kbdintr();
		lapiceoi();
		break;
	case T_IRQ0 + IRQ_COM1:
		uartintr();
		lapiceoi();
		break;
	default:
		if(myproc() == 0 || (tf->cs & 3) == 0) {
			cprintf("unexpected trap %d err %d from cpu %d rip %p (cr2=0x%p)\n", tf->trapno, tf->errcode, cpuid(), tf->rip, rcr2());
			panic("trap");
		} else {
			cprintf("pid %d %s: trap %d err %d on cpu %d "
				"rip 0x%x addr 0x%x--kill proc\n",
				myproc()->pid, myproc()->name, tf->trapno,
				tf->errcode, cpuid(), tf->rip, rcr2());
			myproc()->killed = 1;
		}
	}

	// if process killed and in userspace, exit.
	if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER) {
		exit(128);
	}

	if (myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER && !(ticks % 1)) {
		// cprintf("timeryield\n");
		yield();
	}

	// process could have been killed when yielded
	if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER) {
		exit(128);
	}

	return;
}
