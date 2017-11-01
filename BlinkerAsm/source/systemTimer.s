;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
;@"																		    "			
;@"      Filename: systemTimer.s										    "
;@"      Copyright(c): Leon de Boer(LdB) 2017						        "
;@"      Version: 1.01													    "
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
;@"      This code is designed to extend the SmartStart bootstub code for   "
;@"  the Pi 1/2/3. It utilizes the RPi_IO_Base_Addr address auto detected   "
;@"  by smartstart adding the system timer specific offsets to give access  "
;@"  to various timing routines.											"
;@"										                                    "
;@"+++++++++++++++++++++++[ REVISIONS ]+++++++++++++++++++++++++++++++++++++"
;@"  1.01 Initial release .. Pi1, Pi2, Pi3 supported                        "
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"


;@"========================================================================="
@#		  	 RPi_IO_Base_Addr provided by SmartStart32.s
;@"========================================================================="
.extern RPi_IO_Base_Addr;								;@ Auto detected PI base address

;@"-[timer_wait]----------------------------------------------------"
;@#	  Using 1Mhz ARM system timer tick count waits specified usec
;@#		ASSEMBLER:
;@#			Entry: R0 = LOW 32 bits wait time   R1 = HIGH 32 bits of wait time
;@#			Return: Nothing  
;@"-------------------------------------------------------------------------"
.section .text.timer_wait, "ax", %progbits
.balign	4
.globl timer_wait;
.type timer_wait, %function
timer_wait:	
        stmfd   sp!, {r4, r8, r9}
        mov     r8, r0
        mov     r9, r1									;@ Hold delay in r8 & r9
        ldr     r3, =RPi_IO_Base_Addr					;@ Address of IO base address storage
        ldr     r3, [r3]								;@ Load RPi_IO_Base_Addr value
        add     r3, r3, #0x3000							;@ Add 0x3000 offset = Ssytem Timer
.HiTimerMoved:
		ldr     r2, [r3, #8]							;@ Read timer hi count
        ldr     r4, [r3, #4]							;@ Read timer lo count
        ldr     r1, [r3, #8]							;@ Re-Read timer hi count
        cmp     r2, r1
        bne     .HiTimerMoved
        adds    r8, r8, r4;								;@ Add least significant 32 bits
		adcs    r9, r9, r1;								;@ Add the high 32 bits with carry (Adds current time to us delay .. creating timeout value )
.HiTimerMoved1:
        ldr     r2, [r3, #8]							;@ Read timer hi count
        ldr     r4, [r3, #4]							;@ Read time lo count
        ldr     r1, [r3, #8]							;@ Re-raed timer hi count
        cmp     r2, r1
        bne     .HiTimerMoved1							;@ If timer hi count moved re-read
		cmp     r1, r9
        cmpeq   r4, r8
		bcc		.HiTimerMoved1							;@ Timeout not reached
        ldmfd   sp!, {r4, r8, r9}
		bx lr;
  .ltorg												;@ Tell assembler ltorg data for this code can go here
.size	timer_wait, .-timer_wait
