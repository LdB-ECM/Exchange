#include <arm.h>
#include <mmu.h>

.globl CoreSetup
CoreSetup:
	/* change processor to SYSTEM mode */
	mrs		r0, cpsr
	orr		r0, r0, #ARM_MODE_SYS
	msr		spsr_cxsf, r0
	add		r0, pc, #4
	msr		ELR_hyp, r0
	eret

;@"========================================================================="
@#    PI NSACR regsister setup for access to floating point unit
@#    Cortex A-7 => Section 4.3.34. Non-Secure Access Control Register
@#    Cortex A-53 => Section 4.5.32. Non-Secure Access Control Register
;@"========================================================================="
	mrc p15, 0, r0, c1, c1, 2				;@ Read NSACR into R0
	cmp r0, #0x00000C00						;@ Access turned on or in AARCH32 mode already
	beq .free_to_enable_fpu1
	orr r0, r0, #0x3<<10					;@ Set access to both secure and non secure modes
	mcr p15, 0, r0, c1, c1, 2				;@ Write NSACR
;@"========================================================================="
@#                           Bring fpu online
;@"========================================================================="
.free_to_enable_fpu1:
	mrc p15, 0, r0, c1, c0, #2				;@ R0 = Access Control Register
	orr r0, #(0x300000 + 0xC00000)			;@ Enable Single & Double Precision
	mcr p15,0,r0,c1,c0, #2					;@ Access Control Register = R0
	mov r0, #0x40000000						;@ R0 = Enable VFP
	vmsr fpexc, r0							;@ FPEXC = R0

	/* get CPUID from MPIDR register and put value in r0 */
	mrc		p15, 0, r0, c0, c0, 5		/* MPIDR */
	and		r0, r0, #7					/* last 3 bits is CPUID */

	/* multiply the CPUID by 4 to get the correct index for the arrays */
	mov		r1, r0
	mov		r2, #4
	mul		r1, r1, r2

	/* core_init_sp array is for the initial stack pointer for each core */	
	ldr		r2, =core_init_sp
	ldr		sp, [r2, r1]

	/* move value to non-volatile registers before calling start_mmu */
	mov		r4, r1

	/* call start_mmu so the programmer does not have to manual call it each time they unpark a core 			*/
	mov		r0, #MMUTABLEBASE		/* MMUTABLEBASE from mmu.h */
	bl		start_mmu

	/* corestart array of addresses for the instruction for each core to execute once setup is done */
	ldr		r0, =init_args
	ldr		r0, [r0, r4]
	ldr		r2, =corestart
	ldr		pc, [r2, r4]	


/* Send EVent routine so we can use it in C code.. (unparkcore.c) */
.globl sev
sev:
	.func sev
	sev
	bx	lr
	.endfunc
