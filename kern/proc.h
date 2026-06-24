// x86_64 GDT
struct gdt {			     // TODO fix magic no
	struct segdesc seg[5];	     // normal segs
	struct syssegdesc64 tss_seg; // tss
} __attribute__((packed));

// Per-CPU state
struct cpu {
	struct cpu *this;
	char *cur_kstack, *cur_ustack; // these are used by syscallasm.S
	struct proc *proc;	       // The process running on this cpu or null
	u8 apicid;		       // Local APIC ID
	struct context *scheduler;     // swtch() here to enter scheduler
	struct taskstate64 ts;	       // Used by x86 to find stack for interrupt
	struct gdt gdt;
	volatile u32 started; // Has the CPU started?
	int ncli;	       // Depth of pushcli nesting.
	int intena;	       // Were interrupts enabled before pushcli?
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
	u64 rbx;
	u64 rbp;
	u64 rdi;
	u64 rsi;
	// u64 rsp;
	u64 r12, r13, r14, r15;
	u64 rip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
	struct trapframe *tf;	    // Trap frame for current syscall
	u64 sz;			    // Size of process memory (bytes)
	pde_t *pgdir;		    // Page table
	char *kstack;		    // Bottom of kernel stack for this process
	enum procstate state;	    // Process state
	int pid;		    // Process ID
	struct proc *parent;	    // Parent process
	struct context *context;    // swtch() here to run process
	void *chan;		    // If non-zero, sleeping on chan
	int killed;		    // If non-zero, have been killed
	int status;		    // Process exit status if zombie
	struct file *ofile[NOFILE]; // Open files
	struct inode *cwd;	    // Current directory
	char name[16];		    // Process name (debugging)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
