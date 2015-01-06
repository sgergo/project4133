#ifndef CONSOLE_COMMAND_H
#define CONSOLE_COMMAND_H

#define VERBOSITY_NONE 0
#define VERBOSITY_ERROR 1
#define VERBOSITY_ALL 2

void command_execute(char *commandline_received);

#endif