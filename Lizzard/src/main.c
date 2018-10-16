#include <stdint.h>
#include <math.h>
#include "rpi-SmartStart.h"
#include "emb-stdio.h"
#include "mmu.h"
#include "sound.h"
#include "rpi-GLES.h"

static uint32_t check_hello = 0;
static volatile uint32_t hellocount = 0;

void Core_SayHello(void)
{
	semaphore_inc(&check_hello);
	printf("Core %u says hello\n", GetCoreID());
	hellocount++;
	semaphore_dec(&check_hello);
}


static uint32_t shader1[18] = {  // Vertex Color Shader
		0x958e0dbf, 0xd1724823,   /* mov r0, vary; mov r3.8d, 1.0 */
		0x818e7176, 0x40024821,   /* fadd r0, r0, r5; mov r1, vary */
		0x818e7376, 0x10024862,   /* fadd r1, r1, r5; mov r2, vary */
		0x819e7540, 0x114248a3,   /* fadd r2, r2, r5; mov r3.8a, r0 */
		0x809e7009, 0x115049e3,   /* nop; mov r3.8b, r1 */
		0x809e7012, 0x116049e3,   /* nop; mov r3.8c, r2 */
		0x159e76c0, 0x30020ba7,   /* mov tlbc, r3; nop; thrend */
		0x009e7000, 0x100009e7,   /* nop; nop; nop */
		0x009e7000, 0x500009e7,   /* nop; nop; sbdone */
};

static RENDER_STRUCT scene = { 0 };


struct VC4_Vertex {
	uint16_t  X;				// X in 12.4 fixed point
	uint16_t  Y;				// Y in 12.4 fixed point
	uint32_t  Z;				// Z in 4 byte float format
	uint32_t  W;				// 1/W in byte float format
	uint32_t  VRed;				// Varying 0 (Red)
	uint32_t  VGreen;			// Varying 1 (Green)
	uint32_t  VBlue;			// Varying 2 (Blue)
} __packed;

static float angle = 0.0;


void DoRotate(double delta) {
	double cosTheta, sinTheta;
	angle += delta;
	if (angle >= (3.1415926384 * 2)) angle -= (3.1415926384 * 2);
	cosTheta = cos(angle);
	sinTheta = sin(angle);

	uint16_t centreX = (scene.renderWth / 2) << 4;						// triangle centre x
	uint16_t centreY = (scene.renderHt / 4) << 4;						// triangle centre y
	uint16_t half_shape_size = scene.renderWth/8;						// Half size of triangle
	double x1 = 0;
	double y1 = -half_shape_size;
	double x2 = -half_shape_size;
	double y2 = half_shape_size;
	double x3 = half_shape_size;
	double y3 = half_shape_size;

	struct VC4_Vertex* vd = (struct VC4_Vertex*)(uintptr_t)GPUaddrToARMaddr(scene.vertexVC4);
	vd[0].X = centreX + (int16_t)((cosTheta * x1 - sinTheta * y1) * 16);
	vd[0].Y = centreY + (int16_t)((cosTheta * y1 + sinTheta * x1) * 16);
	vd[1].X = centreX + (int16_t)((cosTheta * x2 - sinTheta * y2) * 16);
	vd[1].Y = centreY + (int16_t)((cosTheta * y2 + sinTheta * x2) * 16);
	vd[2].X = centreX + (int16_t)((cosTheta * x3 - sinTheta * y3) * 16);
	vd[2].Y = centreY + (int16_t)((cosTheta * y3 + sinTheta * x3) * 16);
}


static volatile uint32_t mmu_count = 0;
void CoreLoadMMU(void)
{
	mmu_init();
	printf("Setup MMU on core %d done\n", GetCoreID());
	mmu_count++;
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

	// Step1: Initialize scene
	V3D_InitializeScene(&scene, GetConsole_Width(), GetConsole_Height());

	// Step2: Add vertexes to scene
	V3D_AddVertexesToScene(&scene);

	// Step3: Add shader to scene
	V3D_AddShadderToScene(&scene, &shader1[0], _countof(shader1));

	// Step4: Setup render control
	V3D_SetupRenderControl(&scene, GetConsole_FrameBuffer());

	// Step5: Setup binning
	V3D_SetupBinningConfig(&scene);

	// Step 6: Render the scene
	V3D_RenderScene(&scene);

	// Display smart start details
	displaySmartStart(printf);										

	// Initialize audio
	init_audio_jack();

	printf("Setting up MMU table\n");
	/* Create MMU translation tables with Core 0 */
	init_page_table();
	printf("Starting MMU on core0\n");
	/* setup mmu on core 0 */
	mmu_init();

	/* Setup mmu on additional cores						*/
	/* We can not use semaphores until MMU up on all cores	*/
	/* They would all go for mmu_count at same time so walk */
	/* them in one at a time                                */
	printf("Setting up MMU on cores\n");
	CoreExecute(1, CoreLoadMMU);
	while (mmu_count == 0);
	CoreExecute(2, CoreLoadMMU);
	while (mmu_count == 1);
	CoreExecute(3, CoreLoadMMU);
	while (mmu_count == 2);
	// Allow time for core3 print to complete .. again tricky because can't semaphore until all up
	// The key here is usually you wouldn't print you would just drag the cores online
	timer_wait(10000);
	
	printf("The cores have all started their MMU\n");

	
	printf("Testing semaphore queue ability\n");
	semaphore_inc(&check_hello); // lock hello semaphore
	CoreExecute(1, Core_SayHello);
	CoreExecute(2, Core_SayHello);
	CoreExecute(3, Core_SayHello);
	// Hopefully it is clear the cores are queued up on the semaphore
	// Only one can ever have the semaphore so the rest for a queue
	// There is no way to know what order the cores will come thru in
	// If you want to control the order you need a queued primitive.
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
		V3D_RenderScene(&scene);
		timer_wait(10);
	}

}
