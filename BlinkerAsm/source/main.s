
.globl main
main:
	mov r0, #18         ;@ GPIO 18
	mov r1, #1			;@ Ouput mode
	bl gpio_setup		;@ Set that up

.flashLoop:
	mov r0, #18			;@ GPIO 18
	mov r1, #0			;@ ON  (0 = On on Pi3)
	bl gpio_output      ;@ Output that

	ldr r0, =#1000000	;@ Wait 1000000 usec AKA 1sec
	mov r1, #0
	bl timer_wait		;@ Execute wait

	mov r0, #18			;@ GPIO 18
	mov r1, #1			;@ Off  (1 = Off on Pi3)
	bl gpio_output	    ;@ Output that

	ldr r0, =#1000000	;@ Wait 1000000 usec AKA 1 sec
	mov r1, #0
	bl timer_wait		;@ Output that

	b .flashLoop
