#include <stdint.h>
#include <math.h>
#include "rpi-SmartStart.h"
#include "emb-stdio.h"
#include "mmu.h"
#include "sound.h"
#include "rpi-GLES.h"

static uint32_t check_sem = 0;
static uint32_t check_hello = 0;
static volatile uint32_t hellocount = 0;

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


static const char Spin[4] = { '|', '/', '-', '\\' };
void main(void)
{
	InitV3D();														// Start 3D graphics
	Init_EmbStdio(Embedded_Console_WriteChar);						// Initialize embedded stdio
	PiConsole_Init(0, 0, 32, printf);								// Auto resolution console, message to screen
	ARM_setmaxspeed(printf);										// ARM CPU to max speed

	// Lock GPU memory
	uint32_t handle = V3D_mem_alloc(0x800000, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
	if (!handle) {
		printf("Error: Unable to allocate memory");
		return;
	}
	uint32_t bus_addr = V3D_mem_lock(handle);

	// Draw a triangle
	testTriangle(GetConsole_Width(), GetConsole_Height(),
		ARMaddrToGPUaddr((void*)(uintptr_t)GetConsole_FrameBuffer()), bus_addr);

	// Display smart start details
	displaySmartStart(printf);										

	// Initialize audio
	init_audio_jack();

	// Initialize uart and print string
	pl011_uart_init(115200);
	pl011_uart_puts("Hello UART\n");

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
	printf("Semaphore table_loaded at: %#p\n", &table_loaded);
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
	printf("Core 2 will play sound to PWM\n");


	INCLUDE_BINARY_FILE(Interlude, "src/audio/Interlude.bin", ".rodata.interlude");
	audio_start = &Interlude_start;
	audio_end = &Interlude_end;
	CoreExecute(2, play_audio);

	while (1) {
		DoRotate(0.01f);
		testTriangle(GetConsole_Width(), GetConsole_Height(),
			ARMaddrToGPUaddr((void*)(uintptr_t)GetConsole_FrameBuffer()), bus_addr);
		timer_wait(10);
	
	};

	// Release resources
	//V3D_mem_unlock(handle);
	//V3D_mem_free(handle);


}
