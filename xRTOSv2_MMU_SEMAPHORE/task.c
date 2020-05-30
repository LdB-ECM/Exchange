#include <stdint.h>
#include "xRTOS.h"
#include "rpi-smartstart.h"
#include "mmu.h"
#include "task.h"

 /*--------------------------------------------------------------------------}
 {                          TASK STATE CONSTANTS                             }
 {--------------------------------------------------------------------------*/
#define TASK_ZOMBIE				(  0  )
#define TASK_RUNNING			( 'R' )
#define TASK_SLEEPING          	( 'B' )
#define TASK_SUSPENDED			( 'S' )   
#define TASK_SEMAPHOREWAIT		( 'W' ) 

 /*--------------------------------------------------------------------------}
 {                            STACK CONSTANTS                                }
 {--------------------------------------------------------------------------*/
#if (portSTACK_DIRECTION == 1)
#define IDLESTACKTOP ( 0 )
#else
#define IDLESTACKTOP ( portSTACK_IDLE_SIZE )
#endif

 /*--------------------------------------------------------------------------}
 {                          IDLE TASK CONSTANTS                              }
 {--------------------------------------------------------------------------*/
#define IDLE_TASK_NUM ( 0 )                     // Idle task nunber is always zero
#define NR_IDLENAME   ( 4 )                     // Length of idle name
static const char* idletaskname = "IDLE";       // Idle task name 

/*--------------------------------------------------------------------------}
{                       TASK STRUCTURE DEFINITION                           }
{--------------------------------------------------------------------------*/
struct task_struct {
	CtxSwitch_t CtxStack;                       /*< This is the storage for the task context when it switches out.
													THIS MUST BE THE FIRST MEMBER OF TASK STRUCT FOR ASSEMBLER CODE */
	RegType_t* TaskStack;                       /*< Stack for this task */
	struct task_struct* nextTask;               /*< Next task in task chain or null if last task */
	struct task_struct* nextBlocked;            /*< Next task in blocked (sleep, semaphore) chain list or null if last task */
	TickType_t waketime;                        /*< If task is sleeping this is os_tickcount to wake it */
	char taskName[NR_TASKNAME];                 /*< Task name mainly used for debugging */
	struct {
		uint16_t counter : 8;                   /*< Time slice count assigend to task */
		uint16_t priority : 8;                  /*< Task base priority */
	};
	struct {
		uint16_t state : 8;                     /*< Current task state */
		uint16_t core : 8;                      /*< Core the task is running on */
	};
};

/*--------------------------------------------------------------------------}
{             CORE STRUCTURE DEFINITION ... EACH CPU CORE HAS ONE           }
{--------------------------------------------------------------------------*/
struct core_struct {
	volatile TaskHandle_t currentTask;					/*< Points to the location of current task running on this core.
															THIS MUST BE THE FIRST MEMBER OF CORE STRUCT
															It changes each task switch and the optimizer needs to know that */
	struct task_struct task[NR_TASKS];					/*< Tasks currently running on this core */
	RegType_t IdleStack[portSTACK_IDLE_SIZE];			/*< Idle Stack for this core */
	volatile TaskHandle_t sleeping;						/*< First of any tasks sleeping ... these are time ordered */
	volatile TickType_t osTickTime;						/*< Core tick time */
	struct {
		volatile unsigned uxCurrentNumberOfTasks : 16;	/*< Current number of task running on this core */
		volatile unsigned uxPercentLoadCPU : 16;		/*< Last CPU load calculated = uxIdleTickCount/configTICK_RATE_HZ * 100 in percent for last 1 sec frame   */
		volatile unsigned uxIdleTickCount : 16;			/*< Current ticks in 1 sec analysis frame that idle was current task */
		volatile unsigned uxCPULoadCount : 16;			/*< Current count in 1 sec analysis frame .. one sec =  configTICK_RATE_HZ ticks */
	};
};


/***************************************************************************}
{					   PRIVATE INTERNAL DATA STORAGE					    }
****************************************************************************/

static RegType_t TestStack[portSTACK_MAX_SIZE] __attribute__((aligned(16)));
static RegType_t* TestStackTop = &TestStack[0];

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{	 EXTERNAL FUNCTIONS EACH TARGET PORT MUST PROVIDE THESE 3 FUNCTIONS 	}
{***************************************************************************/

/* Assembler code usually defined in ctxsw.s */
extern void cpu_switch_to(TaskHandle_t next, TaskHandle_t prev);
/* Assembler code usually defined in ctxsw.s */
extern void cpu_context_restore(TaskHandle_t ctx);
/* Specific timer code usually defined in cpu.c for port */
extern void timer_init(uint16_t TickRateHz);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			                PRIVATE INTERNAL DATA                 			}
{***************************************************************************/
static struct core_struct core_data[NR_CORES] = { {0}, };           // Core data for each core

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			                 PUBLIC EXPOSED DATA                 			}
{***************************************************************************/
struct core_struct* core_ptr[NR_CORES] = { 0 };                     // Pointer to each core data

/***************************************************************************}
{					    PRIVATE INTERNAL ROUTINES						    }
****************************************************************************/

/*--------------------------------------------------------------------------}
{					Round Robin Priority Scheduler							}
{--------------------------------------------------------------------------*/
static void rr_schedule(uint8_t coreNum)
{
	struct task_struct* next = 0;                                  // Preset next task to null
	while (1)
	{
		unsigned c = 0;                                             // Preset c to zero
		struct task_struct* p;

		/* walk active tasks find next highest priority */
		p = &core_data[coreNum].task[IDLE_TASK_NUM];                // Start on idle task 
		while (p)                                                   // While task pointer valid
		{
			if ((p->state == TASK_RUNNING) && (p->counter > c))    // If task running and higher priority
			{
				c = p->counter;                                     // c is max count found
				next = p;                                           // Set next pointer to p
			}
			p = p->nextTask;                                        // Move to next task
		}
		/* if we found a count value higher than zero then break its next task */
		if (c && next)
		{
			break;
		}

		/* Walk active task chain resetting counts to priority */
		p = &core_data[coreNum].task[IDLE_TASK_NUM];                // Start on idle task  
		while (p)                                                   // While task pointer valid
		{
			if (p->state == TASK_RUNNING)                           // Only reset running task counts
			{
				p->counter = p->priority + 1;                       // Reset task counters
			}
			p = p->nextTask;                                        // Move to next task
		}
	}
	/* Okay now set current task to the next to run */
	core_data[coreNum].currentTask = next;                          // First simply set current task to next
}

/*-[ INTERNAL: Block Task ]-------------------------------------------------}
.  Places task in the requested block state. Requested task can be set to
.  NULL which is inferred to mean current task on calling core. Note task
.  to block can be on any core it does not have to be on calling core.
.--------------------------------------------------------------------------*/
static void xTaskBlock(TaskHandle_t xTaskToBlock, uint8_t BlockState)
{
	TaskHandle_t p;
	p = (xTaskToBlock == NULL) ? core_data[getCoreID()].currentTask :
		xTaskToBlock;                                               // Set pointer to current task or block task  
	uint8_t corenum = p->core;                                      // Hold core number of task
	disable_fiq();													// Stop preempt we are about to play with O/S pointers
	p->state = BlockState;                                          // Set task state to block state
	if (p == core_data[corenum].currentTask)                        // Suspending current task will require reschedule
		rr_schedule(corenum);                                       // So reschedule tasks it will change because current in SUSPENDED state
	if (p != core_data[corenum].currentTask && p != xTaskToBlock)   // Current task has moved
		cpu_switch_to(core_data[corenum].currentTask, p);           // Should be a new current task switch to it 
	enable_fiq();													// Enable preempt again
}

/*--------------------------------------------------------------------------}
{				The default idle task .. that does nothing :-)				}
{--------------------------------------------------------------------------*/
static void prvIdleTask(void* pvParameters)
{
	/* Stop warnings. */
	(void)pvParameters;

	/** THIS IS THE RTOS IDLE TASK - WHICH IS CREATED AUTOMATICALLY WHEN THE
	SCHEDULER IS STARTED. **/
	for (;; )
	{

	}
}

/*-[ INTERNAL: Initialize_Core ]--------------------------------------------}
.  Called to initialize each core. This must be done before anything else on
.  the core and if not done by createTask will be called by scheduler start.
.--------------------------------------------------------------------------*/
void Initialize_Core(uint8_t corenum)
{
	struct core_struct* core = &core_data[corenum];                 // Pointer to core data    

	/* Setup the idle task on this core */
	core->task[IDLE_TASK_NUM].state = TASK_RUNNING;                 // Set idle task to running state
	InitializeTaskStack(&core->task[IDLE_TASK_NUM].CtxStack,
		&core->IdleStack[IDLESTACKTOP], prvIdleTask, NULL);         // Initialize idle stack
	core->task[IDLE_TASK_NUM].core = corenum;                       // Idle task core number set
	core->task[IDLE_TASK_NUM].priority = 0;                         // Priority is 0 lowest
	core->task[IDLE_TASK_NUM].counter = 2;                          // Set counter to priority + 1 + 1 extra for first restore
	core->task[IDLE_TASK_NUM].nextTask = NULL;                      // Make sure next pointer is null    
	unsigned  j;
	for (j = 0; j < NR_IDLENAME && (idletaskname[j] != 0); j++)
		core->task[IDLE_TASK_NUM].taskName[j] = idletaskname[j];    // Transfer the taskname
	core->task[IDLE_TASK_NUM].taskName[j] = '\0';                   // Make sure name is asciiz
	core->currentTask = &core->task[IDLE_TASK_NUM];                 // Set current task to idle                 
	core->uxCurrentNumberOfTasks = 1;                               // Set 1 as number of tasks 
}

/*--------------------------------------------------------------------------}
{			Starts the tasks running on the core just as it says			}
{--------------------------------------------------------------------------*/
static void StartTasksOnCore(void)
{
	MMU_enable();													// Enable MMU
	timer_init(TICK_RATE_HZ);									    // Start the tick timer on core
	cpu_context_restore(core_data[getCoreID()].currentTask);		// Now restore current task on that core
}

/***************************************************************************}
{					    PUBLIC INTERFACE ROUTINES						    }
****************************************************************************/

/*-[ xTaskCreate ]----------------------------------------------------------}
.  Creates an xRTOS task and returns handle in pxCreatedTask if not null.
.  The task handle can be retrieved at a later time by using xTaskSelf. 
.  RETURN: 0 if task created success, any other value is the error code
.--------------------------------------------------------------------------*/
int xTaskCreate ( uint8_t corenum,
	              void (*pxTaskCode) (void* pxParam),				// The code for the task
				  const char* const pcName,							// The character string name for the task
				  const unsigned int usStackDepth,					// The stack depth in register size for the task stack 
				  void* const pvParameters,							// Private parameter that may be used by the task
				  uint8_t uxPriority,								// Priority of the task
				  TaskHandle_t* const pxCreatedTask)				// A pointer to return the task handle (NULL if not required) 
{   
    struct core_struct* core = &core_data[corenum];                 // Pointer to core data
    if (core->uxCurrentNumberOfTasks == 0)                          // Not even idle task setup so system isn't running yet
    {          
        Initialize_Core(corenum);                                   // Initialize the core which will setup idle task
    }
	disable_fiq();													// Stop preempt we are about to play with O/S pointers
    unsigned i;
    for (i = 0; i < NR_TASKS; i++)                                  // Search task list for first task entry not in use
    {
        if (core->task[i].state == TASK_ZOMBIE) break;              // Task not in use found so break
    }
	enable_fiq();													// Enable preempt again
    if (i == NR_TASKS) return ENOMEM;                               // No free tasks available return error
	disable_fiq();													// Stop preempt we are about to play with O/S pointers
    struct task_struct* p = &core->task[IDLE_TASK_NUM];             // Start on idle task
    while (p->nextTask) p = p->nextTask;                            // Walk along chain until p->next is null
    p->nextTask = &core->task[i];                                   // Set the that tasks next pointer
    core->task[i].state = TASK_RUNNING;                             // Set task to running state
    if (pxCreatedTask) (*pxCreatedTask) = &core->task[i];           // Return handle if requested
    core->task[i].TaskStack = TestStackTop;							// Hold new TOS
	// should check pointer so out of task stack space and return ENOMEM 
    #if (portSTACK_DIRECTION == 1)
    RegType_t* TOS = &core->task[i].TaskStack[0];                   // Top of stack 
    #else
    RegType_t* TOS = &core->task[i].TaskStack[usStackDepth];         // Top of stack 
    #endif   
    InitializeTaskStack(&core->task[i].CtxStack,  TOS, pxTaskCode, 
        pvParameters);                                              // Initialize task stack
	TestStackTop += usStackDepth;									// Move along stack
    core->task[i].priority = uxPriority;                            // Set task priority
    core->task[i].counter = uxPriority + 1;                         // Set counter to priority + 1
    core->task[i].nextTask = 0;                                     // Make sure next pointer is null
    core->task[i].core = corenum;                                   // Set core number to task
    if (pcName) {
        unsigned j;
        for (j = 0; (j < NR_TASKNAME - 1) && (pcName[j] != 0); j++)
            core->task[i].taskName[j] = pcName[j];				  	// Transfer the taskname
        core->task[i].taskName[j] = '\0';           	            // Make sure asciiz
    }
    core->uxCurrentNumberOfTasks++;                                 // Increment task count
	enable_fiq();													// Enable preempt again
    return 0;                                                       // Return success
}


/*-[ xTaskSelf ]------------------------------------------------------------}
.  Returns the calling task handle
.--------------------------------------------------------------------------*/
TaskHandle_t xTaskSelf(void)
{
	return core_data[getCoreID()].currentTask;                      // Return pointer to self
}

/*-[ xTaskDelay ]-----------------------------------------------------------}
.  Takes a task from the running list into the sleeping list to wake-up
.  at the given time period. The task uses no CPU time in this sleep time
.--------------------------------------------------------------------------*/
int xTaskDelay(const TickType_t ticks)
{
	uint8_t corenum = getCoreID();                                  // Fetch core ID this is calling from
	struct core_struct* core = &core_data[corenum];                 // Pointer to core data                                      
	disable_fiq();													// Stop preempt we are about to play with O/S pointers
	struct task_struct* ct = core->currentTask;                     // Current task on core
	ct->state = TASK_SLEEPING;										// Set task state to sleeping  
	ct->waketime = core->osTickTime;								// Load current tick time
	ct->waketime += ticks;											// Add tick count
	ct->counter = 0;												// Zero the counter
	struct task_struct* p = core->sleeping;                         // First sleeping task on core
	if (p)
	{
		while ( (p->nextBlocked != 0) && (p->nextBlocked->waketime < ct->waketime) )
			p = p->nextBlocked;                                     // Walk along chain to place in wake time order
		ct->nextBlocked = p->nextBlocked;							// Current task nextBlocked points to p nextBlocked
		p->nextBlocked = ct;					                    // p nextBlocked is current task so we join the sleep chain
	}
	else {
		ct->nextBlocked = NULL;						                // Make sure task next blocked is null
		core->sleeping = ct;							            // Make us the first sleeping task
	}
	rr_schedule(corenum);                                           // Now reschedule tasks it should change                                      
	if (ct != core->currentTask)
		cpu_switch_to(core->currentTask, ct);						// Should be a new current task switch to it 
	enable_fiq();													// Enable preempt again
	return 0;                                                       // Return success
}

/*-[ xTaskSuspend ]---------------------------------------------------------}
.  Suspends the given task handle preventing it getting any CPU time. If
.  the task handle is NULL it suspends the current calling task. Calls to
.  xTaskSuspend are not accumulative, calling vTaskSuspend twice on the
.  same task still only requires one call to xTaskResume to release task.
.  The task being suspended does not have to be on the calling core.
.--------------------------------------------------------------------------*/
void xTaskSuspend(TaskHandle_t xTaskToSuspend)
{
	xTaskBlock(xTaskToSuspend, TASK_SUSPENDED);                     // Block task with suspend state
}

/*-[ xTaskResume ]----------------------------------------------------------}
.  Resumes a given suspended task handle to running state. The task handle
.  can not be NULL because a suspended task would never be current task.
.  One call to xTaskResume will release any number of xTaskSuspend calls to
.  the same task as they are not accumulative. The task being resumed does
.  not have to be on the core doing the call.
.  RETURN: 0 for no error, EINVAL for an invalid task handle
.--------------------------------------------------------------------------*/
int xTaskResume(TaskHandle_t xTaskToResume)
{
	if (xTaskToResume)
	{
		xTaskToResume->state = TASK_RUNNING;                        // Set task state to running
		xTaskToResume->counter = xTaskToResume->priority + 1;		// Reset process tick count
		return 0;                                                   // Return success
	}
	return EINVAL;                                                  // Return invalid task handle
}


/*-[ xTaskStartScheduler ]--------------------------------------------------}
.  Starts the xRTOS task scheduler effectively starting the whole system
.--------------------------------------------------------------------------*/
void xTaskStartScheduler(void)
{
	uint8_t corenum = getCoreID();                                  // Fetch core ID this is calling from   

	/* MMU table setup done by core 0 */
	MMU_setup_pagetable();

	unsigned i;
	for (i = 0; i < NR_CORES; i++)				                    // For each core
	{
		if (i != corenum)                                           // For each core that isn't caller core (we do that last)
		{
			core_ptr[i] = &core_data[i];                            // Set each core pointer
			if (core_ptr[i]->uxCurrentNumberOfTasks == 0)           // No tasks setup yet so core not initialized
			{
				Initialize_Core(i);                                 // Initialize this core
			}
			CoreExecute(i, StartTasksOnCore);						// Start tasks on that core
		}
	}

	core_ptr[corenum] = &core_data[corenum];                        // Set caller core pointer
	if (core_ptr[corenum]->uxCurrentNumberOfTasks == 0)             // No tasks setup yet so core not initialized
	{
		Initialize_Core(corenum);                                   // Initialize the core
	}

	StartTasksOnCore();
}

/*-[ xTaskGetNumberOfTasks ]------------------------------------------------}
.  Returns the number of xRTOS tasks assigned to the core this is called
.--------------------------------------------------------------------------*/
unsigned int xTaskGetNumberOfTasks (void )
{
	return core_data[getCoreID()].uxCurrentNumberOfTasks;			// Return number of tasks on current core
}

/*-[ xLoadPercentCPU ]------------------------------------------------------}
.  Returns the load on the core this is called from in percent (0 - 100)
.--------------------------------------------------------------------------*/
unsigned int xLoadPercentCPU (void)
{
	return (((TICK_RATE_HZ - core_data[getCoreID()].uxPercentLoadCPU) * 100) / TICK_RATE_HZ);
}


/*
 * Called from the real time kernel tick via the EL0 timer irq this increments 
 * the tick count and checks if any tasks that are blocked for a finite period 
 * required removing from a delayed list and placing on the ready list.
 */
void xTaskIncrementTick (void)
{
	uint8_t corenum = getCoreID();                                  // Fetch core ID this is calling from
	struct core_struct* core = &core_data[corenum];                 // Pointer to core data

	/* increment OS timer */
	core->osTickTime++;                                             // Increment OS timer ticks

	/* LdB - Addition to calc CPU Load */
	if (core->currentTask == &core->task[IDLE_TASK_NUM])			// Is the core current task the core idle task
	{
		core->uxIdleTickCount++;									// Inc idle tick count in the current 1 second analysis frame
	}
	if (core->uxCPULoadCount >= TICK_RATE_HZ)						// If configTICK_RATE_HZ ticks done, time to see how many were idle
	{
		core->uxCPULoadCount = 0;									// Zero the config count for next analysis process period to start again
		core->uxPercentLoadCPU = core->uxIdleTickCount;				// Transfer the idletickcount to uxPercentLoadCPU we will only do calc when asked
		core->uxIdleTickCount = 0;									// Zero the idle tick count
	}
	else core->uxCPULoadCount++;									// Increment the process tick count

	/* check any sleeping tasks .. tasks are waketime ordered so only need to check 1st */
	if (core->sleeping)                                             // There is at least one sleeping task and preempts are ok
	{
		if (core->osTickTime >= core->sleeping->waketime)           // Check if we have reached wake time
		{
			TaskHandle_t p = (TaskHandle_t)core->sleeping;          // Hold current sleeping pointer
			core->sleeping = p->nextBlocked;					    // Sleeping is set to nextBlocked
			p->state = TASK_RUNNING;                                // Set task state back to running
			p->counter = p->priority + 1;                           // Reset the task counter
			p->nextBlocked = 0;                                     // Zero nextBlocked pointer
		}
	}

	/* decrement current counter */
	if (core->currentTask->counter > 0) core->currentTask->counter--;
	//if (core->currentTask->counter > 0)
	//{
	//	return;
	//}

	/* now reschedule */
	rr_schedule(corenum);

}


#include "emb-stdio.h"
#include "windows.h"
void __attribute__((interrupt("UNDEF"))) undefined_exception_handler_stub(void)
{
	WriteText("Undefined Exception raised\n");
	while (1) {}
}

void __attribute__((interrupt("ABORT"))) data_abort_exception_handler_stub(void)
{
	WriteText("Data abort Exception raised\n");
	while (1) {}
}





