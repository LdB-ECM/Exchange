#ifndef _xRTOS_CORE_H_
#define _xRTOS_CORE_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: xRTOS_core.h             									}
{       Copyright(c): Leon de Boer(LdB) 2020        						}
{       Multicore xRTOS Version: 2.00   									}
{																			}
{***************************************************************************}
{                                                                           }
{      Defines an API interface for a xRTOSv2 task implementation           }
{																            }
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  2.00 Initial version														}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>							// C standard unit for bool, true, false
#include <stdint.h>								// C standard unit for uint32_t etc
#include <stddef.h>
#include "xRTOS.h"								// Include xRTOS header

#define xRTOS_CORE_VERSION 2000					// xRTOS version number 2.00 build 0   

/*--------------------------------------------------------------------------}
{        DEFINE THE OS TICK_TIMER SIZE BASED ON CONFIG IN xRTOS.h           }
{--------------------------------------------------------------------------*/
#if (configUSE_16_BIT_TICKS == 1)				
/* At 1000 ticks per second,  16bit max delay would be 65 seconds  */
	typedef uint16_t TickType_t;				// OS Tick Count is 16 bit
#elif (configUSE_64_BIT_TICKS == 1)
/* At 1000 ticks per second, 64bit max delay would be 9223372036854775 seconds (153722867280912 min or 2562047788015 hours or 106751991167 days) */
	typedef uint64_t TickType_t;				// OS Tick Timer is 64 bit
#else
/* At 1000 ticks per second, 32bit max delay would be 4294967 seconds (71582 min or 1193 hours or 49 days) */
	typedef uint32_t TickType_t;				// OS Tick Timer is 32 bit
#endif

/*--------------------------------------------------------------------------}
{		    PREDEFINED SOME ERROR CODES RETURNS BY xRTOS SYSTEM             }
{--------------------------------------------------------------------------*/
#define        ENOMEM           ( 1 )			// Out of memory
#define        EINVAL           ( 2 )			// Invalid argument

/*--------------------------------------------------------------------------}
{		    DEFINE SOME OPAQUE POINTER TYPES USED BY xRTOS SYSTEM           }
{--------------------------------------------------------------------------*/
typedef struct task_struct* TaskHandle_t;       // Opaque pointer to a task struct which is defined in xRTOS_core.c
extern struct core_struct* core_ptr[];			// Exposed core pointers for irq and ctxsw code

/***************************************************************************}
{					    PUBLIC INTERFACE ROUTINES						    }
****************************************************************************/

/*-[ xRTOS_Init ]-----------------------------------------------------------}
.  Initializes xRTOS system and must be called before any other xRTOS call
.--------------------------------------------------------------------------*/
void xRTOS_Init (void);


/*-[ xTaskCreate ]----------------------------------------------------------}
.  Creates an xRTOS task and returns handle in pxCreatedTask if not null.
.  The task handle can be retreived at a later time by using xTaskSelf. 
.  RETURN: 0 if task created success, any other value is the error code
.--------------------------------------------------------------------------*/
int xTaskCreate ( uint8_t corenum,
	              void (*pxTaskCode) (void* pxParam),				// The code for the task
				  const char* const pcName,							// The character string name for the task
				  const unsigned int usStackDepth,					// The stack depth in register size for the task stack 
				  void* const pvParameters,							// Private parameter that may be used by the task
				  uint8_t uxPriority,								// Priority of the task
				  TaskHandle_t* const pxCreatedTask);				// A pointer to return the task handle (NULL if not required)  

/*-[ xTaskSelf ]------------------------------------------------------------}
.  Returns the calling task handle from the task making call
.--------------------------------------------------------------------------*/
TaskHandle_t xTaskSelf (void);

/*-[ xTaskDelay ]-----------------------------------------------------------}
.  Changes running task to sleeping and sets wake-up in given time period.
.  The task while in the sleeping state uses no CPU time
.--------------------------------------------------------------------------*/
int xTaskDelay (const TickType_t ticks);

/*-[ xTaskSuspend ]---------------------------------------------------------}
.  Suspends the given task handle preventing it getting any CPU time. If
.  the task handle is NULL it suspends the current calling task. Calls to
.  xTaskSuspend are not accumulative, calling xTaskSuspend twice on the
.  same task still only requires one call to xTaskResume to release task.
.--------------------------------------------------------------------------*/
void xTaskSuspend (TaskHandle_t xTaskToSuspend);

/*-[ xTaskResume ]----------------------------------------------------------}
.  Resumes a given suspended task handle to running state. The task handle
.  can not be NULL because a suspended task would never get CPU time.
.  One call to xTaskResume will release any number of xTaskSuspend calls to
.  the same task as they are not accumulative.
.  RETURN: 0 for no error, EINVAL for an invalid task handle
.--------------------------------------------------------------------------*/
int xTaskResume (TaskHandle_t xTaskToResume);

/*-[ xTaskStartScheduler ]--------------------------------------------------}
.  Starts the xRTOS task scheduler effectively starting the whole system
.--------------------------------------------------------------------------*/
void xTaskStartScheduler(void);

/*-[ xTaskGetNumberOfTasks ]------------------------------------------------}
.  Returns the number of xRTOS tasks assigned to the core this is called
.--------------------------------------------------------------------------*/
unsigned int xTaskGetNumberOfTasks (void);

/*-[ xLoadPercentCPU ]------------------------------------------------------}
.  Returns the load on the core this is called from in percent (0 - 100)
.--------------------------------------------------------------------------*/
unsigned int xLoadPercentCPU(void);

#ifdef __cplusplus
}
#endif
#endif /* INC_TASK_H */



