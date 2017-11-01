;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
;@"																		    "			
;@"      Filename: smartstart32.s										    "
;@"      Copyright(c): Leon de Boer(LdB) 2017						        "
;@"      Version: 2.04													    "
;@"																		    "		
;@"***************[ THIS CODE IS FREEWARE UNDER CC Attribution]*************"
;@"										                                    "
;@"     This sourcecode is released for the purpose to promote programming  "
;@"  on the Raspberry Pi. You may redistribute it and/or modify with the    "
;@"  following disclaimer and condition.                                    "
;@"										                                    "
;@"      The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES AS TO    "
;@"   PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR IMPLIED.		    "
;@"   Redistributions of source code must retain the copyright notices to   "
;@"   maintain the author credit (attribution) .						    "
;@"										                                    "
;@"*************************************************************************"
;@"                                                                         "
;@"      This code expands on my earlier SmartStart bootstub assembler for  "
;@"   the Pi 1/2/3. It simplifies some of the autodetect code importantly   "
;@"  directly supports multicore operation in C/C++. To do that it provides "
;@"  stack space to each core via allocation in the linker directive file.  "
;@"                           									            "
;@"      The downside is this bootstub must be a little more complex in the "
;@"  start and setup of the multiple cores. The code is highly connected to "
;@"  the linker file which provides stack space to each core. However there "
;@"  is a matching paired AARCH64 stub and linker files so code written for "
;@"  this setup can be ported to AARCH64 easily.					        "
;@"										                                    "
;@"+++++++++++++++++++++++[ REVISIONS ]+++++++++++++++++++++++++++++++++++++"
;@"  1.01 Initial release .. Pi autodetection main aim                      "
;@"  1.02 Many functions moved out C to aide 32/64 bit compatability        "
;@"  2.01 Futher reductions to bare minmum assembeler code                  "
;@"  2.02 Multicore functionality added                                     "
;@"  2.03 Timer Irq support added  							                "
;@"  2.04 Multicore and linker refinements					                "
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

;@"*************************************************************************"
;@#	  		     CONSTANTS INTERNAL TO THIS UNIT
;@"*************************************************************************"

;@"========================================================================="
@#	          	  	      ARM CPU ID CONSTANT DEFINITIONS
;@"========================================================================="
.equ ARM6_CPU_ID, 0x410FB767			;@ CPU id a BCM2835 reports
.equ ARM7_CPU_ID, 0x410FC073			;@ CPU id a BCM2836 reports
.equ ARM8_CPU_ID, 0x410FD034			;@ CPU id a BCM2837 reports

;@"========================================================================="
@#		                ARM CPU MODE CONSTANT DEFINITIONS
;@"========================================================================="
.equ CPU_FIQMODE, 0x11					;@ CPU in FIQ mode
.equ CPU_IRQMODE, 0x12					;@ CPU in IRQ mode
.equ CPU_SVCMODE, 0x13					;@ CPU in SVC mode
.equ CPU_HYPMODE, 0x1A					;@ CPU in HYP mode

;@"========================================================================="
@#		            ARM CPU IRQ/FIQ BIT CONSTANT DEFINITIONS
;@"========================================================================="
.equ I_Bit,  (1 << 7)					;@ Irq flag bit in cpsr (CPUMODE register)
.equ F_Bit,  (1 << 6)					;@ Fiq flag bit in cpsr (CPUMODE register)

;@"========================================================================="
@#				 	 ARM CPU MODE CONSTANT DEFINITIONS
;@"========================================================================="
.equ CPU_FIQMODE_VALUE, (CPU_FIQMODE | I_Bit | F_Bit)	;@ CPU in FIQ mode with irq, fiq off
.equ CPU_IRQMODE_VALUE, (CPU_IRQMODE | I_Bit | F_Bit)	;@ CPU in IRQ mode with irq, fiq off
.equ CPU_SVCMODE_VALUE, (CPU_SVCMODE | I_Bit | F_Bit)	;@ CPU in SVC mode with irq, fiq off

;@"========================================================================="
@#             SCTLR REGISTER DEFINITIONS (controls L1 cache)
;@"========================================================================="
.equ SCTLR_ENABLE_DATA_CACHE,			0x4
.equ SCTLR_ENABLE_BRANCH_PREDICTION,	0x800
.equ SCTLR_ENABLE_INSTRUCTION_CACHE,	0x1000


;@"*************************************************************************"
;@#	  		      >>>>>>>   CODE STARTS HERE   <<<<<<<<<<
;@"*************************************************************************"
.section ".init" 
.globl _start
.balign 4
_start: 
;@"========================================================================="
@#         Grab cpu mode and start address and hold in a high register.
;@"========================================================================="
	mov r12, pc;										;@ Hold boot address in high register R12
	mrs r11, CPSR										;@ Read current CPU mode
	and r11, r11, #0x1F									;@ Clear all but CPU mode bits in register r11
;@"========================================================================="
@#    If the cpu is in HYP_MODE(EL2) we will bring it SVC_MODE (EL1).
;@"========================================================================="
   	mrs r0,cpsr											;@ Fetch the cpsr register which includes CPU mode bits 
 	and r1, r0, #0x1F									;@ Mask off the CPU mode bits to register r1                            
 	cmp r1, #CPU_HYPMODE								;@ Check we are in HYP_MODE											
	bne .NotInHypMode									;@ Branch if not equal meaning was not in HYP_MODE  
	bic r0,r0,#0x1F										;@ Clear the CPU mode bits in register r0							
	orr r0, r0, #CPU_SVCMODE_VALUE						;@ SVC_MODE bits onto register with Irq/Fiq disabled	
   	msr spsr_cxsf,r0									;@ Hold value in spsr_cxsf
   	add lr,pc,#4										;@ Calculate address of .NotInHypMode label
	/* I borrowed this trick from Ultibo because ARM6 code running on an ARM7/8 needs this opcode  */
	/* The ARM6 compiler does not know these instructions so it is a way to get needed opcode here */
  	/* So our ARM6 code can drop an arm7 or arm8 out of HYP mode and run on an ARM7/8.             */
 	/* Native ARM7/8 compilers already understand the OPCODE but do not mind it this way either	   */        
	.4byte 0xE12EF30E									;@ "msr ELR_hyp, lr" Set the address to ELR_hyp
	.4byte 0xE160006E									;@ "eret" Elevated return which will exit at .NotInHypMode in SVC_MODE
.NotInHypMode:
;@"========================================================================="
@#     Setup stack pointers for each core released and each CPU mode
;@"========================================================================="
    ldr r2, = __SVC_stack_core0							;@ Address of svc_stack_core0 stack pointer value
   	ldr r3, = __FIQ_stack_core0							;@ Address of fiq_stack_core0 stack pointer value
   	ldr r4, = __IRQ_stack_core0							;@ Address of irq_stack_core0 stack pointer value
	mrc p15, 0, r0, c0, c0, 0							;@ Read CPU ID Register
	ldr r1, =#ARM6_CPU_ID								;@ Fetch ARM6_CPU_ID
	cmp r1, r0											;@ Check for match
	beq set_svc_stack									;@ ARM6 only has 1 core so goto set svc_stack
	mrc p15, 0, r5, c0, c0, 5							;@ Read core id on ARM7 & ARM8
	ands r5, r5, #0x3									;@ Make cpu id bitmask
	beq set_svc_stack									;@ This is core 0 so set got set svc_stack
    ldr r2, = __SVC_stack_core1							;@ Address of svc_stack_core1 stack pointer value
    ldr r3, = __FIQ_stack_core1							;@ Address of fiq_stack_core1 stack pointer value
    ldr r4, = __IRQ_stack_core1							;@ Address of irq_stack_core1 stack pointer value
	cmp r5, #1											;@ Check cpu id for core 1
	beq set_svc_stack									;@ This is core 1 so set svc_stack
   	ldr r2, = __SVC_stack_core2							;@ Address of svc_stack_core2 stack pointer value
    ldr r3, = __FIQ_stack_core2							;@ Address of fiq_stack_core2 stack pointer value
   	ldr r4, = __IRQ_stack_core2							;@ Address of irq_stack_core2 stack pointer value
	cmp r5, #2											;@ Check cpu id for core 2
	beq set_svc_stack									;@ This is core 2 so set svc_stack
    ldr r2, = __SVC_stack_core3							;@ Address of svc_stack_core3 stack pointer value
   	ldr r3, = __FIQ_stack_core3							;@ Address of fiq_stack_core3 stack pointer value
    ldr r4, = __IRQ_stack_core3							;@ Address of irq_stack_core3 stack pointer value
set_svc_stack:
	mov sp, r2											;@ Set the stack pointer for SVC_MODE 
   	mrs r0,cpsr											;@ Fetch the cpsr register which includes CPU mode bits 
	bic r0, r0, #0x1F									;@ Clear the CPU mode bits in register r0							
	orr r0, r0, #CPU_FIQMODE_VALUE						;@ FIQ_MODE bits onto register with Irq/Fiq disabled
    msr CPSR_c, r0										;@ Switch to FIQ_MODE
	mov sp, r3											;@ Set the stack pointer for FIQ_MODE 
	bic r0, r0, #0x1F									;@ Clear the CPU mode bits in register r0	
	orr r0, r0, #CPU_IRQMODE_VALUE						;@ IRQ_MODE bits onto register with Irq/Fiq disabled	
   	msr CPSR_c, r0										;@ Switch to IRQ_MODE
	mov sp, r4											;@ Set the stack pointer for IRQ_MODE 
	bic r0,r0,#0x1F										;@ Clear the CPU mode bits in register r0							
	orr r0, r0, #CPU_SVCMODE_VALUE						;@ SVC_MODE bits onto register with Irq/Fiq disabled	
    msr CPSR_c, r0										;@ Switch to SVC_MODE again all stacks ready to go
;@"========================================================================="
@#      PI NSACR regsister setup for access to floating point unit
@#      Cortex A-7 => Section 4.3.34. Non-Secure Access Control Register
@#      Cortex A-53 => Section 4.5.32. Non-Secure Access Control Register
;@"========================================================================="
	mrc p15, 0, r0, c1, c1, 2							;@ Read NSACR into R0
	cmp r0, #0x00000C00									;@ Access turned on or in AARCH32 mode and can not touch register or EL3 fault
	beq .free_to_enable_fpu1
	orr r0, r0, #0x3<<10								;@ Set access to both secure and non secure modes
	mcr p15, 0, r0, c1, c1, 2							;@ Write NSACR
;@"========================================================================="
@#                          Bring fpu online
;@"========================================================================="
.free_to_enable_fpu1:
	mrc p15, 0, r0, c1, c0, #2							;@ R0 = Access Control Register
	orr r0, #(0x300000 + 0xC00000)						;@ Enable Single & Double Precision
	mcr p15,0,r0,c1,c0, #2								;@ Access Control Register = R0
	mov r0, #0x40000000									;@ R0 = Enable VFP
	vmsr fpexc, r0										;@ FPEXC = R0
;@"========================================================================="
@#                                                 Enable L1 cache
;@"========================================================================="
   	mrc p15,0,r0,c1,c0,0								;@ R0 = System Control Register

	/* Enable caches and branch prediction */
   	orr r0, #SCTLR_ENABLE_BRANCH_PREDICTION				;@ Enable branch prediction
    orr r0, #SCTLR_ENABLE_DATA_CACHE					;@ Enable data cache
    orr r0, #SCTLR_ENABLE_INSTRUCTION_CACHE				;@ Enable instruction cache

    mcr p15,0,r0,c1,c0,0								;@ System Control Register = R0
;@"========================================================================="
@#     Now store initial CPU boot mode and address we might need later.
;@"========================================================================="
	ldr r1, =RPi_BootAddr								;@ Address to hold Boot address
	sub r12, #8											;@ Subtract op-code offset
	str r12, [r1]										;@ Save the boot address we started at
	ldr r1, =RPi_CPUBootMode							;@ Memory address to save this CPU boot mode
	str r11, [r1]										;@ Save the boot mode we started in
;@"========================================================================="
@#     Fetch and hold CPU changed mode. If we changed modes this value
@#     will now reflect a change from the original held RPi_CPUBootMode.
;@"========================================================================="
	mrs r2, CPSR										;@ Read CPU mode
	and r2, r2, #0x1F									;@ Clear all but CPU mode bits in register r2
	ldr r1, =RPi_CPUCurrentMode							;@ Address of CPU current mode
	str r2, [r1]										;@ Hold the changed CPU mode
;@"========================================================================="
@#         Read the Arm Main CPUID register => sets RPi_CpuId
;@"========================================================================="
	ldr r1, =RPi_CpuId									;@ Address to hold CPU id
	mrc p15, 0, r0, c0, c0, 0							;@ Read Main ID Register
	str r0, [r1]										;@ Save CPU Id for interface 
;@"========================================================================="
@#            Store the compiler mode in RPi_CompileMode
;@"========================================================================="
	eor r0, r0, r0;										;@ Zero register
.if (__ARM_ARCH == 6)				// Compiling for ARM6
	mov r0, #0x06										;@ Compiled for ARM6 CPU
.endif
.if (__ARM_ARCH == 7)				// Compiling for ARM7
	mov r0, #0x07										;@ Compiled for ARM7 CPU
.endif
.if (__ARM_ARCH == 8)				// Compiling for ARM8
	mov r0, #0x08										;@ Compiled for ARM8 CPU
.endif
	orr r0, r0, #(4 << 5)								;@ Code is setup to support 4 cores			
	ldr r1, =RPi_CompileMode							;@ Compile mode address
	str r0, [r1]										;@ Store the compiler mode  
;@"========================================================================="
@#       Try Auto-Detect Raspberry PI IO base address at 1st position
;@"========================================================================="
	ldr r2, =#0x61757830								;@ Value Pi1 uart0 values default
	ldr r1, =#0x20215010								;@ Pi1 address for UART0
	ldr r0, [r1]										;@ Fetch value at 0x20215010 being uart0
	cmp r2, r0;											;@ Check if we have the value the uart0 will be at reset
	bne .not_at_address_1
;@"========================================================================="
@#         Raspberry PI IO base address was detected as 0x20000000
@#        RPi_IO_Base_Addr => 0x20000000
;@"========================================================================="
	ldr r1, =RPi_BusAlias								;@ Address of bus alias is stored
	mov r0, #0x40000000									;@ Preset alias 0x40000000
	str r0, [r1]										;@ Hold bus alias
	ldr r1, =RPi_IO_Base_Addr							;@ Address of base address is stored
	mov r0, #0x20000000									;@ Pi base address is 0x20000000
	str r0, [r1]										;@ Hold the detected address
	b .autodetect_done;
;@"========================================================================="
@#      Try Auto-Detect Raspberry PI IO base address at 2nd position
;@"========================================================================="
.not_at_address_1:
	ldr r1, =#0x3f215010								;@ Address of uart0 on a Pi2,3
	ldr r0, [r1]										;@ Fetch value at 0x3f215010
	cmp r2, r0											;@ Check if we have the value the uart0 will be at reset
	beq .At2ndAddress
;@"========================================================================="
@#     ** Auto-Detected failed, not safe to do anything but deadloop **
@#        Would love to be display an error state but no ability on Pi
;@"========================================================================="
.pi_detect_fail:
    b .pi_detect_fail
;@"========================================================================="
@#          Raspberry PI IO base address was detected as 0x3f000000
@#          RPi_IO_Base_Addr => 0x3f000000
;@"========================================================================="
.At2ndAddress:
	ldr r1, =RPi_BusAlias								;@ Address to store bus alias at
	mov r0, #0xC0000000									;@ Alias value is 0xC0000000
	str r0, [r1]										;@ Store alias address
	ldr r1, =RPi_IO_Base_Addr							;@ Address to store base IO address at
	mov r0, #0x3f000000									;@ Hold the detected address
	str r0, [r1]										;@ Store bus io base address
.autodetect_done:
;@"========================================================================="
@#     We are getting close to handing over to C so we need to copy the 
@#     ISR table to position 0x0000 so interrupts can be used if wanted 
;@"========================================================================="
	ldr r0, = _isr_Table								;@ Address of isr_Table
	mov r1, #0x0000										;@ Destination 0x0000
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
;@"========================================================================="
@#    Clear the .BSS segment as any C, C++ compiler expects us to do 
;@"========================================================================="
	ldr   r0, =__bss_start__							;@ Address of BSS segment start
	ldr   r1, =__bss_end__								;@ Address of BSS segement end
	mov   r2, #0										;@ Zero register R2
.clear_bss:
   	cmp   r0, r1										;@ If not at end address
    bge   .clear_bss_exit								;@ Finished clearing section 
   	str   r2, [r0]										;@ Store the zeroed register
	add   r0, r0, #4									;@ Add 4 to store address
   	b .clear_bss										;@ loop back
.clear_bss_exit:
;@"========================================================================="
@#  Finally that all done jump to the entry point .. we always call it main
;@"========================================================================="
   	bl main												;@ Call main
hang:
	b hang												;@ Hang if it returns from main call
.balign	4
.ltorg													;@ Tell assembler ltorg data for this code can go here

;@"*************************************************************************"
@#                      ISR TABLE FOR SMARTSTART			
;@"*************************************************************************"
_isr_Table:
    ldr pc, _reset_h
    ldr pc, _undefined_instruction_vector_h
    ldr pc, _software_interrupt_vector_h
    ldr pc, _prefetch_abort_vector_h
    ldr pc, _data_abort_vector_h
    ldr pc, _unused_handler_h
    ldr pc, _interrupt_vector_h
    ldr pc, _fast_interrupt_vector_h

_reset_h:                           	.word   _start
_undefined_instruction_vector_h:		.word   hang
_software_interrupt_vector_h:			.word   hang
_prefetch_abort_vector_h:           	.word   hang
_data_abort_vector_h:               	.word   hang
_unused_handler_h:                  	.word   hang
_interrupt_vector_h:                	.word   _irq_handler_stub
_fast_interrupt_vector_h:           	.word   hang	

;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#                  Re-entrant interrupt handler stub  
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka13552.html */
_irq_handler_stub:
	sub lr, lr, #4										;@ Use SRS to save LR_irq and SPSP_irq
    srsfd sp!, #0x13									;@ on to the SVC mode stack

   	cps #0x13											;@ Switch to SVC mode
    push {r0-r3, r12}									;@ Store AAPCS regs on to SVC stack

   	mov r1, sp
    and r1, r1, #4										;@ Ensure 8-byte stack alignment�
   	sub sp, sp, r1										;@ �adjust stack as necessary
    push {r1, lr}										;@ Store adjustment and LR_svc

	ldr r3, = RPi_IO_Base_Addr							;@ Address of Pi Base IO
	ldr r3, [r3]										;@ Fetch Pi IO base Address
	add r2, r3, #45568									;@ Add 0xB200
	ldr r1, [r2]										;@ IRQ->IRQBasicPending
	tst r1, #1											;@ If timer IRQ pending not yet pending
	beq .NoTimerIrq										;@ Nothing to do
	add r3, r3, #46080									;@ Add 0xB400
	mov r1, #1											;@ Load #1
	str r1, [r3, #12]									;@ Write to clear register
	ldr r3, [r2, #4]									;@ Fetch Irq pending
	bic r3, r3, #1										;@ Clear pending bit
	str r3, [r2, #4]									;@ Write value back
.NoTimerIrq:

    cpsie i												;@ Enable IRQ

  	ldr r0, =RPi_TimerIrqAddr							;@ Address to TimerIrqAddr
	ldr r0, [r0]										;@ Load TimerIrqAddr value
	cmp r0, #0											;@ compare to zero
	beq no_irqset										;@ If zero no irq set 
	blx r0												;@ Call Irqhandler that has been set  
no_irqset:	

    cpsid i												;@ Disable IRQ

	pop {r1, lr}										;@ Restore LR_svc
   	add sp, sp, r1										;@ Un-adjust stack

    pop {r0-r3, r12}									;@ Restore AAPCS registers
   	rfefd sp!											;@ Return from the SVC mode stack

;@"*************************************************************************"
@#                INTERNAL DATA FOR SMARTSTART NOT EXPOSED TO INTERFACE			
;@"*************************************************************************"
.section ".data.smartstart", "aw"
.balign 4

RPi_BusAlias: .4byte 0;									;@ Address offset between VC4 physical address and ARM address needed for all DMA
RPi_TimerIrqAddr : .4byte 0;							;@ Current set Timer Irq function call Address

;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#     	                 DATA FOR SMARTSTART32 EXPOSED TO INTERFACE 
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
.section ".data.smartstart32", "aw"
.balign 4

.globl RPi_IO_Base_Addr;								;@ Make sure Pi_IO_Base_Addr label is global
RPi_IO_Base_Addr : .4byte 0;							;@ Peripheral Base addr is 4 byte variable in 32bit mode

.globl RPi_BootAddr;									;@ Make sure RPi_BootAddr label is global
RPi_BootAddr : .4byte 0;								;@ CPU boot address is 4 byte variable in 32bit mode

.globl RPi_CPUBootMode;									;@ Make sure RPi_CPUBootMode label is global
RPi_CPUBootMode : .4byte 0;								;@ CPU Boot Mode is 4 byte variable in 32bit mode

.globl RPi_CpuId;										;@ Make sure RPi_CpuId label is global
RPi_CpuId : .4byte 0;									;@ CPU Id is 4 byte variable in 32bit mode

.globl RPi_CompileMode;									;@ Make sure RPi_CompileMode label is global
RPi_CompileMode : .4byte 0;								;@ Compile mode is 4 byte variable in 32bit mode

.globl RPi_CPUCurrentMode;								;@ Make sure RPi_CPUCurrentMode label is global
RPi_CPUCurrentMode : .4byte 0;							;@  CPU current Mode is 4 byte variable

;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#			IRQ HELPER ROUTINES PROVIDE BY RPi-SmartStart API		    
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

;@"========================================================================="
@#	setTimerIrqAddress -- Composite Pi1, Pi2 & Pi3 code
@#	C Function: TimerIrqHandler setTimerIrqAddress ( TimerIrqHandler* ARMaddress);
@#		ASSEMBLER:
@#			Entry: R0 will have new Timer IRQ function to set
@#			Return: R0 will return the old IRQ function that was set
;@"========================================================================="
.section .text.setTimerIrqAddress, "ax", %progbits
.balign	4
.globl setTimerIrqAddress;
.type setTimerIrqAddress, %function
setTimerIrqAddress:
    cpsid i												;@ Disable irq interrupts as we are clearly changing call
	ldr r1, =RPi_TimerIrqAddr							;@ Load address of function to call on interrupt 
	ldr r2, [r1]										;@ Load current irq call address
	str r0, [r1]										;@ Store the new function pointer address we were given
	mov r0, r2											;@ Return the old call function
	bx  lr												;@ Return
.balign	4
.ltorg													;@ Tell assembler ltorg data for this code can go here
.size	setTimerIrqAddress, .-setTimerIrqAddress


;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#		VC4 GPU ADDRESS HELPER ROUTINES PROVIDE BY RPi-SmartStart API	   
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

;@"========================================================================="
@#		ARMaddrToGPUaddr -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: uint32_t ARMaddrToGPUaddr (void* ARMaddress);
@#                     ASSEMBLER:
@#		Entry: R0 will have ARM Address value
@#                     Return: R0 will have GPU address for that value
;@"========================================================================="
.section .text.ARMaddrToGPUaddr, "ax", %progbits
.balign	4
.globl ARMaddrToGPUaddr;		
.type ARMaddrToGPUaddr, %function
ARMaddrToGPUaddr:
	ldr r1, =RPi_BusAlias
    ldr r1,[r1]											;@ Fetch bus alias	
	orr r0, r0, r1										;@ Create bus address
	bx   lr												;@ Return
.balign	4
.ltorg													;@ Tell assembler ltorg data for this code can go here
.size	ARMaddrToGPUaddr, .-ARMaddrToGPUaddr

;@"========================================================================="
@#		GPUaddrToARMaddr -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: uint32_t GPUaddrToARMaddr (uint32_t BUSaddress); 
@#                     ASSEMBLER:
@#		Entry: R0 will have GPUAddress value
@#                     Return: R0 will have ARM address for that value
;@"========================================================================="
.section .text.GPUaddrToARMaddr, "ax", %progbits
.balign	4
.globl GPUaddrToARMaddr;		
.type GPUaddrToARMaddr, %function
GPUaddrToARMaddr:
	ldr r1, =RPi_BusAlias
    ldr r1,[r1]											;@ Fetch bus alias	
	bic r0, r0, r1										;@ Create arm address
	bx   lr												;@ Return
.balign	4
.ltorg													;@ Tell assembler ltorg data for this code can go here
.size	GPUaddrToARMaddr, .-GPUaddrToARMaddr


;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#		  INTERRUPT HELPER ROUTINES PROVIDE BY RPi-SmartStart API	   
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

;@"========================================================================="
@#		EnableInterrupts -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: void EnableInterrupts (void);
@#                     ASSEMBLER:
@#		Entry: Nothing
@#		Return: Nothing
;@"========================================================================="
.section .text.EnableInterrupts, "ax", %progbits
.balign	4
.globl EnableInterrupts
.type EnableInterrupts, %function
EnableInterrupts:
	cpsie i												;@ Enable IRQ
	bx  lr												;@ Return
.size	EnableInterrupts, .-EnableInterrupts

;@"========================================================================="
@#		DisableInterrupts -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: void DisableInterrupts (void);
@#                     ASSEMBLER:
@#		Entry: Nothing
@#		Return: Nothing
;@"========================================================================="
.section .text.DisableInterrupts, "ax", %progbits
.balign	4
.globl DisableInterrupts
.type DisableInterrupts, %function
DisableInterrupts:
    cpsid i												;@ Disable IRQ
	bx  lr												;@ Return
.size	DisableInterrupts, .-DisableInterrupts
