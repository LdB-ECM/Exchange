#include "include/stdbool.h"
#include "include/multicore.h"
#include "include/emb-stdio.h"
#include "include/delays.h"
#include "include/start.h"
#include "include/mmu.h"
#include "include/lfb.h"

static uint32_t core_execute_lock[3] = { 0 };
static volatile bool core_ready[3] = { 0 };
static volatile bool core_execute_state[3] = { 0 };

void (*core1_jump_func_ptr) (void *);
void (*core2_jump_func_ptr) (void *);
void (*core3_jump_func_ptr) (void *);



void *core1_data;
void *core2_data;
void *core3_data;

void multicore_init()
{
	asm volatile ("mov x1, #0xe0\n"\
				  "mov x2, #0x80000\n"\
				  "str x2, [x1]");
	asm volatile ("dc civac, %0" : : "r" (0xe0) : "memory");
	asm volatile ("sev");
	
	while (core_ready[0] == false) {}
	
	printf("Core 1 acknowledged start\n");
	
	asm volatile ("mov x1, #0xe8\n"\
				  "mov x2, #0x80000\n"\
				  "str x2, [x1]");
	asm volatile ("dc civac, %0" : : "r" (0xe8) : "memory");
	asm volatile ("sev");
	while (core_ready[1] == false) {}

	printf("Core 2 acknowledged start\n");

	asm volatile ("mov x1, #0xf0\n"\
			      "mov x2, #0x80000\n"\
				  "str x2, [x1]");
	asm volatile ("dc civac, %0" : : "r" (0xf0) : "memory");
	asm volatile ("sev");
	
	while (core_ready[2] == false) {}

	printf("Core 3 acknowledged start\n");
}

int get_core_id()
{
	//This function retrieves the core id
	//This is very useful for debugging
	
	uint64_t core_id;

	asm volatile("mrs x1, mpidr_el1\n"\
				 "and %0, x1, #0x3" : "=r"(core_id) : :"x1");

	return core_id;
}

void get_core_ready()
{
	switch(get_core_id())
	{
		//Cache stuff required here
		case 1:
			core_ready[0] = true;
			break;
		case 2:
			core_ready[1] = true;
			break;
		case 3:
			core_ready[2] = true;
			break;
		default:
			break; //I don't know how this would execute
	}
}

//==============================================================================================//
//	Documentation of core_execute(char core_id, void (*func_ptr) (void *), void *data_ptr)	//
//												//
//	core_execute has two paramaters. The first is the number of the core			//
//	to execute on, the second is a pointer to the function to execute.			//
//	The second parameter is the name of the function to call for example			//
//	it would be called in the following manner:						//
//	core_execute(1, printf_core_wrapper, (void *)data)					//
//												//
//	The data is then passed as an argument to the function to be executed.			//
//==============================================================================================//

int core_execute(char core_id, void (*func_ptr) (void *), void *data_ptr) //This will only run on core 0
{
	if(get_core_id()) return 1; //Not running on core 0
	switch(get_core_id())
	{
		case 1:
			semaphore_inc(&core_execute_lock[0]);
			core1_jump_func_ptr = func_ptr;
			core1_data = data_ptr;
			core_execute_state[0] = true;
			semaphore_dec(&core_execute_lock[0]);
			break;
		case 2:
			semaphore_inc(&core_execute_lock[1]);
			core2_jump_func_ptr = func_ptr;
			core2_data = data_ptr;
			core_execute_state[1] = true;
			semaphore_dec(&core_execute_lock[1]);
			break;
		case 3:
			semaphore_inc(&core_execute_lock[2]);
			core3_jump_func_ptr = func_ptr;
			core3_data = data_ptr;
			core_execute_state[2] = true;
			semaphore_dec(&core_execute_lock[2]);
			break;
		default:
			return 2; //Invalid core id
	}
	return 0;
}
			
void core_wait_for_instruction()
{
	//Here I have to do some init stuff as well to get the cores ready to opperate normally
	//This mostly involves the mmu
	//The mmu is required because semaphores are used in this very function!
	mmu_init(); //The page table has already been created by core 0
	printf("[CORE %d] [INFO] Hello from Core %d\n[CORE %d] [INFO] MMU Online\n", get_core_id(), get_core_id(), get_core_id());
	
	//This is a place the cores come when they are done and relaxing
	volatile bool core1_execute_local = false;
	volatile bool core2_execute_local = false;
	volatile bool core3_execute_local = false;
	
	get_core_ready(); //Toggle the ready flag and let another core be released
	
	while(1) //The cores never leave they only branch out and then come back
	{
		switch(get_core_id())
		{
			case 1:
				semaphore_inc(&core_execute_lock[0]);
				if(core_execute_state[0] == true)
				{
					core_execute_state[0] = false;
					core1_execute_local = true;
				}
				semaphore_dec(&core_execute_lock[0]);
				if(core1_execute_local == true)
				{
					core1_jump_func_ptr(core1_data);
					core1_execute_local = false;
				}
				break;
			case 2:
				semaphore_inc(&core_execute_lock[1]);
				if(core_execute_state[1] == true)
				{
					core_execute_state[1] = false;
					core2_execute_local = true;
				}
				semaphore_dec(&core_execute_lock[1]);
				if(core2_execute_local == true)
				{
					core2_jump_func_ptr(core2_data);
					core2_execute_local = false;
				}
				break;
			case 3:
				semaphore_inc(&core_execute_lock[2]);
				if(core_execute_state[2] == true)
				{
					core_execute_state[2] = false;
					core3_execute_local = true;
				}
				semaphore_dec(&core_execute_lock[3]);
				if(core3_execute_local == true)
				{
					core3_jump_func_ptr(core3_data);
					core3_execute_local = false;
				}
				break;
			default:
				//Not valid if this ever occured it isn't being run on the target board
				break;
		}
	}
}
