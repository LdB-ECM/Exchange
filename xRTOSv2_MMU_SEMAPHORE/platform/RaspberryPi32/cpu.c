/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: cpu.c														}
{       Copyright(c): Leon de Boer(LdB) 2020        						}
{       Version: 1.00														}
{																			}
{***************************************************************************}
{                                                                           }
{      Defines an API interface for the 32Bit Raspberry Pi Models           }
{																            }
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  1.00 Initial version														}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>							// C standard unit for bool, true, false
#include <stdint.h>								// C standard unit for uint32_t etc
#include "cpu.h"                                // Include this units header

#if CPU_DRIVER_VERSION != 1000
#error "Header does not match this version of file"
#endif

/*
OFFSET        CONTEXT SWITCH STACK LAYOUT
======	      ===========================
0			cpsr	<- program status register to use on this task when running
4			retaddr <- The address to return to when running
8			r0		<- r0	C caller argument and scratch register & result register
12			r1		<- r1	C caller argument and scratch register & result register
16			r2		<- r2	C caller argument and scratch register
20			r3		<- r3	C caller argument and scratch register
24			r4		<- r4	C callee-save register
28			r5		<- r5	C callee-save register
32			r6		<- r6	C callee-save register
36			r7		<- r7	C callee-save register
40			r8		<- r8	C callee-save register
44			r9		<- r9	might be a callee-save register or not (on some variants of AAPCS it is a special register)
48			r10		<- r10	C callee-save register
52			r11		<- r11	C callee-save register
56			r12		<- likely unused but save just in case
60			r13     <- The sp value prior to the context switch
64			r14		<- The lr value prior to the context switch (which is usually a subroutne branch)
*/

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			                PRIVATE INTERNAL DATA                 			}
{***************************************************************************/
static uint32_t m_nClockTicksPerHZTick = 0;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			                PRIVATE INTERNAL ROUTINES             			}
{***************************************************************************/

/* Assembler code defined in timer.S */
extern RegType_t EL0_Timer_Frequency(void);		
/* Assembler code defined in timer.S */
extern void EL0_Timer_Set(RegType_t nextCount);
/* Assembler code defined in timer.S */
extern bool EL0_Timer_Fiq_Setup(void);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			                PUBLIC INTERFACE ROUTINES             			}
{***************************************************************************/

/*-[ timer_init ]-----------------------------------------------------------}
.  Sets up the context switch timer tick and FIQ to the rate requested
.--------------------------------------------------------------------------*/
void timer_init (uint16_t TickRateHz)
{
	if (m_nClockTicksPerHZTick == 0)								// Divisor not yet calculated
		m_nClockTicksPerHZTick = (EL0_Timer_Frequency() / TickRateHz);// Calculate divisor for req ctxsw frequency
	EL0_Timer_Set(m_nClockTicksPerHZTick);							// Set the divisor
	EL0_Timer_Fiq_Setup();											// Setup the ctxswitch fiq timer interrupt
}

/*-[ timer_clear_irq ]------------------------------------------------------}
.  Clears the timer tick interrupt
.--------------------------------------------------------------------------*/
void timer_clear_irq (void)
{
	EL0_Timer_Set(m_nClockTicksPerHZTick);
}

/*-[ InitializeTaskStack ]--------------------------------------------------}
.  Initialize the task stack ready for entry at pxCode with a parameter of
.  pvParameter. The registers have a simple pattern for aid in debug checks.
.  RETURN: Pointer to the new Top of Stack after the stack initialized.
.--------------------------------------------------------------------------*/
bool InitializeTaskStack (CtxSwitch_t* ctx, const RegType_t* pxTopOfStack, void (*pxcode)(void*), void *pvParameter )
{
	if (ctx && pxTopOfStack && pxcode)
	{
		ctx->retAddr = (RegType_t)pxcode;
		ctx->CPSR = 0x1F;
		ctx->r14 = (RegType_t)pxcode;
		ctx->r13 = (RegType_t)pxTopOfStack;
		ctx->r12 = 0xCCCC;
		ctx->r11 = 0xBBBB;
		ctx->r10 = 0xAAAA;
		ctx->r9 = 0x9999;
		ctx->r8 = 0x8888;
		ctx->r7 = 0x7777;
		ctx->r6 = 0x6666;
		ctx->r5 = 0x5555;
		ctx->r4 = 0x4444;
		ctx->r3 = 0x3333;
		ctx->r2 = 0x2222;
		ctx->r1 = 0x1111;
		ctx->r0 = (RegType_t)pvParameter;
		ctx->exitAddr = 0xDEADBEEF; // Not supported yet
		return true;
	}
	return false;
}

