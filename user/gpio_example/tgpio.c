/*
 * This program is demo how to used the GPIO control in Linux operating system.
 * The DIO control entries are exported at
 *
 *  /sys/class/gpio/doN/value
 *  /sys/class/gpio/diN/value
 *
 * The wrappering functions for DI/DO control are
 *
 * int set_do_state(int do_port, char state)
 * int get_di_state(int do_port, char *state)
 * int get_di_state(int di_port, char *state)
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

	int i, j, retval;
	char state;

	while( 1 ) {

		printf("\nSelect a number of menu, other key to exit.	\n\
	1. set low data.			\n\
	2. set high data.			\n\
	3. get now data.			\n\
	9. quit					\n\
	Choose : ");

		retval =0;
		scanf("%d", &i);

		switch(i) {
			case 1:
				printf("Please keyin the DOUT number : ");
				scanf("%d", &i);
				retval = set_do_state(i, '0');
				break;
			case 2:
				printf("Please keyin the DOUT number : ");
				scanf("%d", &i);
				retval = set_do_state(i, '1');
				break;
			case 3:
				printf("DIN data(%d)  : ", MAX_DIN_PORT);
				for ( j=1; j<=MAX_DIN_PORT; j++ ) {
					get_di_state(j, &state);
					printf("%c ", state);
				}
				printf("\n");
				printf("DOUT data : ");
				for ( j=1; j<=MAX_DOUT_PORT; j++ ) {
					get_do_state(j, &state);
					printf("%c ", state);
				}
				printf("\n");
				break;
			case 9:
				return 0;
			default:
				continue;
		}

	}

	return 0;	
}
