#include <stdint.h>
#include "types.h"
#include "boardconfig.h"
#include "board.h"
#include "board_rtc.h"
#include "console.h"
#include "task.h"
#include "msp430fr4133.h"

uint32_t const delayloopspermicrosecond = 1;	// 10us, measured with scope @ MCLK = 8 MHz
uint32_t const delayloopspermillisecond = 100; // Measured with scope @ MCLK = 8 MHz

void board_delay_us(uint32_t delay) {
	uint64_t volatile dly = delay * delayloopspermicrosecond;
	while(dly--)
		;
}

void board_delay_ms(uint32_t delay) {
	uint64_t volatile dly = delay * delayloopspermillisecond;
	while(dly--)
		;
}

static void board_configure_leds(void) {
	// The LED1 (red) is not configured since it shares pin with UART TX @ P1.0

	LEDPORT4DIR |= LEDPORT4PIN0;
	LEDPORT4OUT &=~LEDPORT4PIN0;

	// Diagnostic clock outputs - not necessary
	P1DIR |= BIT4;                   		// set MCLK pin as output
	P1SEL0 |= BIT4;                         // set MCLK pin as second function
    P8DIR |= BIT0 | BIT1;                   // set ACLK and SMCLK pin as output
    P8SEL0 |= BIT0 | BIT1;                  // set ACLK and SMCLK pin as second function
}

void board_toggle_led(void) {
	LEDPORT4OUT ^=LEDPORT4PIN0;
}

void board_blink_led(void) {

	LEDPORT4OUT |=LEDPORT4PIN0;
	board_delay_ms(LEDBLINKPERIODMS);
	LEDPORT4OUT &=~LEDPORT4PIN0;
	board_delay_ms(LEDBLINKPERIODMS);
}

void board_systick_isr(void) {
	task_systick();
}

static void board_set_clock_8_mhz(void) {

	P4SEL0 |= BIT1 | BIT2;                  // set XT1 pin as second function

    do {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);      // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);               // Test oscillator fault flag


	__bis_SR_register(SCG0);                // disable FLLUNLOCK
    // CSCTL3 |= SELREF__REFOCLK;              // No XTAL, so we set REFO (~32768kHz) 
    										// as FLL reference source
    CSCTL3 |= SELREF__REFOCLK;               // 32786khz XTAL is present, so we use it 
    										// as FLL reference source
    CSCTL0 = 0;                             // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                 // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_3;                    // Set DCO = 8MHz
    CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // enable FLL

    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) // Poll until FLL is locked
    	;

    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
                                            // default DCODIV as MCLK and SMCLK source
    __delay_cycles(8000);
}

void board_init(void) {
  	board_set_clock_8_mhz();
  	PM5CTL0 &= ~LOCKLPM5; // Important!
	board_configure_leds();
	board_rtc_init();
	console_init();
	console_printtext("\n\n\n**** MSP430FR4133  template ****\n");
  	console_printtext("***** System clock: %d MHz *****\n", 8);
  	//console_testprint("MUXIK",6);
	
	__bis_SR_register(GIE); // Enable global interrupt
}

