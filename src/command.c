#include <stdint.h>
#include "types.h"
#include "msp430fr4133.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


// #include "uartstdio.h"

#include "console.h"
#include "command.h"
#include "cmdline.h"
#include "task.h"
#include "taskarg.h"

extern taskentry_t tasktable[];
level_t volatile command_verbosity_level;

// Command 'help'
static int command_help(int argc, char *argv[]) {
	commandentry_t *entry;

    console_printtext("\nAvailable commands\n");
    console_printtext("-----------------------------------------------------------------------------\n");
    console_printtext("%10s %20s %10s %10s\n", "Command", "Args", "Format", "Description");
    console_printtext("-----------------------------------------------------------------------------\n");

    entry = &commandtable[0]; // Point at the beginning of the command table.

    while(entry->commandname_ptr) {

        // Print the command name and the brief description.
        console_printtext("%10s %20s %10s %10s\n", 
            entry->commandname_ptr, 
            entry->commandarg_ptr, 
            entry->commandargformat_ptr, 
            entry->commandhelptext_ptr );

        // Advance to the next entry in the table. 
        entry++;
    }
    
    console_printtext("\n");
    return(0);
}

// Command 'example1'
static int command_example1(int argc, char *argv[]) {
    defint_t taskid;

    if (argc < 2) {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: missing argument.\n");
        return (0);
    }

    taskid = task_find_task_ID_by_infostring("example1");
    if (taskid == -1) {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: task entry error.\n");
        return (0);
    }
    
    if (!strcmp (argv[1], "on")) {
        tasktable[taskid].taskrepetition = -1;
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("example1 task is continuously on.\n");
    } else if (!strcmp (argv[1], "off")) {
        tasktable[taskid].taskrepetition = 0;
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("example1 task is off.\n");
    } else {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: invalid input.\n");
    }

    return(0);
}

// Command 'example2'
static int command_example2(int argc, char *argv[]) {
    defint_t taskid;

    if (argc < 2) {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: missing argument.\n");
        return (0);
    }

    taskid = task_find_task_ID_by_infostring("example2");
    if (taskid == -1) {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: task entry error.\n");
        return (0);
    }

    if (!strcmp (argv[1], "on")) {
        tasktable[taskid].taskrepetition = 10;
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("example2 task will be repeated 10 times.\n");
    } else if (!strcmp (argv[1], "off")) {
        tasktable[taskid].taskrepetition = 0;
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("example2 task is off.\n");
    } else {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: invalid input.\n");
    }

    return(0);
}

// Command 'example3'
static int command_example3(int argc, char *argv[]) {
    defint_t taskid;
    long num;
    char *endptr;
    static default_task_arg_t task_arg;
   
    if (argc < 2) {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: missing value.\n");
        return (0);
    }

    taskid = task_find_task_ID_by_infostring("example3");
    if (taskid == -1) {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: task entry error.\n");
        return (0);
    }

    errno = 0;
    num = strtol(argv[1], &endptr, 16);

    if (*endptr != 0 || errno != 0) {
        if (command_verbosity_level > VERBOSITY_NONE)
            console_printtext("error: invalid input.\n");

        return (0);
    }

    // TODO: num contains a valid value, now do something with it
    console_printtext("example3 command received with value: 0x%02x\n", num);
    task_arg.uintval = num;
    tasktable[taskid].taskarg = &task_arg;
    tasktable[taskid].taskrepetition = 1;
    return(0);
}

// Command 'rep'
static int command_report(int argc, char *argv[]) {
    
    return(0);
}

// Command 'rst'
static int command_reset(int argc, char *argv[]) {

    PMMCTL0 = PMMPW + PMMSWBOR + (PMMCTL0 & 0x0003);
    return (0);
}

// Command 'verb'
static int command_setverbosity(int argc, char *argv[]) {
    if (argc < 2) {
        console_printtext("verbosity level: %d\n", command_verbosity_level);
        return (0);
    }

    if (!strcmp (argv[1], "none")) {
        command_verbosity_level = VERBOSITY_NONE;
    } else if (!strcmp (argv[1],"error")) {
        command_verbosity_level = VERBOSITY_ERROR;
    } else if (!strcmp (argv[1],"all")) {
        command_verbosity_level = VERBOSITY_ALL;
    } else {
        console_printtext("error: invalid input.\n");
        return (0);
    }

    return (0);       
}

void command_execute(char *commandline_received) {
	defint_t ret;
	ret = cmdline_process(commandline_received);

    // If CmdLineProcess returns with a non-zero value something went wrong
	if (ret) 
        console_printtext("Unknown command, error code: %d\n", ret); 
}

// Command table entries - fill it!
commandentry_t commandtable[] = {

    { "ex1", "{'on'/'off'}" , "ascii", command_example1, "Starts/stops a continuous task." },
    { "ex2", "{'on'/'off'}" , "ascii", command_example2, "Starts/stops a task with 10 repetitions." },
    { "ex3", "{VALUE}" , "hex", command_example3, "Execute a task with a hex value input." },
   
    { "help", "-" , "-", command_help,   "Display list of commands." }, 
    { "rep", "-" , "-",  command_report,   "Reports state variables." },
    { "rst", "-" , "-",  command_reset,   "Reset." },
    { "verb", "{none/error/all}" , "ascii",  command_setverbosity, "Sets verbosity level." },
    { 0, 0, 0, 0, 0} // Don't touch it, last entry must be a terminating NULL entry
};
