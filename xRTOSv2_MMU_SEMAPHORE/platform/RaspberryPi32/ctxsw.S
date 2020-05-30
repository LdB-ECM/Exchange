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


;@"========================================================================="
@#	cpu_switch_to -- AARCH32 on Pi1, Pi2, Pi3, Pi4 code
@#	C Function: "void cpu_switch_to (TaskHandle_t next, TaskHandle_t prev);"
@#	Entry: R0 next context to switch onto and R1 prev context stack
@#	Return: nothing
;@"========================================================================="
    .global cpu_switch_to
	.global cpu_context_restore
cpu_switch_to:
	MRS R2, CPSR				;@ Fetch CPSR
	STR R2, [R1, #0]			;@ Save the CPSR
	CPSID f						;@ Disable FIQ it is not stored so in restore below it will re-enable on last instruction
	STR LR, [R1, #4]			;@ Save LR to retAddr of ctx stack (remember on a C call LR is return address)
    /* Okay now save R0-R14 */
	ADD R2, R1, #8				;@ Start at R0 and just save all the registers
	STMIA   R2!, {r0, r1}
	STMIA   R2!, {r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14 }

	/* Note this continues on thru cpu_context_restore it does not end here */

;@"========================================================================="
@#	cpu_context_restore -- AARCH32 on Pi1, Pi2, Pi3, Pi4 code
@#	C Function: "void cpu_context_restore (CtxSwitch_t* next);"
@#  *** Remember this auto decrements preempt count 
@#	Entry: R0 next TaskHandle_t to switch onto
@#	Return: nothing
;@"========================================================================="
cpu_context_restore:
	/* Okay now load R2-R14 */
	ADD R1, R0, #16				;@ Start at R2 in context stack for restore
	LDMIA   R1!, {r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14 }
	LDR R1, [R0, #0]			;@ Fetch the CPSR
	PUSH {R1}					;@ Push CPSR down onto whatever stack we were on
	LDR R1, [R0, #4]			;@ Fetch the return address
	PUSH {R1}					;@ Push the return address onto whatever stack we were on
	LDR R1, [R0, #12]			;@ Restore R1
	LDR R0, [R0, #8]			;@ Restore R0
	RFEFD SP!					;@ POP PC and SPSR from stack


;@"========================================================================="
@#	FetchRegistersToCtx -- AARCH32 on Pi1, Pi2, Pi3, Pi4 code
@#	C Function: "void FetchRegistersToCtx (CtxSwitch_t* ctx);"
@#  This is used mainly for debugging to check how registers would save
@#	Entry: R0 context to store current registers into
@#	Return: nothing
;@"========================================================================="
	.global FetchRegistersToCtx
FetchRegistersToCtx:
	/* Okay now load R2-R14 */
	ADD R1, R0, #16
	STMIA   R1!, {r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14 }
	MRS R2, CPSR_all			;@ Fetch CPSR
	STR R2, [R0, #0]			;@ Save the CPSR
	MOV R2, #0					;@ No way to know return address so just zero				
	STR LR, [R0, #4]			;@ Set return address
	STR R1, [R0, #12]			;@ Save R1 to ctx stack
	STR R0, [R0, #8]			;@ Save R0 to ctx stack
	BX LR;						;@ Return from call