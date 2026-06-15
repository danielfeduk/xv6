#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

enum plevel { PML4E, PDPTE, PDE, PTE };

struct pgent {
	enum plevel lvl;
	pte_t *va;
};

extern char data[], end[]; // segment; see kernel.ld
pde_t *kpgdir;

void *kstack_user;

void
seginit(void)
{
	struct cpu *c;
	extern char syscallasm_entry[];

	int apicid = lapicid();
	for (c = cpus; c < &cpus[NCPU]; ++c) {
		if (c->apicid == apicid)
			break;
	}
	wrmsr(MSR_GSBASE, (u64)c);
	wrmsr(MSR_KERNGSBASE, (u64)c);

	// c = &cpus[cpuid()];
	c->gdt.seg[SEG_KCODE] = SEG(STA_X | STA_R, 1, 0, 0, 0);
	c->gdt.seg[SEG_KDATA] = SEG(STA_W, 0, 0, 0, 0);
	c->gdt.seg[SEG_UDATA] = SEG(STA_W, 0, 0, 0, DPL_USER);
	c->gdt.seg[SEG_UCODE] = SEG(STA_X | STA_R, 1, 0, 0, DPL_USER);

	lgdt(&c->gdt, sizeof(c->gdt));

	// set up syscall handler
	wrmsr(MSR_STAR, 0 | (u64)(SEG_KCODE << 3) << 32 | (u64)(SEG_KDATA << 3) << 48);
	wrmsr(MSR_LSTAR, (u64)syscallasm_entry);
	wrmsr(MSR_SFMASK, 0xFFFFFFFFFFFFFFFF);
}

// it could be written better
static pde_t *
walkpgdir_help(pde_t *pd, int indx, bool alloc)
{
	pde_t *lowerpd, *pde;

	if (!pd)
		return 0;
	pde = &pd[indx];

	if (*pde & PTE_P) {
		lowerpd = (pde_t *)P2CM(PTE_ADDR(*pde));
	} else {
		if (!alloc || (lowerpd = (pde_t *)kalloc()) == 0) {
			return 0;
		}
		memset(lowerpd, 0, PGSIZE);
		*pde = CM2P(lowerpd) | PTE_P | PTE_W | PTE_U;
	}
	return lowerpd;
}

// uses pt twice!
#define PSP(pt, x) (pt && (pt[x] & (PTE_PS | PTE_P)) == (PTE_PS | PTE_P))

static struct pgent
findpt(pde_t *pml4, const void *va)
{
	pde_t *pdpt, *pd, *pt;

	pdpt = walkpgdir_help(pml4, PML4X(va), false);
	if (PSP(pdpt, PDPTX(va)))
		return (struct pgent) { PDPTE, &pdpt[PDPTX(va)] };
	pd = walkpgdir_help(pdpt, PDPTX(va), false);
	if (PSP(pd, PDX(va)))
		return (struct pgent) { PDE, &pd[PDX(va)] };
	pt = walkpgdir_help(pd, PDX(va), false);
	if (!pt)
		return (struct pgent) { 0 };
	return (struct pgent) { PTE, &pt[PTX(va)] };
}

static u64
vtrans(pde_t *pml4, const void *va)
{
	struct pgent pe;
	pe = findpt(pml4, va);
	switch (pe.lvl) {
	case PDPTE:
		return PTE_ADDR(*pe.va) + ((u64)va & 0x2fffffff);
	case PDE:
		return PTE_ADDR(*pe.va) + ((u64)va & 0x1ffff);
	case PTE:
		return PTE_ADDR(*pe.va) + ((u64)va & 0xfff);
	default:
		panic("what the fuck?");
	}
}

/* ret addr of lowest level pte for given va
 * optionally (if alloc == true) allocate intermediate page tables as needed */
static pte_t *
creatpt(pde_t *pml4, enum plevel lvl, const void *va)
{
	pde_t *pdpt, *pd, *pt;

	pdpt = walkpgdir_help(pml4, PML4X(va), true);
	if (PSP(pdpt, PDPTX(va)))
		panic("remap ps page pdpt");
	if (lvl == PDPTE)
		return &pdpt[PDPTX(va)];

	pd = walkpgdir_help(pdpt, PDPTX(va), true);
	if (PSP(pd, PDX(va)))
		panic("remap ps page pd");
	if (lvl == PDE)
		return &pd[PDX(va)];

	pt = walkpgdir_help(pd, PDX(va), true);
	if (!pt)
		return NULL;
	if (pt[PTX(va)] & PTE_P)
		panic("remap");
	return &pt[PTX(va)];
}

#define BIGPGSZ (NPTENTRIES * PGSIZE)

static int
mappages(pde_t *pgdir, void *va, u64 size, u64 pa, int perm)
{
	char *a, *last;
	pte_t *pte;

	a = (char *)PGROUNDDOWN((u64)va);
	last = (char *)PGROUNDDOWN(((u64)va) + size - 1);

	for (;;) {
		if ((u64)a % BIGPGSZ == 0 && (u64)(last - a) >= BIGPGSZ) {
			pde_t *pde;
			if ((pde = creatpt(pgdir, PDE, a)) == 0)
				return -1;
			*pde = pa | perm | PTE_PS | PTE_P;
			if (a == last)
				break;
			a += BIGPGSZ;
			pa += BIGPGSZ;
			continue;
		}
		if (pa >= COREIDENBASE)
			panic("wrong pa");
		if ((pte = creatpt(pgdir, PTE, a)) == 0) {
			return -1;
		}
		if ((u64)pte < COREIDENBASE)
			panic("non cim pte va");

		*pte = pa | perm | PTE_P;

		if (a == last) {
			break;
		}
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

static int
amappages(pde_t *pgdir, void *va, u64 size, int perm)
{
	char *a, *last;
	pte_t *pte;

	a = (char *)PGROUNDDOWN((u64)va);
	last = (char *)PGROUNDDOWN(((u64)va) + size - 1);

	for (;;) {
		u64 pa = CM2P(kalloc());
		if (pa == CM2P(0))
			return -1; // yes ugly conditional dontcare
		if (pa >= COREIDENBASE)
			panic("wrong pa");
		if ((pte = creatpt(pgdir, PTE, a)) == 0) {
			return -1;
		}
		if ((u64)pte < COREIDENBASE)
			panic("non cim pte va");
		if (*pte & PTE_P) {
			cprintf("remap on %p => %p\n", a, pa);
			cprintf("PTE contains %p\n", *pte);
			panic("remap");
		}

		*pte = pa | perm | PTE_P;

		if (a == last) {
			break;
		}
		a += PGSIZE;
	}
	return 0;
}

void
crossvm_write(pde_t *pgdir, void *fva, void *va, size_t sz)
{
	pte_t *pte;
	void *fc;
	u64 n;
	for (void *fa = fva; fa < fva + sz; fa += n, va += n) {
		// this is ugly but i am sleepy damn
		// basically start not necessarily pgalign
		// but go to next page unless end is before it
		n = PGROUNDUP((u64)fa + 1) - (u64)fa;
		if (fa + n > fva + sz)
			n = (u64)(fva + sz - fa);
		fc = P2CM(vtrans(pgdir, fa));
		memcpy(fc, va, n);
	}
}

/*
 * This is all a mess! We need a serious kmap flexible kmap system TODO
 */

static struct kmap {
	void *virt;
	enum { PHYMAP, KALLOC } type;
	u64 phys_start;
	u64 phys_end;
	int perm;
} kmap[] = { { (void *)KERNBASE, PHYMAP, 0, EXTMEM, PTE_W }, { (void *)KERNLINK, PHYMAP, V2P(KERNLINK), V2P(data), 0 },
	{ (void *)data, PHYMAP, V2P(data), V2P(end), PTE_W }, { (void *)COREIDENBASE, PHYMAP, 0, 0x0ffffffff, PTE_W },
	{ (void *)KSTACKBASE, KALLOC, KSTACKBASE, KSTACKTOP, PTE_W } };

pde_t *
setupkvm(void)
{
	pde_t *pgdir;
	struct kmap *k;

	if ((pgdir = (pde_t *)kalloc()) == 0) {
		return 0;
	}
	memset(pgdir, 0, PGSIZE);

	for (k = kmap; k < &kmap[NELEM(kmap)]; k++) {
		switch (k->type) {
		case PHYMAP:
			if (mappages(pgdir, k->virt, k->phys_end - k->phys_start, (u64)k->phys_start, k->perm) < 0) {
				// freevm(pgdir);
				return 0;
			}
			break;
		case KALLOC:
			if (amappages(pgdir, k->virt, k->phys_end - k->phys_start, k->perm) < 0) {
				// freevm(pgdir);
				return 0;
			}
			break;
		}
	}
	return pgdir;
}

void
kvmalloc(void)
{
	kpgdir = setupkvm();
	switchkvm();
}

void
switchkvm(void)
{
	lcr3(CM2P(kpgdir));
}

void
inituvm(pde_t *pgdir, char *init, u32 sz)
{
	char *mem;

	if (sz >= PGSIZE)
		panic("inituvm: more than a page");
	mem = kalloc();
	memset(mem, 0, PGSIZE);
	mappages(pgdir, 0, PGSIZE, CM2P(mem), PTE_W | PTE_U);
	memmove(mem, init, sz);
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t *
copyuvm(pde_t *pgdir, u32 sz)
{
	pde_t *d;
	pte_t *pte;
	u64 pa, i, flags;
	char *mem;

	if ((d = setupkvm()) == 0)
		return 0;
	for (i = 0; i < sz; i += PGSIZE) {
		pte_t *pte = findpt(pgdir, (void *)i).va;
		pa = PTE_ADDR(*pte);
		flags = PTE_FLAGS(*pte);
		if (flags & PTE_PS)
			panic("user pse");

		if ((mem = kalloc()) == 0)
			goto bad;
		memmove(mem, (char *)P2CM(pa), PGSIZE);
		if (mappages(d, (void *)i, PGSIZE, CM2P(mem), flags) < 0) {
			kfree(mem);
			goto bad;
		}
	}
	return d;

bad:
	// freevm(d);
	return 0;
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
	if (p == 0)
		panic("switchuvm: no process");
	if (p->kstack == 0)
		panic("switchuvm: no kstack");
	if (p->pgdir == 0)
		panic("switchuvm: no pgdir");

	pushcli();
	mycpu()->gdt.tss_seg = TSS64(TSS_AVL, &mycpu()->ts, sizeof(mycpu()->ts) - 1, 0);
	mycpu()->ts.rsp0 = (u64)p->kstack + KSTACKSIZE;
	// setting IOPL=0 in rflags *and* iopb beyond the tss segment limit
	// forbids I/O instructions (e.g., inb and outb) from user space
	// TODO check if this is the same on x86_64
	mycpu()->ts.iopb = (u16)0xFFFF;
	ltr(SEG_TSS << 3);
	lcr3(CM2P(p->pgdir)); // switch to process's address space
	popcli();
}
