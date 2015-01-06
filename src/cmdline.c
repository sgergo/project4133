#include <stdint.h>
#include "types.h"
#include <stdbool.h>
#include <string.h>
#include "cmdline.h"

#ifndef CMDLINE_MAX_ARGS
#define CMDLINE_MAX_ARGS        8
#endif

static char *cmdline_arg_array[CMDLINE_MAX_ARGS + 1];

defint_t cmdline_process(char *cmdline) {
    char *character_ptr;
    uint8_t argcounter;
    bool findarg = true;
    commandentry_t *commandentry;

    // Initialize the argument counter, and point to the beginning of the
    // command line string.
    argcounter = 0;
    character_ptr = cmdline;

    // Advance through the command line until a zero character is found.
    while(*character_ptr) {

        // If there is a space, then replace it with a zero, and set the flag
        // to search for the next argument.
        if(*character_ptr == ' ') {
            *character_ptr = 0;
            findarg = true;
        } else { // Otherwise it is not a space, so it must be a character that is part of an argument.

            // If findarg is set, then that means we are looking for the start
            // of the next argument.
            if(findarg) {
                // As long as the maximum number of arguments has not been
                // reached, then save the pointer to the start of this new arg
                // in the argv array, and increment the count of args, argc.
                if(argcounter < CMDLINE_MAX_ARGS) {
                    cmdline_arg_array[argcounter] = character_ptr;
                    argcounter++;
                    findarg = false;
                } else {

                    // The maximum number of arguments has been reached so return the error.
                    return(CMDLINE_TOO_MANY_ARGS);
                }
            }
        }
        // Advance to the next character in the command line.
        character_ptr++;
    }
    
    // If one or more arguments was found, then process the command.
    if(argcounter) {
        // Start at the beginning of the command table, to look for a matching
        // command.
        commandentry = &commandtable[0];

        // Search through the command table until a null command string is
        // found, which marks the end of the table.
        while(commandentry->commandname_ptr) {
            // If this command entry command string matches argv[0], then call
            // the function for this command, passing the command line
            // arguments.
            if(!strcmp(cmdline_arg_array[0], commandentry->commandname_ptr)) {
                
                return(commandentry->commandfunction_ptr(argcounter, cmdline_arg_array));
            }
            
            // Not found, so advance to the next entry.
            commandentry++;
        }
    }

    // Fall through to here means that no matching command was found, so return
    // an error.
    return(CMDLINE_BAD_CMD);
}