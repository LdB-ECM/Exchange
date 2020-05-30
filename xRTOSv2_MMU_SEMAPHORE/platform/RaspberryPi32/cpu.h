#ifndef _CPU_H_
#define _CPU_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: cpu.h														}
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
    
#define CPU_DRIVER_VERSION 1000					// Version number 1.00 build 0

#define NR_CORES      ( 4 )                     // Pi has 4 cores

/*--------------------------------------------------------------------------}
{                EVERY CPU PORT MUST DEFINE A REGISTER SIZE                 }
{--------------------------------------------------------------------------*/
typedef uint32_t RegType_t;                     // Define RegType_t as uint32_t for 32bit Pi operation

/*--------------------------------------------------------------------------}
{            EVERY CPU PORT MUST DEFINE A CONTEXT SWITCH STRUCT             }
{--------------------------------------------------------------------------*/
typedef struct
{
    RegType_t CPSR;                             // CPSR mode to return task operation back with
    RegType_t retAddr;                          // Return address to return task operation back from fiq
    RegType_t r0;                               // R0 value to return task operation back with
    RegType_t r1;                               // R1 value to return task operation back with
    RegType_t r2;                               // R2 value to return task operation back with
    RegType_t r3;                               // R3 value to return task operation back with
    RegType_t r4;                               // R4 value to return task operation back with
    RegType_t r5;                               // R5 value to return task operation back with
    RegType_t r6;                               // R6 value to return task operation back with
    RegType_t r7;                               // R7 value to return task operation back with
    RegType_t r8;                               // R8 value to return task operation back with
    RegType_t r9;                               // R9 value to return task operation back with
    RegType_t r10;                              // R10 value to return task operation back with
    RegType_t r11;                              // R11 value to return task operation back with
    RegType_t r12;                              // R12 value to return task operation back with
    RegType_t r13;                              // SP value to return task operation back with
    RegType_t r14;                              // LR value to return task operation back with
    RegType_t exitAddr;                         // Exit address to return when task completes operation
} CtxSwitch_t;

/*--------------------------------------------------------------------------}
{          EVERY CPU PORT MUST DEFINE STACK ALIGNMENT AND DIRECTION         }
{--------------------------------------------------------------------------*/
#define portBYTE_ALIGNMENT			( 8 )       // Stack must be 8 byte aligned
#define portSTACK_DIRECTION         ( 0 )       // Stack moves down on ARM6,7,8 so a 0


#define portSTACK_IDLE_SIZE    	    ( 256 )     // Default idle stack size is 256 RegType_t (min size of a context switch stack) 
#define portSTACK_MAX_SIZE          ( 4096 )    // Max stack size for a all tasks is 4096 RegType_t

/*--------------------------------------------------------------------------}
{    EVERY CPU PORT MUST DEFINE 16, 32 OR 64 BIT OS TIMER TICK OPERATION    }
{--------------------------------------------------------------------------*/
/* 16bit max delay would be 65 seconds at 1000 ticks per second */
/* 32bit max delay would be 4294967 seconds (71582 min or 1193 hours or 49 days) */
/* 64bit max delay would be 9223372036854775 seconds (153722867280912 min or 2562047788015 hours or 106751991167 days) */
#define configUSE_16_BIT_TICKS		( 0 )       // We are using 32 bit timers so not 16 bit timers
#define configUSE_64_BIT_TICKS		( 0 )       // We are using 32 bit timers so not 64 bit timers

#define disable_irq()	__asm volatile("cpsid i")
#define enable_irq()	__asm volatile("cpsie i")
#define disable_fiq()	__asm volatile("cpsid f")
#define enable_fiq()	__asm volatile("cpsie f")
   
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{					    PUBLIC INTERFACE ROUTINES				   			}
{***************************************************************************/

/*--------------------------------------------------------------------------}
{             EVERY CPU PORT MUST DEFINE THESE TIMER FUNCTIONS              }
{--------------------------------------------------------------------------*/

/*-[ timer_init ]-----------------------------------------------------------}
.  Sets up the context switch timer tick interrupt to the rate requested
.--------------------------------------------------------------------------*/
void timer_init ( uint16_t TickRateHz );

/*-[ timer_clear_irq ]------------------------------------------------------}
.  Clears the timer tick interrupt
.--------------------------------------------------------------------------*/
void timer_clear_irq (void);

/*--------------------------------------------------------------------------}
{             EVERY CPU PORT MUST DEFINE THESE STACK FUNCTIONS              }
{--------------------------------------------------------------------------*/

/*-[ InitializeTaskStack ]--------------------------------------------------}
.  Initialize the task stack ready for entry at pxCode with a parameter of
.  pvParameter. The registers have a simple pattern for aid in debug checks.
.  RETURN: Pointer to the new Top of Stack after the stack initialized.
.--------------------------------------------------------------------------*/
bool InitializeTaskStack ( CtxSwitch_t* ctx, const RegType_t* pxTopOfStack, void (*pxcode)(void*), void *pvParameters );


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif