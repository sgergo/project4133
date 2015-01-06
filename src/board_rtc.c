#include <stdint.h>
#include "types.h"
#include "board.h"
#include "msp430fr4133.h"

void __attribute__ ((interrupt(RTC_VECTOR))) RTC_ISR (void) {

    switch(__even_in_range(RTCIV,RTCIV_RTCIF))
    {
        case  RTCIV_NONE:   break;          // No interrupt
        case  RTCIV_RTCIF:                  // RTC Overflow
            board_systick_isr();
            break;
        default: break;
    }
}

void board_rtc_init(void) {

	/* 	f_aclk = 32768 Hz
	*	100 ms = 32768 / 10 = 3276.8
	*   thus RTCPS divider :=1, RTCMOD = 3277 */

    RTCCTL = RTCSS__XT1CLK | RTCSR | RTCPS__1 | RTCIE;
    RTCMOD = 3277;
}