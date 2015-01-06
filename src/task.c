#include <stdint.h>
#include "types.h"
#include <stdio.h>
#include <string.h>

#include "task.h"
#include "taskarg.h"
#include "console.h"
#include "command.h"

tasklist_t volatile tasklist;

static void task_idle(void *ptr_task_struct) {
	
	console_printtext("idle.\n"); 
}

static void task_example1(void *ptr_task_struct) {
	
	console_printtext("example1 task.\n"); 
}

static void task_example2(void *ptr_task_struct) {
	
	console_printtext("example2 task.\n"); 
}

static void task_example3(void *ptr_task_struct) {

	console_printtext("example3 task with value: 0x%02x\n", ((default_task_arg_t *)ptr_task_struct)->uintval); 
}

static void task_console_process(void *ptr_task_struct) {
	static char stdin_buffer[STDINBUFFERSIZE];
	static defuint_t bytes_in;

	console_read_uart_available_bytes(stdin_buffer, &bytes_in);

	if (bytes_in > STDINDEFAULTCOMMANDSIZE)
		command_execute(stdin_buffer);
}

// Task table entries - fill it!
taskentry_t tasktable[] = {
	/* Entry structure:
	* short description, task function ptr, task period, periodcounter, task repetition, priority, task arg struct ptr 
	*/
	{"example1", task_example1, 5, 0, 0, TASKPRIORITYLEVEL_HIGH, NULL}, 
	{"example2", task_example2, 5, 0, 0, TASKPRIORITYLEVEL_HIGH, NULL},
	{"example3", task_example3, 5, 0, 0, TASKPRIORITYLEVEL_HIGH, NULL}, 
	{"idle", task_idle, 5, 0, 0, TASKPRIORITYLEVEL_HIGH, NULL}, 
	{"console_process", task_console_process, 1, 0, -1, TASKPRIORITYLEVEL_LOW, NULL},
	{0, 0, 0, 0, 0, 0, 0}
};

defint_t task_find_task_ID_by_infostring( char* taskstr) {
	taskentry_t *ptr_taskentry = &tasktable[0];
	defuint_t i = 0;

	while(ptr_taskentry->taskinfo) {
		if (!strcmp (ptr_taskentry->taskinfo, taskstr))
			return i;
		i++;
		ptr_taskentry++; 
	}

	return (-1);
}

void task_systick(void) {
	taskentry_t *ptr_taskentry = &tasktable[0];
	defuint_t i = 0;
	
	while(ptr_taskentry->taskinfo) {

		if (ptr_taskentry->taskrepetition > 0 || ptr_taskentry->taskrepetition == (-1)) {
			
			if (ptr_taskentry->periodcounter < ptr_taskentry->taskperiod) {
				ptr_taskentry->periodcounter++;
			} else { 
				tasklist |=(1<<i);
				ptr_taskentry->periodcounter = 0;
			}
		}

		i++;
		ptr_taskentry++; 
    }
}

void task_process(void) {
	taskentry_t *ptr_taskentry = &tasktable[0];
	tasklist_t tasklist_copy;
	defuint_t i = 0;

	// Make a copy of the tasklist variable to avoid race condition
	tasklist_copy = tasklist;
	if (!tasklist_copy) return;

	while(ptr_taskentry->taskinfo) {
		if (tasklist_copy & (1<<i)) {
			ptr_taskentry->taskfunction(ptr_taskentry->taskarg);
			if (ptr_taskentry->taskrepetition > 0)
				ptr_taskentry->taskrepetition--;
			 
			// Clear task in both variables
			tasklist_copy &=~(1<<i);
			tasklist &=~(1<<i);
		} 

		i++;
		ptr_taskentry++;
	}
}
