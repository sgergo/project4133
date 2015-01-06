#include <stdint.h>
#include "types.h"
#include <stdbool.h>
#include "boardconfig.h"
#include "board_consoleuart.h"
#include "uartstdio.h"
#include "msp430fr4133.h"
#include "board.h"

// BRCLK    Baud Rate UCOS16 UCBRx UCBRFx UCBRSx
// 1000000 115200     0      8     -      0xD6
// 8000000 115200     1      4     5      0x55 
void board_consoleuart_enable_tx_interrupt (void) {
    UCA0IE |= UCTXIE;                         
}

void board_consoleuart_disable_tx_interrupt (void) {
    UCA0IE &=~UCTXIE;                         
}

defuint_t board_consoleuart_txfifo_spaceavail(void) {
    // check TX interrupt flag and negate the result
    // nonzero (true): TXFIFO is empty, zero (false): TXFIFO is busy 
    return ~(UCA0IFG & UCTXIFG);
}

void board_consoleuart_write_blocking (uint8_t data) {
    while(!(UCA0IFG & UCTXIFG))
        ;
    UCA0TXBUF = data;
    board_toggle_led();         
}

uint16_t board_consoleuart_getcharfromrxbuf (void) {
    return UCA0RXBUF;
}

void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void) {

    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
              UCA0IFG &=~ UCRXIFG;            // Clear interrupt
              // board_consoleuart_write_blocking (UCA0RXBUF);
              // UCA0TXBUF = UCA0RXBUF;
              uartstdio_rx_isr();
              break;
        case USCI_UART_UCTXIFG:
        	UCA0IFG &=~ UCTXIFG;            // Clear interrupt
          	//uartstdio_tx_isr();
        	break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

void board_consoleuart_init(void){

    // Configure UART pins as second function
    CONSOLEUARTPORTSEL |= (CONSOLEUARTTXPIN | CONSOLEUARTRXPIN);                    

    // Configure UART
    // Put eUSCI in reset
    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;

    // Baud Rate calculation for 115200
    //UG: UCBRSx:0x55, UCBFx:5, UCOS16:1 
    UCA0MCTLW = (0x55<<8) | (0x05<<4) | 0x01;
    //UG: UCBRx:4
    UCA0BR0 = 4;    
    UCA0BR1 = 0;

    // UCA0BR0 = 8;                              // 1000000/115200 = 8.68
    // UCA0MCTLW = 0xD600;                       // 1000000/115200 - INT(1000000/115200)=0.68
    //                                           // UCBRSx value = 0xD6 (See UG)
    // UCA0BR1 = 0;
    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
    //board_consoleuart_enable_tx_interrupt();
}