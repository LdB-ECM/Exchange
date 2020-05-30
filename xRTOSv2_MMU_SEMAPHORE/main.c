#include <stdint.h>
#include <string.h>
#include "rpi-smartstart.h"
#include "emb-stdio.h"
#include "xRTOS.h"
#include "task.h"
#include "windows.h"

/* UNCOMMENT THIS FOR SOFTWARE SEMAPHORES */
//#define soft_semaphore 

#if defined(soft_semaphore) 
#include "semaphore.h"
    /* USE SOFTWARE MMU SEMAPHORES .. USEFUL FOR DEBUGGING */
	#define semaphore_take(a) xSemaphoreWait(a)
	#define semaphore_give(a) xSemaphoreSignal(a)
	static SemaphoreHandle_t sprintfSem = { 0 };
#else
	/* USE HARD MMU SEMAPHORES */
	static uint32_t sprintfSem = 1;
#endif



void DoProgress(HDC dc, int step, int total, int x, int y, int barWth, int barHt,  COLORREF col)
{

	// minus label len
	int pos = (step * barWth) / total;

	// Draw the colour bar
	COLORREF orgBrush = SetDCBrushColor(dc, col);
	Rectangle(dc, x, y, x+pos, y+barHt);

	// Draw the no bar section 
	SetDCBrushColor(dc, 0);
	Rectangle(dc, x+pos, y, x+barWth, y+barHt);

	SetDCBrushColor(dc, orgBrush);

}

static unsigned int Counts[4] = { 0 };

/* sprintf is non re-enterant so need a semaphore so all task calls share it */
static int use_sprintf(char* buf, const char* fmt, ...)
{
	semaphore_take(&sprintfSem);
	va_list myargs;
	va_start(myargs, fmt);
	int retVal = vsprintf(buf, fmt, myargs);
	va_end(myargs);
	semaphore_give(&sprintfSem);
	return retVal;
}

void task1(void *pParam) {
	char buf[32];
	HDC Dc = CreateExternalDC(1);
	COLORREF col = 0xFFFF0000;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}

		DoProgress(Dc, step, total, 10, 100, GetScreenWidth() - 20, 20, col);
		xTaskDelay(10);
		Counts[0]++;
		use_sprintf(&buf[0], "Core 0 count %u", Counts[0]);
		TextOut(Dc, 0, 160, &buf[0], strlen(&buf[0]));

	}
}


void task1A(void* pParam) {
	char buf[64];
	HDC Dc = CreateExternalDC(5);
	COLORREF col = 0xFF00FFFF;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}
		DoProgress(Dc, step, total, 10, 125, GetScreenWidth() - 20, 20, col);
		xTaskDelay(11);
		use_sprintf(&buf[0], "Core 0 Load: %3i%% Task count: %2i", xLoadPercentCPU(), xTaskGetNumberOfTasks());
		TextOut(Dc, 0, 80, &buf[0], strlen(&buf[0]));
	}
}



void task2(void *pParam) {
	char buf[32];
	HDC Dc = CreateExternalDC(2);
	COLORREF col = 0xFF0000FF;
	int total = 1000;
	volatile int step = 0;
	volatile int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}
		DoProgress(Dc, step, total, 10, 200, GetScreenWidth() - 20, 20, col);
		xTaskDelay(12);
		Counts[1]++;
		use_sprintf(&buf[0], "Core 1 count %u", Counts[1]);
		TextOut(Dc, 0, 256, &buf[0], strlen(&buf[0]));
	}
}

void task2A(void* pParam) {
	char buf[64];
	HDC Dc = CreateExternalDC(6);
	COLORREF col = 0xFFFFFFFF;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}
		DoProgress(Dc, step, total, 10, 225, GetScreenWidth() - 20, 20, col);
		xTaskDelay(13);
		use_sprintf(&buf[0], "Core 1 Load: %3i%% Task count: %2i", xLoadPercentCPU(), xTaskGetNumberOfTasks());
		TextOut(Dc, 0, 180, &buf[0], strlen(&buf[0]));
	}
}

void task3(void *pParam) {
	char buf[32];
	HDC Dc = CreateExternalDC(3);
	COLORREF col = 0xFF00FF00;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}
		DoProgress(Dc, step, total, 10, 300, GetScreenWidth() - 20, 20, col);
		xTaskDelay(14);
		Counts[2]++;
		use_sprintf(&buf[0], "Core 2 count %u", Counts[2]);
		TextOut(Dc, 0, 352, &buf[0], strlen(&buf[0]));
	}
}

void task3A(void* pParam) {
	char buf[64];
	HDC Dc = CreateExternalDC(7);
	COLORREF col = 0xFF7F7F7F;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}
		DoProgress(Dc, step, total, 10, 325, GetScreenWidth() - 20, 20, col);
		xTaskDelay(15);
		use_sprintf(&buf[0], "Core 2 Load: %3i%% Task count: %2i", xLoadPercentCPU(), xTaskGetNumberOfTasks());
		TextOut(Dc, 0, 280, &buf[0], strlen(&buf[0]));
	}
}

void task4 (void* pParam) {
	char buf[32];
	HDC Dc = CreateExternalDC(4);
	COLORREF col = 0xFFFFFF00;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}
		DoProgress(Dc, step, total, 10, 400, GetScreenWidth() - 20, 20, col);
		xTaskDelay(16);
		Counts[3]++;
		use_sprintf(&buf[0], "Core 3 count %u", Counts[3]);
		TextOut(Dc, 0, 448, &buf[0], strlen(&buf[0]));
	}
}


void task4A(void* pParam) {
	char buf[64];
	HDC Dc = CreateExternalDC(8);
	COLORREF col = 0xFFFF00FF;
	int total = 1000;
	int step = 0;
	int dir = 1;
	while (1) {
		step += dir;
		if ((step == total) || (step == 0))
		{
			dir = -dir;
		}
		DoProgress(Dc, step, total, 10, 425, GetScreenWidth() - 20, 20, col);
		xTaskDelay(17);
		use_sprintf(&buf[0], "Core 3 Load: %3i%% Task count: %2i", xLoadPercentCPU(), xTaskGetNumberOfTasks());
		TextOut(Dc, 0, 380, &buf[0], strlen(&buf[0]));
	}
}


int main (void)
{
	Init_EmbStdio(WriteText);										// Initialize embedded stdio
	PiConsole_Init(0, 0, 16, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed
	printf("Task tick rate: %u\n", TICK_RATE_HZ);
	
	#if defined(soft_semaphore) 
	xSemaphoreBinaryInit(&sprintfSem);
	#endif

	/* Core 0 tasks */
	xTaskCreate(0, task1, "Core0-1", 512, NULL, 1, NULL);
	xTaskCreate(0, task1A, "Core0-2", 512, NULL, 1, NULL);

	/* Core 1 tasks */
	xTaskCreate(1, task2, "Core1-1", 512, NULL, 1, NULL);
	xTaskCreate(1, task2A, "Core1-2", 512, NULL, 1, NULL);

	/* Core 2 tasks */
	xTaskCreate(2, task3, "Core2-1", 512, NULL, 1, NULL);
	xTaskCreate(2, task3A, "Core2-2", 512, NULL, 1, NULL);

	/* Core 3 tasks */
	xTaskCreate(3, task4, "Core3-1", 512, NULL, 1, NULL);
	xTaskCreate(3, task4A, "Core3-2", 512, NULL, 1, NULL);

	/* Start scheduler */
	xTaskStartScheduler();
	/*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
	while (1) {
	}

	return (0);
}
