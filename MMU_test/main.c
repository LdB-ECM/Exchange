#include <stdint.h>
#include "rpi-SmartStart.h"
#include "emb-stdio.h"
#include "mmu.h"

static uint32_t check_sem = 0;
static uint32_t check_hello = 0;
static volatile uint32_t hellocount = 0;

void set_ACT_LED(bool on, uintptr_t addr)
{
	uint32_t* armaddr = (uint32_t*)addr;
	armaddr[0] = 8 * 4;
	armaddr[7] = 0;
	armaddr[1] = 0;
	armaddr[2] = 0x00038041;
	armaddr[3] = 8;
	armaddr[4] = 8;
	armaddr[5] = 130;
	armaddr[6] = (uint32_t)on;
	asm volatile ("dc civac, %0" : : "r" (addr) : "memory");
	mailbox_write(0x8, addr);
	mailbox_read(0x8);
}

void Check_Semaphore (void) 
{
	printf("Core %u checked table semaphore and reports %i\n", GetCoreID(), table_loaded);
	semaphore_dec(&check_sem);
}

void Core_SayHello(void)
{
	semaphore_inc(&check_hello);
	printf("Core %u says hello\n", GetCoreID());
	hellocount++;
	semaphore_dec(&check_hello);
}

void Core1_RunLED(void)
{
	uint32_t  __attribute__((aligned(16))) msg[8];
	while (1) {
		timer_wait(1000000);
		set_ACT_LED(true, (uintptr_t)&msg[0]);
		printf("Core 1 Led: 1\r");
		timer_wait(1000000);
		set_ACT_LED(false, (uintptr_t)&msg[0]);
		printf("Core 1 Led: 0\r");
	}
}


static const char Spin[4] = { '|', '/', '-', '\\' };
void main(void)
{
	Init_EmbStdio(Embedded_Console_WriteChar);						// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed

	/* Create MMU translation tables with Core 0 */
	init_page_table();

    /* setup mmu on core 0 */
	table_loaded = 1;
    mmu_init();

	/* setup mmu on core 1 */
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("Setting up MMU on core 1\n");
	CoreExecute(1, mmu_init);

	/* setup mmu on core 2 */
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("Setting up MMU on core 2\n");
	CoreExecute(2, mmu_init);

	/* setup mmu on core 3 */
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("Setting up MMU on core 3\n");
	CoreExecute(3, mmu_init);


	// Dont print until table load done
	semaphore_inc(&table_loaded);  // Lock the semaphore
	printf("The cores have all started their MMU\n");
	semaphore_dec(&table_loaded);  // Lock the semaphore


	semaphore_inc(&table_loaded);
	printf("Semaphore table_loaded locked .. check from other cores\n");
	printf("Semaphore table_loaded at: %08p:%08p\n", (void*)((uintptr_t)&table_loaded >> 32), (void*)((uintptr_t)&table_loaded & 0xFFFFFFFFul));
	semaphore_inc(&check_sem);
	CoreExecute(1, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(2, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(3, Check_Semaphore);
	
	semaphore_inc(&check_sem);
	semaphore_dec(&table_loaded);
	printf("Core 0 unlocked table semaphore .. re-run test\n");
	semaphore_dec(&check_sem);

	semaphore_inc(&check_sem);
	CoreExecute(1, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(2, Check_Semaphore);
	semaphore_inc(&check_sem);
	CoreExecute(3, Check_Semaphore);

	semaphore_inc(&check_sem);  // need to wait for check to finish writing to screen
	semaphore_dec(&check_sem); // lets be pretty and release sem
	printf("Testing semaphore queue ability\n");
	semaphore_inc(&check_hello); // lock hello semaphore
	CoreExecute(1, Core_SayHello);
	CoreExecute(2, Core_SayHello);
	CoreExecute(3, Core_SayHello);
	printf("Cores queued we will waiting 10 seconds to release\n");

	int i = 0;
	uint64_t endtime = timer_getTickCount64() + 10000000ul;
	while (timer_getTickCount64() < endtime) {
		printf("waiting %c\r", Spin[i]);
		timer_wait(50000);
		i++;
		i %= 4;
	}
	printf("Releasing semaphore\n");
	semaphore_dec(&check_hello); // release hello semaphore
	while (hellocount != 3) {};

	printf("test all done ... now deadlooping stop\n");
	printf("however core 1 will continue to Flash activity LED\n\n");
	CoreExecute(1, Core1_RunLED);
	while (1) {};


}
