OUTPUT_FORMAT(binary)
STARTUP(loader.o)

SECTIONS {
	 .text 0x7c00 : {
	       *(.text)
	 }
	 .data : {
	       *(.data)
	       *(.rodata*)
	       *(.eh_frame)
		. = ALIGN (8);
                __init_begin = .;
                *(.init_func)
                __init_end = .;
                __deinit_begin = .;
                *(.deinit_func)
                __deinit_end = .;
	 }
	 .bss : {
	      __bss_begin = .;
	      *(.bss)
	      __bss_end = .;
	 }
}
