/******************************************************************************
*	frameBuffer.s
*	 by Alex Chadwick
*
*	A sample assembly code implementation of the screen01 operating system.
*	See main.s for details.
*
*	frameBuffer.s contains code that creates and manipulates the frame buffer.
******************************************************************************/


/* LdB
    The info from Alex about page alignment is very very wrong.
    The structure is simply sent via DMA to GPU and must be aligned to 16 byte boundary to do that
    So what it is saying is the last 4 bits in the address must be 0 so the address is of form 0xFFFFFFF0
    Where the F represent can can any valid hex but the last nibble is always zero

	Next the structure needs to be as that described in
	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
*/

.section .data
.balign 16
.globl FrameBufferInfo 
FrameBufferInfo:
	.int FrameBufferInfoEnd - FrameBufferInfo; /* size of structure */
	.int 0                      /* mailbox response */	
	.int 0x00048003				/* Set screen dimensions command */
	.int 8						/* Size of buffer */
	.int 8						/* Size of data */
FBLabel1:                       /* Marks width/height entries so I can access */
	.int 1024					/* Screen Width */
	.int 768					/* screen Height */
	.int 0x00048004				/* Set screen virtual dimensions command */
	.int 8						/* Size of buffer */
	.int 8						/* Size of data */
FBLabel2:						/* Marks vwidth/vheight entries so I can access */
	.int 1024					/* vWidth */
	.int 768					/* vHeight */
	.int 0x00048005				/* Set screen depth */
	.int 4						/* Size of buffer */
	.int 4						/* Size of data */
FBLabel3:						/* Marks depth entry so I can access */
	.int 16						/* Colour Depth of screen */
	.int 0x00040001				/* Get frame buffer */
	.int 8						/* Size of buffer */
	.int 4						/* Size of data */
FBLabel4:						/* Marks framebuffer entry so I can access */
	.int 16						/* align 16 .. respose 0 */
	.int 0						/* buffer  .. response 1 */
	.int 0						/* end of strcuture */
FrameBufferInfoEnd:

/* NEW
	This just patches the code as used by Alex 
*/
.section .text
.globl InitialiseFrameBuffer
InitialiseFrameBuffer:
	width .req r0
	height .req r1
	bitDepth .req r2
	cmp width,#4096
	cmpls height,#4096
	cmpls bitDepth,#32
	result .req r0
	movhi result,#0
	movhi pc,lr

	push {r4,lr}	
	fbInfoAddr .req r4		
	ldr fbInfoAddr,=FrameBufferInfo
	str width,[r4,#FBLabel1-FrameBufferInfo]       // Change structure width to provided
	str height,[r4,#FBLabel1-FrameBufferInfo+4]	   // Change structure height to provided
	str width,[r4,#FBLabel2-FrameBufferInfo]       // Change structure vwidth to provided
	str height,[r4,#FBLabel2-FrameBufferInfo+4]	   // Change structure vheight to provided
	str bitDepth,[r4,#FBLabel3-FrameBufferInfo+4]  // Change structure colour depth to provided
	.unreq width
	.unreq height
	.unreq bitDepth

	mov r0,fbInfoAddr
	orr r0,#0xC0000000                            // Pi 2 alias
	mov r1,#1
	bl MailboxWrite
	
	mov r0,#1
	bl MailboxRead

	ldr r4,[r4,#FBLabel4-FrameBufferInfo]			// Framebuffer address returned	

	teq result,#0
	movne result,#0
	popne {r4,pc}

	mov result,fbInfoAddr
	pop {r4,pc}
	.unreq result
	.unreq fbInfoAddr
