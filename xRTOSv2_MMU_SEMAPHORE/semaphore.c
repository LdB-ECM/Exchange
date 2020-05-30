/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: semaphore.c												}
{       Copyright(c): Leon de Boer(LdB) 2020        						}
{       Version: 1.00														}
{																			}
{***************************************************************************}
{                                                                           }
{    Defines an API interface for semaphore creation on xRTOSv2 CPU ports   }
{																            }
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  1.00 Initial version														}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>							// C standard unit for bool, true, false
#include <stdint.h>								// C standard unit for uint32_t etc    
#include "xRTOS.h"
#include "task.h"
#include "Semaphore.h"

#if SEMAPHORE_VERSION != 1000
#error "Header does not match this version of file"
#endif

   
static bool test_and_set (volatile bool* lock)
{
    disable_fiq();                   // Can not allow preempting in here 
    volatile bool initial = *lock;  // Read and hold initial value
    *lock = true;                   // Set lock to new value
    enable_fiq();                   // Can allow preempting again now
    return initial;                 // Return the initial value
}

void xSemaphoreBinaryInit (SemaphoreHandle_t *sem)
{
    if (sem)
    {
        sem->lock = false;
        sem->val = 1;
        sem->queue = 0;
    }
}

void xSemaphoreWait (SemaphoreHandle_t *sem)
{
    while(test_and_set(&sem->lock)){};
    sem->val--;
    if (sem->val < 0) 
    {         
        sem->queue = xTaskSelf();
        sem->lock = false;    
        xTaskSuspend(0);     
    }     
    sem->lock = false;
}

void xSemaphoreSignal (SemaphoreHandle_t *sem)
{
    while(test_and_set(&sem->lock)){};
    sem->val++;
    if (sem->val <= 0) 
    {         
        if (sem->queue != 0) 
        {
            xTaskResume(sem->queue);
            sem->queue = 0;
        }   
    }  
    sem->lock = false;
}

