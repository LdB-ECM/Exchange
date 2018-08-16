.section ".text.boot"
.balign 4
.global _start

_start:	
//"================================================================"
//  Initilize MPID/MPIDR registers for all Cores
//"================================================================"
	mrs	x0, midr_el1
	mrs	x1, mpidr_el1
	msr	vpidr_el2, x0
	msr	vmpidr_el2, x1

//"================================================================"
//  Disable coprocessor traps for all Cores
//"================================================================"
	mov	x0, #0x33ff
	msr	cptr_el2, x0						// Disable coprocessor traps to EL2
	msr	hstr_el2, xzr						// Disable coprocessor traps to EL2
	mov	x0, #3 << 20
	msr	cpacr_el1, x0						// Enable FP/SIMD at EL1

//"================================================================"
//  Enable CNTP for EL1
//"================================================================"
	mrs	x0, cnthctl_el2
	orr	x0, x0, #3
	msr	cnthctl_el2, x0
	msr	cntvoff_el2, xzr

//"================================================================"
//  Initialize HCR_EL2 so EL1 is 64 bits for all Cores
//"================================================================"
	mov	x0, #(1 << 31)						// 64bit EL1
	msr	hcr_el2, x0

//"================================================================"
//  Initialize SCTLR_EL1 for all Cores
//"================================================================"
	/*  RES1 bits (29,28,23,22,20,11) to 1
	 *  RES0 bits (31,30,27,21,17,13,10,6) +
	 *  UCI,EE,EOE,WXN,nTWE,nTWI,UCT,DZE,I,UMA,SED,ITD,
	 *  CP15BEN,SA0,SA,C,A,M to 0 */
	mov	x0, #0x0800
	movk	x0, #0x30d0, lsl #16
	orr    x0, x0, #(0x1 << 2)            // The C bit on (data cache). 
	orr    x0, x0, #(0x1 << 12)           // The I bit on (instruction cache)
	msr	sctlr_el1, x0

//"================================================================"
//  Set up exception handlers
//"================================================================"
	ldr	x0, =_vectors
	msr	vbar_el1, x0

//"================================================================"
//  Return to the EL1_SP1 mode from EL2 for all Cores
//"================================================================"
	mov	x0, #0x3c5					// EL1_SP1 | D | A | I | F
	msr	spsr_el2, x0					// Set spsr_el2 with settings
	adr	x0, exit_el1					// Address to exit EL2
	msr	elr_el2, x0					// Set elevated return register
	eret							// Call elevated return

//"================================================================"
//  Branch all cores to their destination
//"================================================================"
exit_el1:	
	mrs 	x1, mpidr_el1 // Read core id on AARCH64
	and 	x1, x1, #0x3  // Make core 2 bit bitmask in x1
	cbz  	x1, 2f	      // Core 0 jumps out to label 2
	cmp 	x1, #1        // Check for core1
	beq  	1f
	b 	_hang

1:
	ldr     x1, =(_start-0x10000)
	mov     sp, x1
	bl       core1_main
	b _hang

// core0 main
2:	
	ldr     x1, =_start
	mov     sp, x1


//"================================================================"
//  About to go to into C kernel clear BSS (Core0 only)
//"================================================================"
	ldr x3, =__bss_end
	ldr x0, =__bss_start
	cmp	x0, x3
	bcs	main
.bss_zero_loop:
	str	wzr, [x0], 4
	cmp	x3, x0
	bhi	.bss_zero_loop

.global _asm_enter_main
	bl	main

.global _hang
_hang:
	wfe
	b _hang


// save registers before we call any C code
dbg_saveregs:
	ldr     x29, =dbg_regs     // We saved this register before call so fine to use it
	str     x0, [x29], #8
	str     x1, [x29], #8
	str     x2, [x29], #8        // dbg_regs[2]=x2
	str     x3, [x29], #8        // ...etc.
	str     x4, [x29], #8
	str     x5, [x29], #8
	str     x6, [x29], #8
	str     x7, [x29], #8
	str     x8, [x29], #8
	str     x9, [x29], #8
	str     x10, [x29], #8
	str     x11, [x29], #8
	str     x12, [x29], #8
	str     x13, [x29], #8
	str     x14, [x29], #8
	str     x15, [x29], #8
	str     x16, [x29], #8
	str     x17, [x29], #8
	str     x18, [x29], #8
	str     x19, [x29], #8
	str     x20, [x29], #8
	str     x21, [x29], #8
	str     x22, [x29], #8
	str     x23, [x29], #8
	str     x24, [x29], #8
	str     x25, [x29], #8
	str     x26, [x29], #8
	str     x27, [x29], #8
	str     x28, [x29], #8
	/* remember the extra x29,x30 push well pull then back in x0, x1 and bring stack back up 16 */
	ldp	x0, x1, [sp], #16		
	str     x0, [x29], #8
	str     x1, [x29], #8
   // also read and store some system registers
	mrs     x1, elr_el1
	str     x1, [x29], #8
	mrs     x1, spsr_el1
	str     x1, [x29], #8
	mrs     x1, esr_el1
	str     x1, [x29], #8
	mrs     x1, far_el1
	str     x1, [x29], #8
	mrs     x1, sctlr_el1
	str     x1, [x29], #8
	mrs     x1, tcr_el1
	str     x1, [x29], #8
	ret

// Save all corruptible registers (x29,x30 are assumed done before call)
register_save:
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!
	str	x0, [sp, #-16]!
	ret

// Restore all corruptible registers
register_restore:
	ldr	x0, [sp], #16
	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x9, x10, [sp], #16
	ldp	x11, x12, [sp], #16
	ldp	x13, x14, [sp], #16
	ldp	x15, x16, [sp], #16
	ldp	x17, x18, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ret

	// important, code has to be properly aligned
	.balign 0x800
_vectors:
	// synchronous
	.balign  0x80
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 just so we dont waste space
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 for display we will pop inside dbg_saveregs
	bl		register_save		 // Save corruptible registers .. it assumes x29,x30 saved
	bl      dbg_saveregs
	mov     x0, #0
	bl      dbg_decodeexc
	bl      dbg_main
	mov     x0, #1
	bl      set_ACT_LED
	bl		register_restore	// restore corruptible registers .. does all but x29,x30
	ldp	x29, x30, [sp], #16		// restore x29,x30 pulling stack back up 16
	eret

	// IRQ
	.balign  0x80
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 just so we dont waste space
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 for display we will pop inside dbg_saveregs
	bl		register_save		 // Save corruptible registers .. it assumes x29,x30 saved
	bl      dbg_saveregs
	mov     x0, #1
	bl      dbg_decodeexc
	bl      dbg_main
	bl		register_restore	// restore corruptible registers .. does all but x29,x30
	ldp	x29, x30, [sp], #16		// restore x29,x30 pulling stack back up 16
	eret

	// FIQ
	.balign  0x80
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 just so we dont waste space
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 for display we will pop inside dbg_saveregs
	bl		register_save		 // Save corruptible registers .. it assumes x29,x30 saved
	bl      dbg_saveregs
	mov     x0, #2
	bl      dbg_decodeexc
	bl      dbg_main
	bl		register_restore	// restore corruptible registers .. does all but x29,x30
	ldp	x29, x30, [sp], #16		// restore x29,x30 pulling stack back up 16
	eret

	// SError
	.balign  0x80
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 just so we dont waste space
	stp	x29, x30, [sp, #-16]!	 // Save x30 link register and x29 for display we will pop inside dbg_saveregs
	bl		register_save		 // Save corruptible registers .. it assumes x29,x30 saved
	bl      dbg_saveregs
	mov     x0, #3
	bl      dbg_decodeexc
	bl      dbg_main
	bl		register_restore	// restore corruptible registers .. does all but x29,x30
	ldp	x29, x30, [sp], #16		// restore x29,x30 pulling stack back up 16
	eret
