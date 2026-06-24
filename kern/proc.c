#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
	struct spinlock lock;
	struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
	initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid(void)
{
	return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
	return ((PERCPU struct cpu *)(0))->this;
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
	struct cpu *c;
	struct proc *p;
	pushcli();
	c = mycpu();
	p = c->proc;
	popcli();
	return p;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void)
{
	struct proc *p;
	// char *sp;

	acquire(&ptable.lock);

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if (p->state == UNUSED)
			goto found;

	release(&ptable.lock);
	return 0;

found:
	p->state = EMBRYO;
	p->pid = nextpid++;

	release(&ptable.lock);

	/*
	// TODO kernel stack should be mapped elsewhere, extend kmaps
	// Allocate kernel stack.
	if((p->kstack = (char*)KSTACKBASE) == 0){
		p->state = UNUSED;
		return 0;
	}
	*/
	p->kstack = (char *)KSTACKBASE;
	/*
	sp = p->kstack + KSTACKSIZE;

	// Leave room for trap frame.
	sp -= sizeof *p->tf;
	p->tf = (struct trapframe*)sp;

	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= 8;
	*(u64*)sp = (u64)trapret;

	sp -= sizeof *p->context;
	p->context = (struct context*)sp;
	memset(p->context, 0, sizeof *p->context);
	p->context->rip = (u64)forkret;
	*/
	cprintf("new proc: %d\n", p->pid);

	return p;
}

// process pgdir needs to exist by now!
static void
setupproc(struct proc *p, struct trapframe *tf)
{
	char *sp;
	sp = p->kstack + KSTACKSIZE;
	struct context c = { .rip = (u64)forkret };
	u64 *ret;
	void *rv = trapret;

	if (!p->pgdir)
		panic("setupproc no pgdir");

	sp -= sizeof *p->tf;
	p->tf = (struct trapframe *)sp;
	sp -= 8;
	ret = (u64 *)sp;
	sp -= sizeof *p->context;

	p->context = (struct context *)sp;

	crossvm_write(p->pgdir, p->tf, tf, sizeof *p->tf);
	crossvm_write(p->pgdir, ret, &rv, sizeof(void *));
	crossvm_write(p->pgdir, p->context, &c, sizeof *p->context);
}

#define INITSTACKSZ 4096

// Set up first user process.
void
userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];

	const size_t sz = PGROUNDUP((u64)_binary_initcode_size) + INITSTACKSZ;

	p = allocproc();

	initproc = p;
	if ((p->pgdir = setupkvm()) == 0) {
		panic("userinit: out of memory?");
	}
	inituvm(p->pgdir, _binary_initcode_start, sz);

	struct trapframe tf = { .cs = (SEG_UCODE << 3) | DPL_USER, .ss = (SEG_UDATA << 3) | DPL_USER, .rflags = FL_IF, .rsp = sz, .rip = 0 };

	setupproc(p, &tf);

	p->sz = sz;

	strlcpy(p->name, "initcode", sizeof(p->name));
	// p->cwd = namei("/");

	// this assignment to p->state lets other cores
	// run this process. the acquire forces the above
	// writes to be visible, and the lock is also needed
	// because the assignment might not be atomic.
	acquire(&ptable.lock);

	p->state = RUNNABLE;

	release(&ptable.lock);
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
	int i, pid;
	struct proc *np;
	struct proc *curproc = myproc();

	// Allocate process.
	if ((np = allocproc()) == 0) {
		cprintf("forkfail!\n");
		return -1;
	}

	// Copy process state from proc.
	if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
		kfree(np->kstack);
		np->kstack = 0;
		np->state = UNUSED;
		cprintf("failcopyuvm!");
		return -1;
	}

	np->sz = curproc->sz;
	np->parent = curproc;

	struct trapframe ntf = *curproc->tf;
	ntf.rax = 0;
	setupproc(np, &ntf);

	/*
	for(i = 0; i < NOFILE; i++)
		if(curproc->ofile[i])
			np->ofile[i] = filedup(curproc->ofile[i]);
	np->cwd = idup(curproc->cwd);
	*/

	strlcpy(np->name, curproc->name, sizeof(curproc->name));

	pid = np->pid;

	acquire(&ptable.lock);

	np->state = RUNNABLE;

	release(&ptable.lock);

	return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(int status)
{
	struct proc *curproc = myproc();
	struct proc *p;
	int fd;

	if (curproc == initproc)
		panic("init exiting");
	/*
	  // Close all open files.
	  for(fd = 0; fd < NOFILE; fd++){
	    if(curproc->ofile[fd]){
	      fileclose(curproc->ofile[fd]);
	      curproc->ofile[fd] = 0;
	    }
	  }

	  begin_op();
	  iput(curproc->cwd);
	  end_op();
	  curproc->cwd = 0;
	*/
	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(curproc->parent);

	// Pass abandoned children to init.
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->parent == curproc) {
			p->parent = initproc;
			if (p->state == ZOMBIE)
				wakeup1(initproc);
		}
	}

	// Jump into the scheduler, never to return.
	curproc->state = ZOMBIE;
	curproc->status = status;
	sched();
	panic("zombie exit");
}

int
wait(int *status)
{
	struct proc *p;
	int havekids, pid;
	struct proc *curproc = myproc();

	acquire(&ptable.lock);
	for (;;) {
		// Scan through table looking for exited children.
		havekids = 0;
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if (p->parent != curproc)
				continue;
			havekids = 1;
			if (p->state == ZOMBIE) {
				// Found one.
				pid = p->pid;
				// kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->state = UNUSED;
				*status = p->status;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if (!havekids || curproc->killed) {
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock); // DOC: wait-sleep
	}
}

void
scheduler(void)
{
	struct proc *p;
	struct cpu *c = mycpu();
	c->proc = 0;

	for (;;) {
		sti();

		// Loop over process table looking for process to run.
		acquire(&ptable.lock);
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if (p->state != RUNNABLE)
				continue;

			// It is process's job to release ptable.lock
			// and then reacquire before jumping back to us.
			c->proc = p;
			c->cur_kstack = p->kstack;
			switchuvm(p);
			p->state = RUNNING;

			swtch(&(c->scheduler), p->context);
			switchkvm();

			// Process is done running for now.
			// It should have changed its p->state before coming back.
			c->proc = 0;
		}
		release(&ptable.lock);
	}
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
	int intena;
	struct proc *p = myproc();

	if (!holding(&ptable.lock)) {
		panic("sched ptable.lock");
	}
	if (mycpu()->ncli != 1) {
		panic("sched locks");
	}
	if (p->state == RUNNING) {
		panic("sched running");
	}
	if (readrflags() & FL_IF) {
		panic("sched interruptible");
	}
	intena = mycpu()->intena;
	// we dont need cur_kstack to be update whereas kernel code never syscalls
	swtch(&p->context, mycpu()->scheduler);
	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
	acquire(&ptable.lock); // DOC: yieldlock (what?)
	myproc()->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
	static int first = 1;
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	if (first) {
		// Some initialization functions must be run in the context
		// of a regular process (e.g., they call sleep), and thus cannot
		// be run from main().
		first = 0;
		// iinit(ROOTDEV);
		// initlog(ROOTDEV);
	}

	// Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
	struct proc *p = myproc();

	if (p == 0)
		panic("sleep");

	if (lk == 0)
		panic("sleep without lk");

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.
	if (lk != &ptable.lock) {      // DOC: sleeplock0
		acquire(&ptable.lock); // DOC: sleeplock1
		release(lk);
	}
	// Go to sleep.
	p->chan = chan;
	p->state = SLEEPING;

	sched();

	// Tidy up.
	p->chan = 0;

	// Reacquire original lock.
	if (lk != &ptable.lock) { // DOC: sleeplock2
		release(&ptable.lock);
		acquire(lk);
	}
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
	struct proc *p;

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if (p->state == SLEEPING && p->chan == chan)
			p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}
// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
	struct proc *p;

	acquire(&ptable.lock);
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->pid == pid) {
			p->killed = 1;
			// Wake process from sleep if necessary.
			if (p->state == SLEEPING) {
				p->state = RUNNABLE;
			}
			release(&ptable.lock);
			return 0;
		}
	}
	release(&ptable.lock);
	return -1;
}
