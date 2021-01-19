/***************************************************************************
File Name: br-util.c

Description:
	This program is a brightness control example on MPC-2070, MPC-2120, MPC-2101 and MPC-2121.
	The brightness can only set in manually control mode.

History:
	Version	Author	Date		Comment
	1.0	Jared	01-29-2019	Set brightness example
***************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "br.h"

#define VERSION "1.0"

int main(int argc, char *argv[]) {
	int i,  mc_fd;

	mc_fd = open_brightness_control();
	if( mc_fd < 0 ) {
		if ( mc_fd == -1 )
			printf("%s opened fail.\n", BRIGHTNESS_DEVICE);
		else if( mc_fd == -2)
			printf("Please close another process using %s.\n", BRIGHTNESS_DEVICE);
		goto __exit2;
	}


	/* Set brightness manually in Auto Brightness Mode disabled. */
	for ( i=MIN_BRIGHTNESS_VALUE; i<= MAX_BRIGHTNESS_VALUE; i++) {
		if ( i == 0 )
			printf("Turn off the backlight at level %d.\n", i);
		else
			printf("Set brightness: %d.\n", i);
		sleep(1);
		set_brightness(mc_fd, i);
	}

__exit1:
	close_brightness_control(mc_fd);
__exit2:
	return 0;
}
