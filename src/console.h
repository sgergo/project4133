
#ifndef CONSOLE_H
#define CONSOLE_H

#define STDINBUFFERSIZE						128	// Static stdin buffer size in console_process function
#define STDINDEFAULTCOMMANDSIZE				3   // Default length of a console command
#define CONSOLE_DEFAULT_BAUDRATE			115200

void console_init(void);
void console_testprint(char* text, defuint_t len);
void console_printtext(const char *format , ...);
void console_read_uart_available_bytes(char *stdin_buffer, uint32_t *bytes_in);
//void console_process(void);

#endif

