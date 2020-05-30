.extern RPi_IO_Base_Addr					;@ Auto-detected Raspberry Pi Base address
.equ ARM6_CPU_ID, 0x410FB767;				;@ CPU id an ARM6 BCM2835 reports

/* "PROVIDE C FUNCTION: RegType_t EL0_Timer_Frequency (void);" */
.globl EL0_Timer_Frequency
EL0_Timer_Frequency:
	ldr r0, =#1000000						;@ Preset 1Mhz for single core
	mrc p15, 0, r1, c0, c0, 0				;@ Read CPU id register
	ldr r2, =#ARM6_CPU_ID					;@ Load ARM6 id register value
	cmp r2, r1								;@ Compare read register versus ARM6 id
	beq 1f									;@ If equal then core is ARM6 and single core
	mrc p15, 0, r0, c14, c0, 0				;@ Read core frequency register
1:
	bx  lr									;@ Return

/* "PROVIDE C FUNCTION: void EL0_Timer_Set (RegType_t nextCount);" */
.globl EL0_Timer_Set
EL0_Timer_Set:
	mrc p15, 0, r1, c0, c0, 0							;@ Read CPU id register
	ldr r2, =#ARM6_CPU_ID								;@ Load ARM6 id register value
	cmp r2, r1											;@ Compare read register versus ARM6 id
	beq 1f												;@ If equal then core is ARM6 and single core
	mcr p15, 0, r0, c14, c2, 0							;@ Set the frequency register
	bx  lr												;@ Return
1:
	ldr r1, =RPi_IO_Base_Addr							;@ Address of base address storage
	ldr r1, [r1]										;@ Fetch base address
	ldr r2, =#0x3000									;@ Offset to clock registers
	add r1, r1, r2										;@ Add offsets together
	ldr r2, [r1, #4]									;@ Read clock low register
	add r0, r0, r2										;@ Add cycles to value
	str r0, [r1, #0x18]									;@ Write to compare 3 register
 	mov r0, #8											;@ Create clear Interrupt mask
	str r0, [r1]										;@ Clear the interrupt
	bx  lr												;@ Return

/* "PROVIDE C FUNCTION: bool EL0_Timer_Fiq_Setup (void);" */
.globl EL0_Timer_Fiq_Setup
EL0_Timer_Fiq_Setup:
	mrc p15, 0, r1, c0, c0, 0							;@ Read cpu ID register
	ldr r2, =#ARM6_CPU_ID								;@ ARM 6 id
	cmp r2, r1											;@ Compare read cpu id to ARM6 id
	beq 1f												;@ Branch if equal
	mov	r2, #0x40000000									;@ QA7 start address
	mrc p15, 0, r0, c0, c0, 5							;@ Read core id on ARM7 & ARM8
	ands r0, r0, #0x3									;@ Make core 2 bit number in R0
	lsl	r0, r0, #2										;@ Multiply core number x4
	add	r3, r0, r2										;@ r3 = QA7 register address 
	mov	r1, #0x80000000									;@ R1 = divisor of 0x80000000
	str	r1, [r2, #8]									;@ QA7 prescaler = 0x80000000
	ldr	r2, [r3, #64]									;@ Read Core? timer Interrupt control 
	bic r2, r2, #0xF
	orr	r2, r2, #0x20									;@ nCNTPNSIRQ FIQ enable on core
	str	r2, [r3, #64]									;@ Write Core? timer Interrupt control
	mov r0, #1											;@ Return true
	mcr p15, 0, r0, c14, c2, 1							;@ Enable EL0 timer
	bx  lr												;@ Return
1:
	ldr r1, =RPi_IO_Base_Addr							;@ Address of RPi_IO_Base_Addr
	ldr r1, [r1]										;@ Fetch the RPi_IO_Base_Addr value
	add r1, r1, #0xB200									;@ Add irq registers offset
	mov r0, #0x83										;@ Set fiq source is 83
	str r0, [r1, #0x0C]									;@ Set the Fiq enable register	
	mov r0, #1											;@ Return true
	bx  lr												;@ Return
