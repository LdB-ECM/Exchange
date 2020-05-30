
# xRTOSv2 ... PI 2,3 AARCH32
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card and place in Pi to test.
To compile yourself will need to change the directory at top of makefile to match the compiler path on your machine. 
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/xRTOS_SEMS.jpg?raw=true)
>
So this is a rework of the xRTOS system to give greater stack protect and work with either software of hardware semaphores.
You can engage software semaphore by simply uncommenting the #define at line 10 of main.c, they are mainly used for debugging because they work with and without MMU.

At the top of each task the core registers are now held there directly not down on the stack, the struct is defined in cpu.h but it sets up this as a block of memory

   OFFSET    |  CONTEXT SWITCH STACK LAYOUT
------------ | -------------
0			    | cpsr		  = program status register to use on this task when running
4            |	retaddr	  = The address to return to when running
8			    | r0			  = r0	C caller argument and scratch register & result register
12			    | r1			  = r1	C caller argument and scratch register & result register
16			    | r2			  = r2	C caller argument and scratch register
20			    | r3			  = r3	C caller argument and scratch register
24			    | r4			  = r4	C callee-save register
28			    | r5			  = r5	C callee-save register
>
32			r6			<- r6	C callee-save register
>
36			r7			<- r7	C callee-save register
>
40			r8			<- r8	C callee-save register
>
44			r9			<- r9	might be a callee-save register or not (on some variants of AAPCS it is a special register)
>
48			r10			<- r10	C callee-save register
>
52			r11			<- r11	C callee-save register
>
56			r12			<- likely unused but save just in case
>
60			r13			<- The sp value prior to the context switch 
>
64			r14			<- The lr value prior to the context switch (which is usually a subroutne branch)
>
68			exitAddr	<- Exit address to return when task completes operation

The fiq interrupt saves the current task stack when entering and the switches to the FIQ stack during all the interrupt handler. This means none of the fiq interrupt handler uses the task stack anymore.
