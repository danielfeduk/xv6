// Memory layout
#include "param.h"

#define EXTMEM 0x100000	    // Start of extended memory
#define PHYSTOP 0x2000000   // Top physical memory
#define DEVSPACE 0xFE000000 // Other devices are at high addresses

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KSTACKBASE 0xfffffd8000000000
#define KSTACKTOP (0xfffffd8000000000 + KSTACKSIZE)
#define COREIDENBASE 0xfffffe8000000000
#define KERNBASE 0xffffff8000000000
#define KERNLINK (KERNBASE + EXTMEM) // Address where kernel is linked

#define V2P(a) (((u64)(a)) - KERNBASE)
#define P2V(a) ((void *)(((char *)(a)) + KERNBASE))
#define CM2P(a) (((u64)(a)) - COREIDENBASE)
#define P2CM(a) ((void *)(((char *)(a)) + COREIDENBASE))

#define V2P_WO(x) ((x) - KERNBASE) // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE) // same as P2V, but without casts
