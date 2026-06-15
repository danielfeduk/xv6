// Format of an ELF executable file
/*
#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  u32 magic;  // must equal ELF_MAGIC
  u8 elf[12];
  u16 type;
  u16 machine;
  u32 version;
  u32 entry;
  u32 phoff;
  u32 shoff;
  u32 flags;
  u16 ehsize;
  u16 phentsize;
  u16 phnum;
  u16 shentsize;
  u16 shnum;
  u16 shstrndx;
};

// Program section header
struct proghdr {
  u32 type;
  u32 off;
  u32 vaddr;
  u32 paddr;
  u32 filesz;
  u32 memsz;
  u32 flags;
  u32 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
*/
#define ELF_MAGIC 0x464C457FU // "\x7FELF" in little endian

// File header
struct elfhdr {
	u32 magic; // must equal ELF_MAGIC
	u8 elf[12];
	u16 type;
	u16 machine;
	u32 version;
	u64 entry; // was u32 (4B), now 8B
	u64 phoff; // was u32 (4B), now 8B
	u64 shoff; // was u32 (4B), now 8B
	u32 flags;
	u16 ehsize;
	u16 phentsize;
	u16 phnum;
	u16 shentsize;
	u16 shnum;
	u16 shstrndx;
};

// Program section header
struct proghdr {
	u32 type;
	u32 flags; // moved from end — critical change
	u64 off;    // was u32 (4B), now 8B
	u64 vaddr;  // was u32 (4B), now 8B
	u64 paddr;  // was u32 (4B), now 8B
	u64 filesz; // was u32 (4B), now 8B
	u64 memsz;  // was u32 (4B), now 8B
	u64 align;  // was u32 (4B), now 8B
};

// Values for Proghdr type
#define ELF_PROG_LOAD 1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4
