project4133
===========

MSP430FR4133 Launchpad project

FRAM size: 15.5 kB
SRAM size: 2 kB

--------------------

- TXD and RXD jumpers are not connected by default. use CTS/RTS jumpers to bridge, you won't need those pins anyway

- IMPORTANT: P1.0 is connected to the red LED (LED1) by default, besides being the UART TX pin. if you configure it to be the UART TX then you CAN'T use the red LED in the same time since it will be always switched on! the best practice is to remove the jumper from JP1 not to interfere with the UART operations.

- if the board is not programmable using 'make install' and you receive 'interface error 35' or similar error messages you may want to upgrade the ez-FET Lite fw in the board's programming interface. the easiest way is using MSPFlasher in Windows - it'll automatically detect the need of firmware upgrade and will fix it instantly

- debug channel is ttyACM0 - don't connect to it or else you can't program your board
- back channel UART is ttyACM1, capable of baud rate 115200

- get rid of modemmanager otherwise your back channel UART is not gonna work properly:
sudo apt-get purge modemmanager
you can always reinstall it by:
sudo apt-get install modemmanager
