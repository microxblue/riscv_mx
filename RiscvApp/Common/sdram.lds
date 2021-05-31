/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.
*/

MEMORY {
	/* the memory in the testbench is 64k in size;
	 * set LENGTH=48k and leave at least 16k for stack */
	mem : ORIGIN = 0xf0000000, LENGTH = 0x01000000
}

SECTIONS {
.memory : {
		. = 0x000000;
		*start.o(.text);
		*(.text);
		*(.rodata .rodata*)
		. = ALIGN(4);
	} > mem

	/* .bss section which is used for uninitialized data */
	.bss (NOLOAD) :
	{
			__bss_start__ = . ;
			*(.bss .bss.*)
			*(COMMON)
			. = ALIGN(4);
			 __bss_end__ = . ;
	} > mem

  __StackTop   = ORIGIN(mem) + LENGTH(mem);
  __StackLimit = __StackTop - 0x0200;
  PROVIDE(__stack = __StackTop);
	end = .;

	/* Check if data + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __bss_end__, "region RAM overflowed with stack")
}
