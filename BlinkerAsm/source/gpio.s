;@"========================================================================="
@#		  	 RPi_IO_Base_Addr provided by SmartStart32.s
;@"========================================================================="
.extern RPi_IO_Base_Addr;								;@ Auto detected PI base address

;@"-[gpio_output]-----------------------------------------------------------"
;@#		Given a valid GPIO port the output is set high(true) or Low (false)
;@#		ASSEMBLER:
;@#			Entry: R0 = GPIO port (0 to 53),  R1 = 0 (off), 1 (On)
;@#			Return: R0 = 0 (fail)   R0 = 1 (success)  
;@"-------------------------------------------------------------------------"
.section .text.gpio_output, "ax", %progbits
.balign	4
.globl gpio_output;
.type gpio_output, %function
gpio_output:	
    cmp r0, #54											;@ GPIO port number can only be 0..53
	blt .SetClr_GPIO_Valid
	mov r0, #0											;@ Outside valid port number return fail
	bx   lr												;@ Return with fail result
.SetClr_GPIO_Valid:
    cmp r0, #32											;@ Bank 0 is 0..31, Bank 1 is 32 .. 53
	bge .SetClr_GPIO_Bank1
    mov r3, #0x1C										;@ BANK 0 GPIO_BIT_SET offset
    cmp r1, #0 
	beq .SetClr_GPIO     
	mov r3, #0x28										;@ BANK 0 GPIO_BIT_CLR offset
    b .SetClr_GPIO
.SetClr_GPIO_Bank1:
    sub r0, r0, #32										;@  GPIO number % 32
    mov r3, #0x20										;@ BANK 1 GPIO_BIT_SET offset
    cmp r1, #0
	beq .SetClr_GPIO     
	mov r3, #0x2C										;@ BANK 1 GPIO_BIT_CLR offset
.SetClr_GPIO:
    ;@#REGISTERS:  R0 = GPIO port number modulo 32, R3 = GPIO set/clr register offset
  	ldr r1, =RPi_IO_Base_Addr
	ldr r1, [r1]
	add r1, r1, #0x200000								;@ Create GPIO base offset
	mov r2, #1											;@ 1 bit to shift
	lsl r0, r2, r0										;@ shift it by modulo 32 GPIO port number
	str r0, [r1, r3]									;@ Store bit mask  at GPIO base addr + Register offset
	mov r0, #1											;@ return success		  
	bx   lr												;@ Return.
.size	gpio_output, .-gpio_output

;@"-[gpio_setup]------------------------------------------------------------"
;@#		Given a valid GPIO port and mode sets GPIO to given mode
;@#		ASSEMBLER:
;@#			Entry: R0 = GPIO port (0 to 53),  R1 = Mode (0 - 7)
;@#			Return: R0 = 0 (fail)   R0 = 1 (success)  
;@"-------------------------------------------------------------------------"
.section .text.gpio_setup, "ax", %progbits
.balign	4
.globl gpio_setup;
.type gpio_setup, %function
gpio_setup:	
    cmp r0, #54											;@ GPIO port number can only be 0..53
	blt .SetFunc_GPIO_Valid
	mov r0, #0											;@ Outside valid port number return fail
	bx   lr												;@ Return with fail
.SetFunc_GPIO_Valid:
    push {r4, r5, fp, lr}								;@ Keep the callee saved registers
	mov r2, #0											;@ Start bank count at 0
.BankCalcLoop:
    cmp r0, #10											;@ Bank 0 is 0..31, Bank 1 is 32 .. 53
	blt .Bank_Calc_Done
	add r2, r2, #1										;@ Inc Bank
	sub r0, r0, #10;									;@ Subtract 10 from gpio number
	b .BankCalcLoop
.Bank_Calc_Done:  
	add r3, r0, r0
	add r3, r3, r0										;@ r3 = shift value = (gpio num % 10) * 3						
	;@#REGISTERS:  R2 = bank,  R3 = GPIO modulo shift 0..27  (shift = (gpio num % 10) * 3)
	mov r4, #04
	mul r0, r4, r2										;@ Port offset is simply 0x04 * bank  (r4 is set)
    mov r2, #7  
	lsl r2, r2, r3										;@ shift mask from GPIO number created (r2 is set)
	lsl r3, r1, r3										;@ GPIO mode shifted by r3 and moved to r3  (r3 set)
	;@#REGISTERS: R2 = shift mask from GPIO number,  R3 = FSEL_GPIO_TYPE mask shifted from GPIO number,  R4 = GPIO port offset
	ldr r5, =RPi_IO_Base_Addr
	ldr r5, [r5]
	add r5, r5, #0x200000								;@ GPIO BASE OFFSET
	ldr r1, [r5, r0]									;@ Read current FSELx 
	bic r1, r1, r2										;@ AND read value with the "NOT" of shift mask
	orr r1, r1, r3										;@ ORR shifted FSEL_GPIO_TYPE bit mask onto R1
	str r1, [r5, r0]									;@ Write r1 to FSELx 
	pop {r4, r5, fp, lr}								;@ Restore the callee saved registers.
	mov r0, #1											;@ return success		  
	bx   lr												;@ Return
.balign 4
.ltorg													;@ Tell assembler its ok to put ltorg data for code above here
.size	gpio_setup, .-gpio_setup
