OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
	/*
	 * First and formost we need the .init section, containing the code to 
     * be run first. We allow room for the ATAGs and stack and conform to 
     * the bootloader's expectation by putting this code at 0x8000.
	 */
	.init 0x80000 : {
		KEEP(*(.init))
	}

	/* 
	 * Next we put the rest of the code.
	 */
	.text : {
		. = ALIGN(4);
		 __text_start__ = .;     	/* Label in case we want address of text section start */
		*(.text .text.*)
		 __text_end__ = .;      	/* Label in case we want address of text section end */
	}

	/* 
	 * Next we put the rodata .. C/C++ compilers store preset constants here.
	 */
	.rodata : {
       	. = ALIGN(4);
        __rodata_start__ = .;		/* Label in case we want address of rodata section start */
        *(.rodata .rodata.*)
        __rodata_end__ = .;			/* Label in case we want address of rodata section start */
   	}

	/* 
	 * Next we put the data.
	 */
	.data : {
		. = ALIGN(4);
		 __data_start__ = .;     	/* Label in case we want address of data section start */
		*(.data .data.*)
		 __data_end__ = .;     		/* Label in case we want address of data section end */
	}

	/* 
	 * Next we put the bss data .. C/C++ compilers produce this and needs to be zeroed by startup
	 */
	.bss : {
		. = ALIGN(4);
		 __bss_start__ = .;     	/* Label in case we want address of BSS section start */
		*(.bss .bss.*)
 		*(COMMON)
		 __bss_end__ = .;     		/* Label in case we want address of BSS section end */
   	}

	.heap :	{
     	. = ALIGN(4);
     	__heap_start__ = .;			/* Label in case we want address of heap section start */
    	_end = .; PROVIDE (end = .);/* Any memory from here is free to use so this is end of code and start of heap */
	}

	/*
	* Finally comes everything else. A fun trick here is to put all other 
	* sections into this section, which will be discarded by default.
	*/
	/DISCARD/ : {
		*(*)
	}
}
