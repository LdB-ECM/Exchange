#include "include/stdbool.h"
#include "include/multicore.h"
#include "include/emb-stdio.h"
#include "include/delays.h"
#include "include/start.h"
#include "include/mmu.h"
#include "include/lfb.h"

#include "include/sound.h"

static volatile bool core_ready[3] = { 0 };

void multicore_init()
{
	asm volatile ("mov x1, #0xe0\n"\
				  "mov x2, #0x80000\n"\
				  "str x2, [x1]\n"\
				  "dc civac, x1\n"\
				  "ic ivau, x1\n"\
				  "sev");
	
	while (core_ready[0] == false) {}
	printf("[CORE %d] [INFO] CORE 1 acknowledged start\n", get_core_id());
	asm volatile ("mov x1, #0xe8\n"\
				  "mov x2, #0x80000\n"\
		          "str x2, [x1]\n"\
		          "dc civac, x1\n"\
				  "ic ivau, x1\n"\
				  "sev");
	while (core_ready[1] == false) {}
	printf("[CORE %d] [INFO] CORE 2 acknowledged start\n", get_core_id());
	asm volatile ("mov x1, #0xf0\n"\
				  "mov x2, #0x80000\n"\
				  "str x2, [x1]\n"\
				  "dc civac, x1\n"\
				  "ic ivau, x1\n"\
				  "sev");
	while (core_ready[2] == false) {}
	printf("[CORE %d] [INFO] CORE 3 acknowledged start\n", get_core_id());
}

int get_core_id(void)
{
	//This function retrieves the core id
	//This is very useful for debugging
	
	uint64_t core_id = 0;

	asm volatile("mrs x1, mpidr_el1\n"\
		"and %0, x1, #0x3" : "=r"(core_id) : : "x1");

	return core_id;
}


int core_execute(char core_id, uint32_t functype, void* func, void* param1, void* param2)
{
	volatile struct icm_message req;
	req.func.val = func;
	req.functype = functype;
	req.response = 0;
	req.param1 = param1;
	req.param2 = param2;
	volatile uint32_t* mailbox = (uint32_t*)0x4000008C;
	mailbox[core_id * 4] = (uint32_t)(uintptr_t)&req;
	asm("sev");
	while (req.response == 0) {}; // wait for core to respond .. really should have a timeout error
	return 0;
}
			
void core_wait_for_instruction()
{
	//Here I have to do some init stuff as well to get the cores ready to opperate normally
	//This mostly involves the mmu
	//The mmu is required because semaphores are used in this very function!
	int CoreId = get_core_id();
	volatile uint32_t* mailbox = (uint32_t*)0x400000CC;
	if (CoreId == 0) return;

	mmu_init(); //The page table has already been created by core 0
	printf("[CORE %d] [INFO] Hello from Core %d\n[CORE %d] [INFO] MMU Online\n", CoreId, CoreId, CoreId);
	core_ready[CoreId-1] = true; // signal can start next core to start in
	while (1) {
		volatile struct icm_message* req = 0;
		int mbox = get_core_id() * 4;
		asm("wfe");
		req = (struct icm_message*)(uintptr_t)mailbox[mbox];
		mailbox[mbox] = 0;
		if (req) {
			/* Variables are on core_execute stack we must copy to here before we ack   */
			/* The moment we acknowledge the core_execute will release the stack memory */
			union caller_func func = req->func;
			int ftype = req->functype;
			uint16_t* p1 = req->param1;
			uint16_t* p2 = req->param2;
			req->response = 1;  // copied now safe to set acknowledge flag
			printf("[CORE %d] type: %i, Sound start %#x, Sound end %#x\n", get_core_id(), ftype, p1, p2);
			switch (ftype)
			{
				case 0:   /* void (*) (void) function */
					func.func0();
					break;
				case 1:   /* void (*) (void*) function */
					func.func1(p1);
					break;
				case 2:   /* void (*) (void*, void*) function */
					func.func2(p1, p2);
					break;
			}
		}
	}
}
