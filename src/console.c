#include <stdint.h>
#include "types.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "boardconfig.h"
#include "console.h"
#include "board_consoleuart.h"
#include "command.h"
#include "uartstdio.h"
#include "board.h"

extern uint8_t volatile command_verbosity_level;

void console_read_uart_available_bytes(char *stdin_buffer, uint32_t *bytes_in) {	
	int f = uartstdio_peek('\r');
	if (f > -1) {
			*bytes_in = uartstdio_rxbytesavail();
			uartstdio_gets(stdin_buffer, *bytes_in);
			uartstdio_flushrx();
	} else
		*bytes_in = 0;		
}

static void console_uart_init(uint32_t baudrate) {
    // ROM_SysCtlPeripheralEnable(CONSOLEUARTPINPERIPHERIAL); // Enable the GPIO Peripheral used by the UART.
    // ROM_SysCtlPeripheralEnable(CONSOLEUARTPERIPHERIAL); // Enable UART0

    // ROM_GPIOPinConfigure(CONSOLEUARTRXPINTYPE); // Configure GPIO Pins for UART mode.
    // ROM_GPIOPinConfigure(CONSOLEUARTTXPINTYPE);
    // ROM_GPIOPinTypeUART(CONSOLEUARTPINPERIPHERIALBASE, CONSOLEUARTRXPIN | CONSOLEUARTTXPIN);

    // ROM_UARTConfigSetExpClk(CONSOLEUARTPINPERIPHERIALBASE, ROM_SysCtlClockGet(), baudrate,
    //                         (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
    //                          UART_CONFIG_PAR_NONE));
    // uartstdio_config(0, baudrate, ROM_SysCtlClockGet()); // Initialize the UART for console I/O.
}

void console_printtext(const char *format , ...) {
	va_list arglist;
 	va_start( arglist, format );
 	uartstdio_vprintf( format, arglist );
 	va_end( arglist );
}

void console_testprint(char* text, defuint_t len){
	uartstdio_write(text, len);
}

void console_init(void) {
	board_consoleuart_init();
	command_verbosity_level = VERBOSITY_ALL; // Set default verbosity: we want to see all replies and error messages 
}



