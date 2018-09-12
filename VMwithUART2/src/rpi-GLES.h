#ifndef _RPI_GLES_
#define _RPI_GLES_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: rpi-GLES.h												}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 1.01														}
{																			}		
{***************[ THIS CODE IS FREEWARE UNDER CC Attribution]***************}
{																            }
{     This sourcecode is released for the purpose to promote programming    }
{  on the Raspberry Pi. You may redistribute it and/or modify with the      }
{  following disclaimer and condition.                                      }
{																            }
{      The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES AS TO      }
{   PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR IMPLIED.            }
{   Redistributions of source code must retain the copyright notices to     }
{   maintain the author credit (attribution) .								}
{																			}
{***************************************************************************}
{                                                                           }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc
#include <math.h>								// Needed for trig
#include "rpi-smartstart.h"						// Need for mailbox

// Flags for allocate memory.
	enum {
		MEM_FLAG_DISCARDABLE = 1 << 0,				/* can be resized to 0 at any time. Use for cached data */
		MEM_FLAG_NORMAL = 0 << 2,					/* normal allocating alias. Don't use from ARM */
		MEM_FLAG_DIRECT = 1 << 2,					/* 0xC alias uncached */
		MEM_FLAG_COHERENT = 2 << 2,					/* 0x8 alias. Non-allocating in L2 but coherent */
		MEM_FLAG_L1_NONALLOCATING = (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT), /* Allocating in L2 */
		MEM_FLAG_ZERO = 1 << 4,						/* initialise buffer to all zeros */
		MEM_FLAG_NO_INIT = 1 << 5,					/* don't initialise (default is initialise to all ones */
		MEM_FLAG_HINT_PERMALOCK = 1 << 6,			/* Likely to be locked for long periods of time. */
	};

bool InitV3D (void);
uint32_t V3D_mem_alloc (uint32_t size, uint32_t align, uint32_t flags);
bool V3D_mem_free (uint32_t handle);
uint32_t V3D_mem_lock (uint32_t handle);
bool V3D_mem_unlock (uint32_t handle);
bool V3D_execute_code (uint32_t code, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5);
bool V3D_execute_qpu (int32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);

#define GL_FRAGMENT_SHADER	35632
#define GL_VERTEX_SHADER	35633

void DoRotate(float delta);
int LoadShader(int shaderType, int(*prn_handler) (const char *fmt, ...));
// Render a single triangle to memory.
void testTriangle (uint16_t renderWth, uint16_t renderHt, uint32_t renderBufferAddr, uint32_t bus_addr);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif