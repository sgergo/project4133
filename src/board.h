#ifndef BOARD_H
#define BOARD_H

#define RED 	0
#define GREEN 	1
typedef uint32_t led_t;

#define LEDBLINKPERIODMS 100

void board_toggle_led(void);
void board_blink_led(void);
void board_delay_us (uint32_t delay);
void board_delay_ms (uint32_t delay);
void board_systick_isr(void);
void board_init(void);

#endif