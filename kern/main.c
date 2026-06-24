#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "string.h"

static void startothers(void);
static void mpmain(void) __attribute__((noreturn));
void mocksched(void);
extern pde_t *kpgdir;
extern char end[]; // first address after kernel loaded from ELF file

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int
main(void)
{
	mpinit();
	lapicinit();
	seginit();
	picinit();
	ioapicinit();
	consoleinit();
	cprintf("lapicid %d firstcpu\n", lapicid());
	kinit1(P2CM(V2P(end)), P2CM(PHYSTOP));
	kvmalloc();
	tvinit();
	idtinit();
	fileinit();
	startothers();
	kinit2(NULL, NULL);
	userinit();
	mpmain();
}

static void
mpenter(void)
{
	switchkvm();
	seginit();
	lapicinit();
	mpmain();
}

static void
mpmain(void)
{
	cprintf("cpu%d start\n", cpuid());
	idtinit();
	xchg(&(mycpu()->started), 1);
	scheduler();
}

static void
startothers(void)
{
	extern u8 _binary_entryother_start[], _binary_entryother_size[], initpml4[], pgdir32[], gdtr64[];
	u8 *code;
	struct cpu *c;
	char *stack;

	code = P2CM(0x7000);
	memmove(code, _binary_entryother_start, (u64)_binary_entryother_size);

	for (c = cpus; c < cpus + ncpu; ++c) {
		if (c == mycpu()) {
			continue;
		}

		stack = kalloc();
		*(u64 *)(code - 8) = (u64)(stack + KSTACKSIZE);
		*(u64 *)(code - 16) = (u64)mpenter;
		*(u64 *)(code - 24) = (u64)V2P(initpml4);
		*(u64 *)(code - 32) = (u64)V2P(pgdir32);
		*(u64 *)(code - 40) = (u64)V2P(gdtr64);

		lapicstartap(c->apicid, CM2P(code));

		while (!c->started) {
			asm("pause");
		}
	}
}
