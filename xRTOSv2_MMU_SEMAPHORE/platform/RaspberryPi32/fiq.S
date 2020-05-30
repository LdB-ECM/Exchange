/*      
OFFSET        CONTEXT SWITCH STACK LAYOUT
======	      ===========================
0			cpsr		<- program status register to use on this task when running
4			retaddr		<- The address to return to when running
8			r0			<- r0	C caller argument and scratch register & result register
12			r1			<- r1	C caller argument and scratch register & result register
16			r2			<- r2	C caller argument and scratch register
20			r3			<- r3	C caller argument and scratch register
24			r4			<- r4	C callee-save register
28			r5			<- r5	C callee-save register
32			r6			<- r6	C callee-save register
36			r7			<- r7	C callee-save register
40			r8			<- r8	C callee-save register
44			r9			<- r9	might be a callee-save register or not (on some variants of AAPCS it is a special register)
48			r10			<- r10	C callee-save register
52			r11			<- r11	C callee-save register
56			r12			<- likely unused but save just in case
60			r13			<- The sp value prior to the context switch 
64			r14			<- The lr value prior to the context switch (which is usually a subroutne branch)
68			exitAddr	<- Exit address to return when task completes operation
*/

.equ ARM_MODE_FIQ, 0x11;								;@ CPU in FIQ mode .. FIQ Processing
.equ ARM_MODE_IRQ, 0x12;								;@ CPU in IRQ mode .. IRQ Processing
.equ ARM_MODE_SVC, 0x13;								;@ CPU in SVC mode .. Service mode
.equ ARM_MODE_SYS, 0x1F;								;@ CPU in SYS mode .. Priviledged Operating System Tasks mode

	.extern current
	.global fiq_handler_stub
fiq_handler_stub:
	SUB LR, LR, #4
	MRC  p15, 0, R10, c0, c0, 5	;@ Fetch core id
	AND R10, R10, #3			;@ Create core id value 
	LSL	R10, R10, #2			;@ x4 for core ptr offset

	LDR	R9, =core_ptr			;@ Address of core_ptr (defined in task.h) .. remember on FIQ R9 is banked so safe to do so
	ADD R9, R9, R10				;@ Add coreID x4 to locate table entry 
	LDR R9, [R9]				;@ Fetch addr of core data of that core
	LDR R9, [R9]				;@ Fetch current task on that core

	MRS R8, SPSR		        ;@ Fetch  SPSR_fiq into R8 .. remember on FIQ R8 is banked so safe to do so
    STR R8, [R9, #0]			;@ Save SPSR mode to ctx stack 
	STR LR, [R9, #4]			;@ Save LR_irq as retAddr to ctx stack

    /* Okay now save R0-R7 they are not banked */
	ADD R8, R9, #8				;@ Set (R8 + #8)  = R0 stack address for r0
	STMIA   R8!, {r0, r1, r2, r3, r4, r5, r6, r7 }
	MOV R0, R8					;@ Okay now transfer R8 to R0

	/* Change to SYS mode, with FIQ still disabled.  */
	cpsid f, #ARM_MODE_SYS

	/* Okay now save R8-R14 as they are exposed */ 
	STMIA   R0!, {r8, r9, r10, r11, r12, r13, r14 }

	/* Change to FIQ mode, with FIQ still disabled.  */
	cpsid f, #ARM_MODE_FIQ

	/* The stack pointer is 4-byte aligned at all times, but
	 * it must be 8-byte aligned when calling an externally visible
	 * function.  This is important because this code is reached from an FIQ
	 * and therefore the stack currently may only be 4-byte aligned.  If
	 * this is the case, the stack must be padded to an 8-byte boundary
	 * before calling timer_tick().  */
	AND R4, SP, #7
	SUB SP, SP, R4

	BL xTaskIncrementTick
	BL timer_clear_irq

	/* Restore the original stack alignment (see note about 8-byte alignment above).  */
	ADD SP, SP, R4

	MRC  p15, 0, R1, c0, c0, 5	;@ Fetch core id
	AND R1, R1, #3				;@ Create core id value 
	LSL	R1, R1, #2				;@ x4 for core ptr offset

	/* Fetch Context stack addres to R0 */
	LDR	R0, =core_ptr			;@ Address of core_ptr (defined in task.h) .. remember on FIQ R9 is banked so safe to do so
	ADD R0, R0, R1				;@ Add coreID x4 to locate table entry 
	LDR R0, [R0]				;@ Fetch addr of core data of that core
	LDR R0, [R0]				;@ Fetch current task on that core

	/* Okay now restore R2-R7 they are not banked */
	ADD R8, R0, #16				;@ Set (R0 + #16)  = R8 stack address for r2
	LDMIA   R8!, {r2, r3, r4, r5, r6, r7 }	
	MOV R1, R8					;@ Okay now transfer R8 to R1

	/* Change to SYS mode, with FIQ still disabled.  */
	cpsid f, #ARM_MODE_SYS

	/* Okay now restore SYS_MODE R8-R14 as they are exposed */ 
	LDMIA   R1!, {r8, r9, r10, r11, r12, r13, r14 }


	/* Ok restore SPRS_fiq and LR_fiq */
	LDR R1, [R0, #0]			;@ Fetch SPSR mode from ctx stack
	PUSH {R1}					;@ Push CPSR down onto whatever SYS stack
	LDR R1, [R0, #4]			;@ Restore LR_irq from retAddr of ctx stack
	PUSH {R1}					;@ Push the return address onto SYS stack

	LDR R1, [R0, #12]			;@ Restore R1
	LDR R0, [R0, #8]			;@ Restore R0
	RFEFD SP!					;@ POP PC and SPSR from stack


	



