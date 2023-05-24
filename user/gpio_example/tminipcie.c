/*
 * This program is demo how to used the mini-PCIE GPIO control in Linux operating system.
 * The DIO control entries are exported at
 *
 *  /sys/class/gpio/minipcie/value
 *  /sys/class/gpio/minipcie/value
 *
 * These are the wrappering functions for mini-PCIE power
 *
 * int set_minipcie_state(char state)
 * int get_minipcie_state(char *state)
 *
 * History:
 * Date		Author		Comment
 * 09-21-2016	Jared Wu.	Create it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "gpio.h"

int main() {

	int i;
	char state;

	while( 1 ) {

		printf("\nSelect a number of menu, other key to exit.	\n\
	1. Power off mini-PCIE module.			\n\
	2. Power on mini-PCIE module.			\n\
	3. Get mini-PCIE power status.			\n\
	9. Quit					\n\
	Choose : ");

		scanf("%d", &i);

		switch(i) {
			case 1:
				set_minipcie_state('0');
				break;
			case 2:
				set_minipcie_state('1');
				break;
			case 3:
				get_minipcie_state(&state);
				printf("	The mini-PCIE power now is %c \n", state);
				break;
			case 9:
				return 0;
			default:
				continue;
		}
	}

	return 0;	
}
