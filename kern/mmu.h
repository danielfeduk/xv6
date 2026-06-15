// This file contains definitions for the
// x86 memory management unit (MMU).

// rflags register
#define FL_IF 0x00000200 // Interrupt Enable

// Control Register flags
#define CR0_PE 0x00000001 // Protection Enable
#define CR0_WP 0x00010000 // Write Protect
#define CR0_PG 0x80000000 // Paging

#define CR4_PSE 0x00000010 // Page size extension
#define CR4_PAE 0x00000020 // Phy. addr extension (PAE)
#define CR4_PGE 0x00000080 // Page global ext.

// MSRs
#define MSR_EFER 0xC0000080
#define EFER_SCE 0x001
#define EFER_LME 0x100
#define EFER_NXE 0x800
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_SFMASK 0xC0000084
#define MSR_FSBASE 0xC0000100
#define MSR_GSBASE 0xC0000101
#define MSR_KERNGSBASE 0xC0000102

// various segment selectors.
#define SEG_KCODE 1 // kernel code
#define SEG_KDATA 2 // kernel data+stack
#define SEG_UDATA 3 // user data+stack
#define SEG_UCODE 4 // user code
#define SEG_TSS 5   // this process's task state

// cpu->gdt[NSEGS] holds the above segments.
#define NSEGS 7
// I DON'T EVEN WANT TO COMMENT ON HOW FUCKED UP THIS IS

#ifndef __ASSEMBLER__
// Segment Descriptor
struct segdesc {
	u32 lim_15_0 : 16;  // Low bits of segment limit
	u32 base_15_0 : 16; // Low bits of segment base address
	u32 base_23_16 : 8; // Middle bits of segment base address
	u32 type : 4;	     // Segment type (see STS_ constants)
	u32 s : 1;	     // 0 = system, 1 = application
	u32 dpl : 2;	     // Descriptor Privilege Level
	u32 p : 1;	     // Present
	u32 lim_19_16 : 4;  // High bits of segment limit
	u32 avl : 1;	     // Unused (available for software use)
	u32 l : 1;	     // long mode
	u32 db : 1;	     // 0 = 16-bit segment, 1 = 32-bit segment
	u32 g : 1;	     // Granularity: limit scaled by 4K when set
	u32 base_31_24 : 8; // High bits of segment base address
};

struct syssegdesc64 {
	u16 lim_15_0;
	u16 base_15_0;
	u8 base_23_16;

	u8 type : 4;
	u8 s : 1; // must be 0 for system segments
	u8 dpl : 2;
	u8 p : 1;

	u8 lim_19_16 : 4;
	u8 avl : 1;
	u8 zero : 2; // must be 0
	u8 g : 1;

	u8 base_31_24;
	u32 base_63_32; // upper 32 bits of base
	u32 zero2;	// must be 0

} __attribute__((packed));

#define SEG(type, long, base, lim, dpl)                                                                                                                       \
	(struct segdesc) { ((lim) >> 12) & 0xffff, (u32)(base) & 0xffff, ((u32)(base) >> 16) & 0xff, type, 1, dpl, 1, (u32)(lim) >> 28, 0, long, !long, 1, \
		(u32)(base) >> 24 }
#define SEG16(type, base, lim, dpl)                                                                                                            \
	(struct segdesc) { (lim) & 0xffff, (u32)(base) & 0xffff, ((u32)(base) >> 16) & 0xff, type, 1, dpl, 1, (u32)(lim) >> 16, 0, 0, 1, 0, \
		(u32)(base) >> 24 }
#endif

#define TSS_AVL 0x9
#define TSS_BUSY 0xB

#define TSS64(_type, base, lim, _dpl)                                   \
	(struct syssegdesc64) { .lim_15_0 = (u16)((u64)(lim) & 0xFFFF), \
		.base_15_0 = (u16)((u64)(base) & 0xFFFF),               \
		.base_23_16 = (u8)(((u64)(base) >> 16) & 0xFF),         \
		.type = _type,                                          \
		.s = 0,                                                 \
		.dpl = (_dpl),                                          \
		.p = 1,                                                 \
		.lim_19_16 = (u8)(((u64)(lim) >> 16) & 0xF),            \
		.avl = 0,                                               \
		.zero = 0,                                              \
		.g = 0,                                                 \
		.base_31_24 = (u8)(((u64)(base) >> 24) & 0xFF),         \
		.base_63_32 = (u32)(((u64)(base) >> 32) & 0xFFFFFFFF),  \
		.zero2 = 0 }

#define DPL_USER 0x3 // User DPL

// Application segment type bits
#define STA_X 0x8 // Executable segment
#define STA_W 0x2 // Writeable (non-executable segments)
#define STA_R 0x2 // Readable (executable segments)

// System segment type bits
#define STS_T32A 0x9 // Available 32-bit TSS
#define STS_IG32 0xE // 32-bit Interrupt Gate
#define STS_TG32 0xF // 32-bit Trap Gate

/*
// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/

// page directory index
#define PDX(va)         (((u32)(va) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(va)         (((u32)(va) >> PTXSHIFT) & 0x3FF)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((u32)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NPDENTRIES      1024    // # directory entries per page directory
#define NPTENTRIES      1024    // # PTEs per page table
#define PGSIZE          4096    // bytes mapped by a page

#define PTXSHIFT        12      // offset of PTX in a linear address
#define PDXSHIFT        22      // offset of PDX in a linear address
*/
// ABOVE IS OLD I386 CONST'S

/*
 * X86_64 VIRTUAL ADDRESS STRUCTURE
 * +----9---------------9---------------9---------------9---------------12----+
 * | PML4	| PDPT		| PD		| PT		| page        |
 * |	index	|	index	|	index	|	index	|      offset |
 * +47----------+38-------------+29-------------+20-------------+11-----------0
 * */

#define PGSIZE 4096

#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

// Page table/directory entry flags.
#define PTE_P 0x001  // Present
#define PTE_W 0x002  // Writeable
#define PTE_U 0x004  // User
#define PTE_PS 0x080 // Page Size

#define NPML4ENTRIES 512
#define NPDPTENTRIES 512
#define NPDENTRIES 512
#define NPTENTRIES 512

#define PML4XSHIFT 39
#define PDPTXSHIFT 30
#define PDXSHIFT 21
#define PTXSHIFT 12

#define PML4X(va) (((u64)(va) >> PML4XSHIFT) & 0x1FF)
#define PDPTX(va) (((u64)(va) >> PDPTXSHIFT) & 0x1FF)
#define PDX(va) (((u64)(va) >> PDXSHIFT) & 0x1FF)
#define PTX(va) (((u64)(va) >> PTXSHIFT) & 0x1FF)

// todo fix ugly 1<<63 NX retrofit
// Address in page table or page directory entry
#define PGFLAGMASK 0xFFF0000000000FFF
#define PTE_ADDR(pte) ((u64)(pte) & ~PGFLAGMASK)
#define PTE_FLAGS(pte) ((u64)(pte) & PGFLAGMASK)

#ifndef __ASSEMBLER__
typedef u64 pte_t;
typedef u64 pde_t;
/*
// Task state segment format
struct taskstate {
  u32 link;         // Old ts selector
  u32 esp0;         // Stack pointers and segment selectors
  u16 ss0;        //   after an increase in privilege level
  u16 padding1;
  u32 *esp1;
  u16 ss1;
  u16 padding2;
  u32 *esp2;
  u16 ss2;
  u16 padding3;
  void *cr3;         // Page directory base
  u32 *eip;         // Saved state from last task switch
  u32 eflags;
  u32 eax;          // More saved state (registers)
  u32 ecx;
  u32 edx;
  u32 ebx;
  u32 *esp;
  u32 *ebp;
  u32 esi;
  u32 edi;
  u16 es;         // Even more saved state (segment selectors)
  u16 padding4;
  u16 cs;
  u16 padding5;
  u16 ss;
  u16 padding6;
  u16 ds;
  u16 padding7;
  u16 fs;
  u16 padding8;
  u16 gs;
  u16 padding9;
  u16 ldt;
  u16 padding10;
  u16 t;          // Trap on task switch
  u16 iomb;       // I/O map base address
};
*/

// note: we only use rsp0 here for now
struct taskstate64 {
	u32 resvd0;
	u64 rsp0, rsp1, rsp2;
	u64 resvd1;
	u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
	u64 resvd2;
	u16 resvd3;
	u16 iopb;
} __attribute((packed));

// Gate descriptors for interrupts and traps
struct gatedesc {
	u32 off_15_0 : 16;  // low 16 bits of offset in segment
	u32 cs : 16;	     // code segment selector
	u32 args : 5;	     // # args, 0 for interrupt/trap gates
	u32 rsv1 : 3;	     // reserved(should be zero I guess)
	u32 type : 4;	     // type(STS_{IG32,TG32})
	u32 s : 1;	     // must be 0 (system)
	u32 dpl : 2;	     // descriptor(meaning new) privilege level
	u32 p : 1;	     // Present
	u32 off_31_16 : 16; // high bits of offset in segment
};

struct gatedesc64 {
	u16 off_15_0;		 // low 16 bits of offset in segment
	u16 cs;			 // code segment selector
	unsigned ist : 3;	 // TSS IST entry (stack pointer)
	unsigned rsv1 : 5;	 // reserved(should be zero I guess)
	unsigned type : 4;	 // type(STS_{IG32,TG32})
	unsigned s : 1;		 // must be 0 (system)
	unsigned dpl : 2;	 // descriptor(meaning new) privilege level
	unsigned p : 1;		 // Present
	unsigned off_31_16 : 16; // 16 higher bits of offset in segment
	u32 off_63_32;		 // top 32 bits of offset
	u32 rsv2;
} __attribute__((packed));

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, d)                    \
	{                                                     \
		(gate).off_15_0 = (u32)(off) & 0xffff;       \
		(gate).cs = (sel);                            \
		(gate).args = 0;                              \
		(gate).rsv1 = 0;                              \
		(gate).type = (istrap) ? STS_TG32 : STS_IG32; \
		(gate).s = 0;                                 \
		(gate).dpl = (d);                             \
		(gate).p = 1;                                 \
		(gate).off_31_16 = (u32)(off) >> 16;         \
	}

#define SETGATE64(gate, istval, istrap, sel, off, d)                \
	{                                                           \
		(gate).off_15_0 = (u64)(off) & 0xffff;              \
		(gate).cs = (sel);                                  \
		(gate).ist = istval;                                \
		(gate).rsv1 = 0;                                    \
		(gate).type = (istrap) ? STS_TG32 : STS_IG32;       \
		(gate).s = 0;                                       \
		(gate).dpl = (d);                                   \
		(gate).p = 1;                                       \
		(gate).off_31_16 = ((u64)(off) >> 16) & 0xffffffff; \
		(gate).off_63_32 = (u64)(off) >> 32;                \
	}

#endif
