#ifndef _xRTOS_H_
#define _xRTOS_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: xRTOS.h           										}
{       Copyright(c): Leon de Boer(LdB) 2020        						}
{       xRTOS Version: 2.00				                                    }
{																			}
{***************************************************************************}
{                                                                           }
{     Defines an API interface that defines xRTOS config values             }
{																            }
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  2.00 Initial version														}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>							// C standard unit for bool, true, false
#include <stdint.h>								// C standard unit for uint32_t etc

#ifdef __XC16                                   // XC16 must be PIC24F port
#define CPU_HEADER "platform\pic24f\cpu.h"
#endif

#ifdef __XC32                                   // XC32 must be PIC32mx port
#define CPU_HEADER "platform\pic32mx\cpu.h"
#endif

#if defined(__arm__)							// GCC must be compiling for an ARM cpu

#if defined(__aarch64__)						// AARCH64 BIT
#define CPU_HEADER "platform/RaspberryPi64/cpu.h" // Raspberry Pi in 64bit mode
#else											// AARCH32 BIT
#define CPU_HEADER "platform/RaspberryPi32/cpu.h" // Raspberry Pi in 32bit mode
#endif
#endif

#include CPU_HEADER                             // Okay include defined cpu header which defines RegType_t and CtxSwitch_t     

#define NR_TASKS	  ( 8 )                    // xRTOSv2 has maximum 8 tasks per core    

#ifndef NR_CORES                               // If the number of cores has not been defined in CPU.h
#define NR_CORES      ( 1 )                     // Default is 1 core
#endif  

#ifndef TICK_RATE_HZ                            // If the tick rate has not been defined in CPU.h
#define TICK_RATE_HZ  ( 1000 )                  // Default timer tick rate of 1000 Hz
#endif  

#ifndef NR_TASKNAME                             // If the length of tasknames is not defined in CPU.h
#define NR_TASKNAME   ( 16 )                    // Default is 16 characters
#endif  

#ifndef configUSE_16_BIT_TICKS                  // If 16 bit timer size has not been defined in CPU.h
#define configUSE_16_BIT_TICKS		( 0 )       // Assume default timer tick size to 32 bit
#endif  
#ifndef configUSE_64_BIT_TICKS                  // If 64 bit timer size has not been defined in CPU.h
#define configUSE_64_BIT_TICKS		( 0 )       // Assume default timer tick size to 32 bit
#endif  

#define tskIDLE_PRIORITY						( 0	)				// Idle priority is 0 .. rarely would this ever change	
#define configMINIMAL_STACK_SIZE				( 128 )				// Minimum stack size used by idle task

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{         SOME CRITICAL DEFINITION CHECKS THAT MUST BE IN CPU.H       		}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef portSTACK_DIRECTION
#error Missing definition:  portStack_DIRECTION must be defined in cpu.h as either 1 (positive) or 0 (negative).  
#endif

#ifndef portSTACK_IDLE_SIZE
#error Missing definition:  portSTACK_IDLE_SIZE must be define in cpu.h which is the idle task stack size for each core   
#endif

#ifndef portSTACK_MAX_SIZE
#error Missing definition:  portSTACK_MAX_SIZE must be define in cpu.h which is the stack size for all tasks on all cores  
#endif

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif 

