/* these .equ lines are just like #define lines for the assembler */
.equ CORE0, 0x0
.equ CORE1, 0x1
.equ CORE2, 0x2
.equ CORE3, 0x3 
.equ CORE0_MAILBOX0, 0x400000CC	     // As we are using the hardware mailbox +0x40 from write see sheet A47
.equ CORE1_MAILBOX0, 0x400000DC      // As we are using the hardware mailbox +0x40 from write see sheet A47
.equ CORE2_MAILBOX0, 0x400000EC      // As we are using the hardware mailbox +0x40 from write see sheet A47
.equ CORE3_MAILBOX0, 0x400000FC      // As we are using the hardware mailbox +0x40 from write see sheet A47

/* here starts our .text.startup where our startup code is present */
.section ".text.startup" 
.globl _start
.balign 4
_start: 

/*
 Here what we will do first is we allocate  stack for each and every core. By doing this each and every core will get its own stack
 Here i am giving a 512 bytes of stack. It is just a vague number which i took.
how do we achieve this? 
1. check which core it is either core0, core1, core2, core3 by reading the multi processor affinity id register
---> for more information see this folder readme.md note 1 
2. r0 stores the stack starting address 
3. so for core0 r0 holds __stack_pointer_core0  address and for core 1 r0 holds __stack_pointer_core1 etc.,. 
4. __stack_pointer_core0, __stack_pointer_core1 etc are defined in the linker script for now just think them as address in the memory 
5. we will store those values in stack pointer register [SP] and this can be seen at the label set_stack_func 
6. So finally we now have got four different stacks for four cores.
*/
	ldr r4, =CORE0					
	ldr r0, =__stack_pointer_core0
	mrc p15,0,r5,c0,c0,5  				
	ands r5, r5, #0x3				
	cmp r4,r5					
	beq set_stack_func				
	ldr r0,=__stack_pointer_core1
	ldr r4, =CORE1
	cmp r4,r5
	beq set_stack_func
	ldr r0,=__stack_pointer_core2
	ldr r4, =CORE2
	cmp r4,r5
	beq set_stack_func
	ldr r0,=__stack_pointer_core3
	
	ldr r1, =CoreReadyCount   // address of CoreReadyCount
	ldr r2, [r1]
	add r2, #1                // Increment CoreReadycount
	str r2, [r1]

set_stack_func:
	mov sp,r0
		
/*
Disclaimer : for those who know GPU mailboxes and for their information these are not those GPU mailboxes
	     instead they are just memory location . For those who dont know GPU mailboxes ignore this disclaimer
	    
After setting up the stack now we need to make our cores execute some code and we need to direct them to that code. 
As our aim is to make diferrent cores to execute different code what we do is
how we do this ?
1. we will make our cores to loop over a single memory address. 
2. we will make one of our core (here core0) to write the memory location where they need to jump to and start executing the code  
3. These other cores (here core 1,core 2,core 3) will read the memory location which is named as CORE1_MAILBOX0  in the top of the file 
   and if its values is zero it will again read and repeat this untill some non zero value is written in those memory locations by some core(here core 0)
4. here in this example every other core except core 0 loops and core 0 jumps to main function and sets the value at CORE1_MAILBOX0 
   in the main function. 
*/
	b mailbox
 	loop_in_mailbox:
		ldr r0,[r2]              // every core comes and checks whether  the value specified at the mailbox address is 0 or non zero
		cmp r0,#0                // here it compares 
		beq loop_in_mailbox	 // if it is non-zero then it means that CORE-0 has written the address where our core needs 
		str r0,[r2]             // Clear the mailbox 	
	bx r0                       // jump to the particular address given by CORE-0 and start executing the program.

	// if it comes back we nee to remake r5 mask and drop thru mailbox setup again
	mrc p15,0,r5,c0,c0,5  				
	ands r5, r5, #0x3	

	mailbox:
	ldr r4, =CORE3
	ldr r2, =CORE3_MAILBOX0
	cmp r4,r5
	beq loop_in_mailbox
	ldr r4, =CORE2
	ldr r2,=CORE2_MAILBOX0
	cmp r4,r5
	beq loop_in_mailbox
	ldr r4, =CORE1
	ldr r2,=CORE1_MAILBOX0
	cmp r4,r5
	beq loop_in_mailbox
	bl main
.balign	4
.ltorg

.section ".data", "aw"
.balign 4

.globl CoreReadyCount;					// Make sure this label is global
CoreReadyCount : .4byte 0;			    // How many cores are ready in park position


