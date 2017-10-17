OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY
{
	ram (xw) : ORIGIN = 0X8000, LENGTH = 0x8000000  
}

SECTIONS
{
	.text :
	{
		. = ALIGN(4);
		_start = . ;
		KEEP(*(.text.startup))
		*(.text*)
		. = ALIGN(4);
	} > ram
	.bss :
	{
		. = ALIGN(4);
		*(.bss*)
		. = ALIGN(4);
	} > ram
	.rodata :
	{
		. = ALIGN(4);
		*(.rodata*)
		. = ALIGN(4);
	} > ram
	.data :
	{
		. = ALIGN(4);
		*(.data*)
		. = ALIGN(4);
	} > ram
	.stack :
	{
		. = ALIGN(8);
		_stack_start = . ;
		. = . + 512;
		__stack_pointer_core0 = .;
		. = . + 512;
		__stack_pointer_core1 = .;
		. = . + 512;
		__stack_pointer_core2 = .;
		. = . + 512;
		__stack_pointer_core3 = .;	
		. = ALIGN(8);
	} > ram
}
