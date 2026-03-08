OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	. = 0x7C00;

	.text : {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}
	. = 0x7C00 + 510;
	
	.sig : {
		SHORT(0xAA55)
	}

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}