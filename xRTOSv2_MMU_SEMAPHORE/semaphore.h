#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif
    
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: semaphore.h												}
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
    
#define SEMAPHORE_VERSION 1000					// Version number 1.00 build 0
    
struct semaphore_t
{
  volatile int8_t val;
  volatile bool lock;
  volatile TaskHandle_t queue;
};

typedef struct semaphore_t SemaphoreHandle_t;                    


void xSemaphoreBinaryInit (SemaphoreHandle_t *sem);

void xSemaphoreWait(SemaphoreHandle_t *sem);
#define xSempaphoreTake(a) xSemaphoreWait(a)          // xSemaphoreTake is the same as xSemaphoreWait
void xSemaphoreSignal(SemaphoreHandle_t *ssem);
#define xSempaphoreGive(a) xSemaphoreSignal(a)        // xSemphoreGive is the same as xSemaphoreSignal


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif
#endif /*SEMAPHORE_H*/

