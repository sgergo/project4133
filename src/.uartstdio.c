#include <stdint.h>
#include "types.h"
#include <stdbool.h>
#include <stdarg.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "uartstdio.h"

#include "board.h"
#include "console.h"

static bool g_disable_echo;

static unsigned char uarttxbuffer[UART_TX_BUFFER_SIZE];
static volatile defuint_t uarttxwriteindex = 0;
static volatile defuint_t uarttxreadindex = 0;

static unsigned char uartrxbuffer[UART_RX_BUFFER_SIZE];
static volatile defuint_t uartrxreadindex = 0;
static volatile defuint_t uartrxwriteindex = 0;

#define TX_BUFFER_USED          (uartstdio_getbuffercount(&uarttxreadindex,  \
                                                &uarttxwriteindex, \
                                                UART_TX_BUFFER_SIZE))

#define TX_BUFFER_FREE          (UART_TX_BUFFER_SIZE - TX_BUFFER_USED)

#define TX_BUFFER_EMPTY         (uartstdio_isbufferempty(&uarttxreadindex,   \
                                               &uarttxwriteindex))

#define TX_BUFFER_FULL          (uartstdio_isbufferfull(&uarttxreadindex,  \
                                              &uarttxwriteindex, \
                                              UART_TX_BUFFER_SIZE))
#define ADVANCE_TX_BUFFER_INDEX(Index) \
                                (Index) = ((Index) + 1) % UART_TX_BUFFER_SIZE

#define RX_BUFFER_USED          (uartstdio_getbuffercount(&uartrxreadindex,  \
                                                &uartrxwriteindex, \
                                                UART_RX_BUFFER_SIZE))

#define RX_BUFFER_FREE          (UART_RX_BUFFER_SIZE - RX_BUFFER_USED)

#define RX_BUFFER_EMPTY         (uartstdio_isbufferempty(&uartrxreadindex,   \
                                               &uartrxwriteindex))

#define RX_BUFFER_FULL          (uartstdio_isbufferfull(&uartrxreadindex,  \
                                              &uartrxwriteindex, \
                                              UART_RX_BUFFER_SIZE))

#define ADVANCE_RX_BUFFER_INDEX(Index) \
                                (Index) = ((Index) + 1) % UART_RX_BUFFER_SIZE

static defuint_t g_uartbase = 0;
static const char * const g_hex = "0123456789abcdef";
static const defuint_t g_uartbaselist[3] = {
    UART0_BASE, UART1_BASE, UART2_BASE
};
static const defuint_t g_uartintlist[3] = {
    INT_UART0, INT_UART1, INT_UART2
};
static defuint_t g_portnum;
static const defuint_t g_uartperiphlist[3] = {
    SYSCTL_PERIPH_UART0, SYSCTL_PERIPH_UART1, SYSCTL_PERIPH_UART2
};

static bool uartstdio_isbufferfull(volatile defuint_t *read_ptr, volatile defuint_t *write_ptr, defuint_t size) {
    defuint_t write;
    defuint_t read;

    write = *write_ptr;
    read = *read_ptr;

    return((((write + 1) % size) == read) ? true : false);
}                                

static bool uartstdio_isbufferempty(volatile defuint_t *read_ptr, volatile defuint_t *write_ptr) {
    defuint_t write;
    defuint_t read;

    write = *write_ptr;
    read = *read_ptr;

    return((write == read) ? true : false);
}

static defuint_t uartstdio_getbuffercount(volatile defuint_t *read_ptr, volatile defuint_t *write_ptr, defuint_t size) {
    defuint_t write;
    defuint_t read;

    write = *write_ptr;
    read = *read_ptr;

    return((write >= read) ? (write - read) : (size - (read - write)));
}

static void uartstdio_primetransmit(defuint_t base) {

    if(!TX_BUFFER_EMPTY) {

        MAP_IntDisable(g_uartintlist[g_portnum]);

        while(MAP_UARTSpaceAvail(base) && !TX_BUFFER_EMPTY) {
            // MAP_UARTCharPutNonBlocking(base,
            //                           uarttxbuffer[uarttxreadindex]);
            MAP_UARTCharPut(base, uarttxbuffer[uarttxreadindex]);
            ADVANCE_TX_BUFFER_INDEX(uarttxreadindex);
        }

        MAP_IntEnable(g_uartintlist[g_portnum]);
    }
}

void uartstdio_config(defuint_t portnum, defuint_t baud, defuint_t clockfreq) {
    //
    // Check the arguments.
    //
    ASSERT((portnum == 0) || (portnum == 1) ||
           (portnum == 2));

    //
    // In buffered mode, we only allow a single instance to be opened.
    //
    ASSERT(g_uartbase == 0);

    //
    // Check to make sure the UART peripheral is present.
    //
    if(!MAP_SysCtlPeripheralPresent(g_uartperiphlist[portnum])) 
        return;

    //
    // Select the base address of the UART.
    //
    g_uartbase = g_uartbaselist[portnum];

    //
    // Enable the UART peripheral for use.
    //
    MAP_SysCtlPeripheralEnable(g_uartperiphlist[portnum]);

    //
    // Configure the UART for 115200, n, 8, 1
    //
    MAP_UARTConfigSetExpClk(g_uartbase, clockfreq, baud,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_WLEN_8));

    //
    // Set the UART to interrupt whenever the TX FIFO is almost empty or
    // when any character is received.
    //
    MAP_UARTFIFOLevelSet(g_uartbase, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    //
    // Flush both the buffers.
    //
    uartstdio_flushrx();
    uartstdio_flushtx(true);

    //
    // Remember which interrupt we are dealing with.
    //
    g_portnum = portnum;

    //
    // We are configured for buffered output so enable the master interrupt
    // for this UART and the receive interrupts.  We don't actually enable the
    // transmit interrupt in the UART itself until some data has been placed
    // in the transmit buffer.
    //
    MAP_UARTIntDisable(g_uartbase, 0xFFFFFFFF);
    MAP_UARTIntEnable(g_uartbase, UART_INT_RX | UART_INT_RT);
    MAP_IntEnable(g_uartintlist[portnum]);

    //
    // Enable the UART operation.
    //
    MAP_UARTEnable(g_uartbase);
}

defint_t uartstdio_write(const char *buf, defuint_t len) {
    defuint_t i;

    // Check for valid arguments.
    ASSERT(buf != 0);
    ASSERT(g_uartbase != 0);

    // Send the characters
    for(i = 0; i < len; i++) {
        // If the character to the UART is \n, then add a \r before it so that
        // \n is translated to \n\r in the output.
        if(buf[i] == '\n') {

            if(!TX_BUFFER_FULL) {
                uarttxbuffer[uarttxwriteindex] = '\r';
                ADVANCE_TX_BUFFER_INDEX(uarttxwriteindex);
            } else {

                // Buffer is full - discard remaining characters and return.
                break;
            }
        }

        // Send the character to the UART output.
        if(!TX_BUFFER_FULL) {
            uarttxbuffer[uarttxwriteindex] = buf[i];
            ADVANCE_TX_BUFFER_INDEX(uarttxwriteindex);
        } else {
            // Buffer is full - discard remaining characters and return.
            break;
        }
    }

    // If we have anything in the buffer, make sure that the UART is set
    // up to transmit it.
    if(!TX_BUFFER_EMPTY) {
        uartstdio_primetransmit(g_uartbase);
        MAP_UARTIntEnable(g_uartbase, UART_INT_TX);
    }

    // Return the number of characters written.
    return(i);
}

defint_t uartstdio_gets(char *buf, uint32_t len) {
    defuint_t i = 0;
    int8_t character;

    // Check the arguments.
    ASSERT(buf != 0);
    ASSERT(len != 0);
    ASSERT(g_uartbase != 0);

    // Adjust the length back by 1 to leave space for the trailing
    // null terminator.
    len--;

    // Process characters until a newline is received.
    while(1) {
        // Read the next character from the receive buffer.
        if(!RX_BUFFER_EMPTY) {
            character = uartrxbuffer[uartrxreadindex];
            ADVANCE_RX_BUFFER_INDEX(uartrxreadindex);

            // See if a newline or escape character was received.
            if((character == '\r') || (character == '\n') || (character == 0x1b)) {
                // Stop processing the input and end the line.
                break;
            }

            // Process the received character as long as we are not at the end
            // of the buffer.  If the end of the buffer has been reached then
            // all additional characters are ignored until a newline is
            // received.
            if(i < len) {
                // Store the character in the caller supplied buffer.
                buf[i] = character;

                // Increment the count of characters received.
                i++;
            }
        }
    }

    // Add a null termination to the string.
    buf[i] = 0;

    // Return the count of int8_ts in the buffer, not counting the trailing 0.
    return(i);
}

uint8_t uartstdio_getc(void) {
    uint8_t character;

    // Wait for a character to be received.
    while(RX_BUFFER_EMPTY) // Block waiting for a character to be received (if the buffer is currently empty).
        ;

    // Read a character from the buffer.
    character = uartrxbuffer[uartrxreadindex];
    ADVANCE_RX_BUFFER_INDEX(uartrxreadindex);

    // Return the character to the caller.
    return(character);
}

void uartstdio_vprintf(const char *str_ptr, va_list vaArgP) {
    defuint_t i, value, position, count, base, negative;
    char *char_ptr, charbuf[16], fillcharacter;

    // Check the arguments.
    ASSERT(str_ptr != 0);

    // Loop while there are more characters in the string.
    while(*str_ptr) {
        // Find the first non-% character, or the end of the string.
        for(i = 0;(str_ptr[i] != '%') && (str_ptr[i] != '\0'); i++)
            ;

        // Write this portion of the string.
        uartstdio_write(str_ptr, i);

        // Skip the portion of the string that was written.
        str_ptr += i;

        // See if the next character is a %.
        if(*str_ptr == '%') {
            // Skip the %.
            str_ptr++;

            // Set the digit count to zero, and the fill character to space
            // (in other words, to the defaults).
            count = 0;
            fillcharacter = ' ';

            // It may be necessary to get back here to process more characters.
            // Goto's aren't pretty, but effective.  I feel extremely dirty for
            // using not one but two of the beasts.
again:

            // Determine how to handle the next character.
            switch(*str_ptr++) {
                // Handle the digit characters.
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':

                    // If this is a zero, and it is the first digit, then the
                    // fill character is a zero instead of a space.
                    if((str_ptr[-1] == '0') && (count == 0)) {
                        fillcharacter = '0';
                    }

                    // Update the digit count.
                    count *= 10;
                    count += str_ptr[-1] - '0';

                    // Get the next character.
                    goto again;

                case 'c':

                    // Get the value from the varargs.
                    value = va_arg(vaArgP, uint32_t);

                    // Print out the character.
                    uartstdio_write((char *)&value, 1);
                    break;

                case 'd':
                case 'i': 
                    // Get the value from the varargs.
                    value = va_arg(vaArgP, defuint_t);

                    // Reset the buffer position.
                    position = 0;

                    // If the value is negative, make it positive and indicate
                    // that a minus sign is needed.
                    if((int32_t)value < 0) {
                        // Make the value positive.
                        value = -(int32_t)value;

                        // Indicate that the value is negative.
                        negative = 1;
                    } else {
                        // Indicate that the value is positive so that a minus
                        // sign isn't inserted.
                        negative = 0;
                    }

                    // Set the base to 10.
                    base = 10;

                    // Convert the value to ASCII.
                    goto convert;

                case 's':
                    // Get the string pointer from the varargs.
                    char_ptr = va_arg(vaArgP, char *);

                    // Determine the length of the string.
                    for(i = 0; char_ptr[i] != '\0'; i++)
                        ;

                    // Write the string.
                    uartstdio_write(char_ptr, i);

                    // Write any required padding spaces
                    if(count > i) {
                        count -= i;
                        while(count--) {
                            uartstdio_write(" ", 1);
                        }
                    }
                    break;

                case 'u':
                    // Get the value from the varargs.
                    value = va_arg(vaArgP, defuint_t);

                    // Reset the buffer position.
                    position = 0;

                    // Set the base to 10.
                    base = 10;

                    // Indicate that the value is positive so that a minus sign
                    // isn't inserted.
                    negative = 0;

                    // Convert the value to ASCII.
                    goto convert;

                // Handle the %x and %X commands.  Note that they are treated
                // identically; in other words, %X will use lower case letters
                // for a-f instead of the upper case letters it should use.  We
                // also alias %p to %x.
                case 'x':
                case 'X':
                case 'p':
                    // Get the value from the varargs.
                    value = va_arg(vaArgP, defuint_t);

                    // Reset the buffer position.
                    position = 0;

                    // Set the base to 16.
                    base = 16;

                    // Indicate that the value is positive so that a minus sign
                    // isn't inserted.
                    negative = 0;

                    // Determine the number of digits in the string version of
                    // the value.
convert:
                    for(i = 1; (((i * base) <= value) && (((i * base) / base) == i)); i *= base, count--)
                        ;

                    // If the value is negative, reduce the count of padding
                    // characters needed.
                    if(negative) {
                        count--;
                    }

                    // If the value is negative and the value is padded with
                    // zeros, then place the minus sign before the padding.
                    if(negative && (fillcharacter == '0')) {
                        // Place the minus sign in the output buffer.
                        charbuf[position++] = '-';

                        // The minus sign has been placed, so turn off the
                        // negative flag.
                        negative = 0;
                    }

                    // Provide additional padding at the beginning of the
                    // string conversion if needed.
                    if((count > 1) && (count < 16)) {
                        for(count--; count; count--) {
                            charbuf[position++] = fillcharacter;
                        }
                    }

                    // If the value is negative, then place the minus sign
                    // before the number.
                    if(negative) {
                        // Place the minus sign in the output buffer.
                        charbuf[position++] = '-';
                    }

                    // Convert the value into a string.
                    for(; i; i /= base) {
                        charbuf[position++] =
                            g_hex[(value / i) % base];
                    }

                    // Write the string.
                    uartstdio_write(charbuf, position);
                    break;

                case '%':
                    // Simply write a single %.
                    uartstdio_write(str_ptr - 1, 1);
                    break;

                default:
                    // Indicate an error.
                    uartstdio_write("ERROR", 5);
                    break;
            }
        }
    }
}

void uartstdio_printf(const char *char_ptr, ...) {
    va_list vaArgP;

    // Start the varargs processing.
    va_start(vaArgP, char_ptr);

    uartstdio_vprintf(char_ptr, vaArgP);

    // We're finished with the varargs now.
    va_end(vaArgP);
}

defint_t uartstdio_rxbytesavail(void) {
    return(RX_BUFFER_USED);
}
defint_t uartstdio_txbytesfree(void) {
    return(TX_BUFFER_FREE);
}

defint_t uartstdio_peek(unsigned char character) {
    defint_t count;
    defint_t available;
    defuint_t readindex;

    //
    // How many characters are there in the receive buffer?
    //
    available = (defint_t)RX_BUFFER_USED;
    readindex = uartrxreadindex;

    // Check all the unread characters looking for the one passed.
    for(count = 0; count < available; count++) {

        if(uartrxbuffer[readindex] == character) {
            // We found it so return the index
            return(count);
        } else {

            // This one didn't match so move on to the next character.
            ADVANCE_RX_BUFFER_INDEX(readindex);
        }
    }
    // If we drop out of the loop, we didn't find the character in the receive
    // buffer.
    return(-1);
}

void uartstdio_flushrx(void) {
    defuint_t temp;

    // Temporarily turn off interrupts.
    temp = MAP_IntMasterDisable();

    // Flush the receive buffer.
    uartrxreadindex = 0;
    uartrxwriteindex = 0;

    // If interrupts were enabled when we turned them off, turn them
    // back on again.
    if(!temp) {
        MAP_IntMasterEnable();
    }
}

void uartstdio_flushtx(bool is_discard) {
    defuint_t temp;

    // Should the remaining data be discarded or transmitted?
    if(is_discard) {

        // The remaining data should be discarded, so temporarily turn off
        // interrupts.
        temp = MAP_IntMasterDisable();

        // Flush the transmit buffer.
        uarttxreadindex = 0;
        uarttxwriteindex = 0;

        // If interrupts were enabled when we turned them off, turn them
        // back on again.
        if(!temp) {
            MAP_IntMasterEnable();
        }
    } else {

        // Wait for all remaining data to be transmitted before returning.
        while(!TX_BUFFER_EMPTY)
            ;
    }
}

void uartstdio_echoset(bool enable) {
    g_disable_echo = !enable;
}

void uartstdio_isr(void) {
    defuint_t status;
    int8_t character;
    defint_t store;
    static bool lastwas_carriage_return = false;

    // Get and clear the current interrupt source(s)
    status = MAP_UARTIntStatus(g_uartbase, true);
    MAP_UARTIntClear(g_uartbase, status);

    // Are we being interrupted because the TX FIFO has space available?
    if(status & UART_INT_TX) {
        // Move as many bytes as we can into the transmit FIFO.
        uartstdio_primetransmit(g_uartbase);

        // If the output buffer is empty, turn off the transmit interrupt.
        if(TX_BUFFER_EMPTY) {
            MAP_UARTIntDisable(g_uartbase, UART_INT_TX);
        }
    }

    // Are we being interrupted due to a received character?
    if(status & (UART_INT_RX | UART_INT_RT)) {
        // Get all the available characters from the UART.
        while(MAP_UARTCharsAvail(g_uartbase)) {
            // Read a character
            store = MAP_UARTCharGetNonBlocking(g_uartbase);
            character = (unsigned char)(store & 0xFF);

            // If echo is disabled, we skip the various text filtering
            // operations that would typically be required when supporting a
            // command line.
            if(!g_disable_echo) {
                // Handle backspace by erasing the last character in the
                // buffer.
                if(character == '\b') {
                    // If there are any characters already in the buffer, then
                    // delete the last.
                    if(!RX_BUFFER_EMPTY) {
                        // Rub out the previous character on the users
                        // terminal.
                        uartstdio_write("\b \b", 3);

                        // Decrement the number of characters in the buffer.
                        if(uartrxwriteindex == 0) {
                            uartrxwriteindex = UART_RX_BUFFER_SIZE - 1;
                        } else {
                            uartrxwriteindex--;
                        }
                    }

                    // Skip ahead to read the next character.
                    continue;
                }

                // If this character is LF and last was CR, then just gobble up
                // the character since we already echoed the previous CR and we
                // don't want to store 2 characters in the buffer if we don't
                // need to.
                if((character == '\n') && lastwas_carriage_return) {
                    lastwas_carriage_return = false;
                    continue;
                }

                // See if a newline or escape character was received.
                if((character == '\r') || (character == '\n') || (character == 0x1b)) {
                    // If the character is a CR, then it may be followed by an
                    // LF which should be paired with the CR.  So remember that
                    // a CR was received.
                    if(character == '\r') {
                        lastwas_carriage_return = 1;
                    }

                    // Regardless of the line termination character received,
                    // put a CR in the receive buffer as a marker telling
                    // UARTgets() where the line ends.  We also send an
                    // additional LF to ensure that the local terminal echo
                    // receives both CR and LF.
                    character = '\r';
                    uartstdio_write("\n", 1);
                }
            }

            // If there is space in the receive buffer, put the character
            // there, otherwise throw it away.
            if(!RX_BUFFER_FULL) {

                // Store the new character in the receive buffer
                uartrxbuffer[uartrxwriteindex] =
                    (unsigned char)(store & 0xFF);
                ADVANCE_RX_BUFFER_INDEX(uartrxwriteindex);

                // If echo is enabled, write the character to the transmit
                // buffer so that the user gets some immediate feedback.
                if(!g_disable_echo) {
                    uartstdio_write((const char *)&character, 1);
                }
            }
        }

        // If we wrote anything to the transmit buffer, make sure it actually
        // gets transmitted.
        uartstdio_primetransmit(g_uartbase);
        MAP_UARTIntEnable(g_uartbase, UART_INT_TX);
    }
}