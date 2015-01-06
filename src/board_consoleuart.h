#ifndef BOARD_UART_H
#define BOARD_UART_H

void board_consoleuart_enable_tx_interrupt (void);
void board_consoleuart_disable_tx_interrupt (void);
defuint_t board_consoleuart_txfifo_spaceavail(void);
void board_consoleuart_write_blocking (uint8_t data);
uint16_t board_consoleuart_getcharfromrxbuf(void);
void board_consoleuart_init(void);

#endif //BOARD_UART_H