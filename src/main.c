#include <stdint.h>
#include "types.h"
#include "board.h"
#include "task.h"
#include "msp430fr4133.h"

int main() {

	WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer  
    board_init();

    while(1)
    	task_process();
        
    return 0;
}
