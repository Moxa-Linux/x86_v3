/***************************************************************************
File Name: br-util.c

Description:
	This program is a light sensor/brightness control utility on MPC-2121.

Usage:
	1. Compile the code and execute it on MPC.
	2. Follow the br-util usage message to do brightness controll.
	EX: br-util -m
	EX: br-util -s 5
	EX: br-util -g
	EX: br-util -w 1,2,3,4,5,6,7,8
	EX: br-util -l
	EX: br-util -f

History:
	Version	Author	Date		Comment
	1.0	Jared	10-04-2018	lightsensor/brightness control utility.
	1.1	Jared	01-28-2019	Add quiet mode for UI to launch this utility.
	1.2	Jared	01-29-2019	Add get/set holdtime feature.
	1.3	Jared	05-01-2019	Fix the supportBrightnessControl = 1; condition bug.
	1.4	Jared	10-15-2019	Add the '-L' option to get the current lightsensor level value.
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

#define VERSION "1.4"
#define MAX_RETRY 5

void usage(char *progname, int supportAutoBrightness) {
	printf("Usage: %s [options]\n", progname);
	printf("MPC serials brightness control utility, %s Build %s.\n", VERSION, BUILTDATE);
	printf("options:\n");
	if ( supportAutoBrightness) {
		printf("  -m - Check the brightness controller working mode.\n");
		printf("        0 - Auto Brightness control is disabled (Default).\n");
		printf("        1 - Auto Brightness control is enabled.\n");
		printf("  -e - Enable Auto Brightness Mode.\n");
		printf("  -d - Disable Auto Brightness Mode.\n");
		printf("  -t <holdtime>\n");
		printf("        The holdtime is the brightness level change interval. holdtime=%d, %d,...,%d. Default is 10.\n", MIN_HOLDTIME_VALUE, MIN_HOLDTIME_VALUE+1, MAX_HOLDTIME_VALUE);
	        printf("        (eg: br-util -t 3)\n");
		printf("  -v - Get Auto Brightness control holdtime.\n");
		printf("  -w <b1,b2,b3,b4,b5,b6,b7,b8>\n");
	        printf("        Apply and write the brightness level for automatic brightness control mode.\n");
		printf("        The brightness value would be keep in the brightness controller even reboot.\n");
		printf("	bn, n=1, 2, ...,8.\n");
		printf("	%d - The darkest level.\n", MIN_BRIGHTNESS_VALUE+1);
		printf("	%d - The second darkest level.\n", MIN_BRIGHTNESS_VALUE+2);
		printf("	...\n");
		printf("	%d - the brightest level.\n", MAX_BRIGHTNESS_VALUE);
	        printf("        (Set the default level, eg: br-util -w 2,5,5,7,7,9,9,9)\n");
		printf("  -l - Load the brightness level from brightness controller using in automatic brightness control.\n");
	}
	printf("  -s <level>\n");
	printf("        Set brightness level manually in Disabled Auto Brightness Mode. Default is 7.\n");
	printf("        %d - Turn off the brightness.\n", MIN_BRIGHTNESS_VALUE);
	printf("        %d - The darkest level.\n", MIN_BRIGHTNESS_VALUE+1);
	printf("	%d - The second darkest level.\n", MIN_BRIGHTNESS_VALUE+2);
	printf("	...\n");
	printf("        %d - The brightest level.\n", MAX_BRIGHTNESS_VALUE);
	printf("        (eg: br-util -s 7)\n");
	printf("  -g - Get the brightness level manually in Disabled Auto Brightness Mode.\n");
	printf("  -L - Get the current lightsensor level value in Auto Brightness Mode.\n");
	printf("  -p - Get the brightness value if the brightness key pressed.\n");
	printf("  -q - Quiet mode. This option should used with other options.\n");
	printf("  -f - Get brightness controller firmware version.\n");
	printf("  -u - Upgrade brightness controller firmware.\n");
	printf("        Note: Don't use this command alone. It does not include the ASCII protocol to transfer the firmware.\n");
	printf("        Use the upgrade_mcfwr.sh to upgrade firmware.\n");
	printf("  -H - Get brightness controller system status.\n");
}

extern int optind, opterr, optopt; 
extern char *optarg;
	
int main(int argc, char *argv[]) {
	int quietMode = 0;
	int setBrightness = 0;
	int getBrightness = 0;
	int setHoldtime = 0;
	int getHoldtime = 0;
	int getFirmwareVersion = 0;
	int upgradeFirmware = 0;
	int pressBrightnessKey = 0;
	int getBrightnessSystemStatus = 0;
	int supportBrightnessControl = 0;
	int checkMode = 0;
	int enableAutoBrightnessControl = 0;
	int disableAutoBrightnessControl = 0;
	int loadBrightnessLevel = 0;
	int writeBrightnessLevel = 0;
	int getLightsensorLevel = 0;
	int showUsage = 0;
	int b[MAPPING_NUM];
	int i, value, system_status, frm_ver, mc_fd, retry, retval = 0, holdtime;
	fd_set read_set;
	struct timeval tv;
	char c, optstring[] = "qmeds:gw:lLfuphHt:v";

	while ((c = getopt(argc, argv, optstring)) != -1)
		switch (c) {
		case 'm':
			checkMode = 1;
			break;
		case 'e':
			enableAutoBrightnessControl = 1;
			break;
		case 'd':
			disableAutoBrightnessControl = 1;
			break;
		case 'w':
			writeBrightnessLevel = 1;

			/* Get the brightness level from STDIN */
			sscanf(optarg, "%d,%d,%d,%d,%d,%d,%d,%d", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7] );

			for ( i=0; i < MAPPING_NUM; i++ ) {
				if ( (b[i] < MIN_BRIGHTNESS_VALUE || b[i] > MAX_BRIGHTNESS_VALUE) ) {
					printf("The %dth brightness level in %d ~ %d.\n", i, MIN_BRIGHTNESS_VALUE, MAX_BRIGHTNESS_VALUE);
					goto __exit1;
				}
			}

			break;
		case 'l':
			loadBrightnessLevel = 1;
			break;
		case 'L':
			getLightsensorLevel = 1;
			break;
		case 'H':
			getBrightnessSystemStatus = 1;
			break;
		case 'q':
			quietMode = 1;
			break;
		case 's':
			setBrightness = 1;

			sscanf(optarg, "%d", &value);
			if ( value < MIN_BRIGHTNESS_VALUE || value > MAX_BRIGHTNESS_VALUE ) {
				printf("The brightness level, %d, should be %d, %d,..., %d.\n", value, MIN_BRIGHTNESS_VALUE, MIN_BRIGHTNESS_VALUE+1, MAX_BRIGHTNESS_VALUE);
				goto __exit1;
			}

			break;
		case 'g':
			getBrightness = 1;
			break;
		case 't':
			setHoldtime = 1;

			sscanf(optarg, "%d", &holdtime);
			if ( holdtime < MIN_HOLDTIME_VALUE || holdtime > MAX_HOLDTIME_VALUE ) {
				printf("The hold time value, %d, should be %d, %d, ..., %d.\n", holdtime, MIN_HOLDTIME_VALUE, MIN_HOLDTIME_VALUE+1, MAX_HOLDTIME_VALUE);
				goto __exit1;
			}

			break;
		case 'v':
			getHoldtime = 1;
			break;
		case 'p':
			pressBrightnessKey = 1;
			break;
		case 'f':
			getFirmwareVersion = 1;
			break;
		case 'u':
			upgradeFirmware = 1;
			break;
		case 'h':
		default:
			showUsage = 1;
		}

	mc_fd = open_brightness_control();
	if( mc_fd < 0 ) {
		if ( quietMode )
			printf("%d", mc_fd);
		else if ( mc_fd == -1 )
			printf("%s opened fail.\n", BRIGHTNESS_DEVICE);
		else if( mc_fd == -2)
			printf("Please close another process using %s.\n", BRIGHTNESS_DEVICE);
		goto __exit2;
	}

	retval = get_brightness_system_status(mc_fd, &system_status);
	/* If the 2 LSB are set, it support auto brightness control feature */
	if ( retval == 0 && (system_status & 0x03) == 0x03 ) 
		supportBrightnessControl = 1;


	/* Check the brightness controller working mode. */
	if( supportBrightnessControl == 1 && checkMode == 1 ) {
		if ( !quietMode ) {
			printf("The auto brightness control feature is %s.\n", get_auto_brightness_control_status(mc_fd) ? "Enabled" : "Disabled" );
		}
		else {
			printf("%d", get_auto_brightness_control_status(mc_fd) );
		}
	}


	if ( showUsage ) {
		usage(argv[0], supportBrightnessControl);
		return 0;
	}

	/* Enable brightness controller Auto Brightness Mode */
	if ( supportBrightnessControl == 1 && enableAutoBrightnessControl == 1 ) {
		if ( !quietMode )
			printf("Enable the auto brightness control feature.\n");

		retry = 0;
		do {
			retval = set_auto_brightness_control(mc_fd, 1);
			if ( retval < 0 ) {
				printf("enable auto brightness control fail. retval:%d.\n", retval);
				sleep_ms(100);
			}
			retry ++;
			if( retry >= MAX_RETRY )
				break;
		} while ( retval < 0 );
	}

	/* Disable brightness controller Auto Brightness Mode */
	if ( supportBrightnessControl == 1 && disableAutoBrightnessControl == 1 ) {
		if ( !quietMode )
			printf("Disable the auto brightness control feature.\n");
		retry = 0;
		do {
			retval = set_auto_brightness_control(mc_fd, 0);
			if ( retval < 0 ) {
				if ( !quietMode )
					printf("disable auto brightness control fail. retval:%d.\n", retval);
				sleep_ms(100);
			}
			retry ++;
			if( retry >= MAX_RETRY )
				break;
		} while ( retval < 0 );
	}

	/* Apply and write the brightness level for automatic brightness control mode. */
	if ( supportBrightnessControl == 1 && writeBrightnessLevel == 1 ) {

		/* Apply these value to brightness controller. */ 
		if ( !quietMode )
			printf("Set auto brightness level: %d %d %d %d %d %d %d %d.\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);

		for ( i=0; i < MAPPING_NUM; i++ )
			set_auto_brightness_level(mc_fd, i+1, b[i]);
	}

	/* Load the brightness level from brightness controller and output to STDOUT. */
	if ( supportBrightnessControl == 1 && loadBrightnessLevel == 1 ) {
		for ( i=0; i < MAPPING_NUM; i++ ) {
			get_auto_brightness_level(mc_fd, i+1, &b[i]);
			if ( !quietMode ) {
				printf("The light sensor level %d is mapped at brightness level: %d.", i+1, b[i]);
			}
			else {
				printf("%d ", b[i]);
			}
			printf("\n");
		}
	}

	/* Get the current lightsensor level value from brightness controller and output to STDOUT. */
	if ( supportBrightnessControl == 1 && getLightsensorLevel == 1 ) {
		retval = get_lightsensor_level(mc_fd, &value);
		if ( !quietMode && retval < 0 )
			printf("get_auto_brightness_control_holdtime() fail.\n");

		if ( !quietMode )
			printf("The current light sensor level value is %d.\n", value);
		else
			printf("%d", value);

	}

	/* Set brightness manually in Auto Brightness Mode disabled. */
	if ( setBrightness == 1 ) {
		if ( supportBrightnessControl == 1 && get_auto_brightness_control_status(mc_fd) == 1 ) {
			printf("The brightness level cannot access because of the auto-brightness control mode is Enabled.\n");
			goto __exit1;
		}

		if ( !quietMode )
			printf("Set brightness: %d.\n", value);

		set_brightness(mc_fd, value);
	}

	/* Get brightness manually in Auto Brightness Mode disabled. */
	if ( getBrightness == 1 ) {
		if ( supportBrightnessControl == 1 && get_auto_brightness_control_status(mc_fd) == 1 ) {
			if ( !quietMode )
				printf("The light sensor level cannot access because of the auto-brightness control mode is Enabled.\n");
			goto __exit1;
		}

		retval = get_brightness(mc_fd, &value);
		if ( !quietMode && retval < 0 )
			printf("get_brightness() fail.\n");

		if ( !quietMode )
			printf("The brightness: %d\n", value);
		else
			printf("%d", value);
	}

	/* Set holdtime. */
	if ( setHoldtime == 1 ) {

		if ( !quietMode )
			printf("Set holdtime: %d\n", holdtime);

		set_auto_brightness_control_holdtime(mc_fd, holdtime);
	}

	/* Get holdtime. */
	if ( getHoldtime == 1 ) {

		retval = get_auto_brightness_control_holdtime(mc_fd, &holdtime);
		if ( !quietMode && retval < 0 )
			printf("get_auto_brightness_control_holdtime() fail.\n");

		if ( !quietMode )
			printf("The holdtime: %d\n", holdtime);
		else
			printf("%d", holdtime);
	}

	/* Get the brightness value if key pressed */
	if ( pressBrightnessKey == 1 ) {
		if ( supportBrightnessControl == 1 && get_auto_brightness_control_status(mc_fd) == 1 ) {
			if ( !quietMode )
				printf("The brightness level cannot read because of the auto-brightness control mode is Enabled.\n");
			goto __exit1;
		}

		if ( !quietMode )
			printf("Please press the brightness up/down button.\n");
		
		FD_ZERO(&read_set);
		FD_SET(mc_fd, &read_set);

		tv.tv_sec = SELECT_TIMEOUT;
		retval = select( mc_fd + 1, &read_set, NULL, NULL, NULL); // &tv

		if ( retval < 0 ) {
			if ( !quietMode )
				perror("select fail.\n");
			goto __exit1;
		}
		else if ( retval == 0 ) {
			if ( !quietMode )
				perror("It never goes to the timeout condition.\n");
			goto __exit1;
		}
		else {
			if ( FD_ISSET(mc_fd, &read_set) ) {
				/* Read the brightness value if the brightness key were pressed */
				get_brightness_in_brkey_pressed(mc_fd, &value);
				if ( !quietMode ) {
					printf("Brightness value: %d\n", value);
				}
				else {
					printf("%d", value);
				}
			}
			else {
				printf("Exception.\n");
				goto __exit1;
			}
		}
	}

	/* Get brightness controller firmware version. */
	if ( getFirmwareVersion == 1 ) {
		get_firmware_version(mc_fd, &frm_ver);
		if ( !quietMode )
			printf("The firmware version: %d.%d.%dS%02d (%x).\n", (frm_ver & 0xF000)>>12, (frm_ver & 0x0F00)>>8, (frm_ver & 0x00F0)>>4, (frm_ver & 0x000F), frm_ver);
		else
			printf("%d.%d.%dS%02d", (frm_ver & 0xF000)>>12, (frm_ver & 0x0F00)>>8, (frm_ver & 0x00F0)>>4, (frm_ver & 0x000F));
	}

	/* Upgrade brightness controller firmware. */
	if ( upgradeFirmware == 1 ) {
		retval = start_firmware_upgrade(mc_fd);
		if ( !quietMode  ) {
			if ( retval < 0 )
				printf("Brightness controller cannot start the firmware upgrade. [%d].\n", retval);
			else
				printf("Continue the Brightness controller firmware upgrade.\n");
		}
		else {
			printf("%d", retval);
		}
	}

	/* Get brightness controller system status. */
	if ( getBrightnessSystemStatus == 1 ) {

		retval = get_brightness_system_status(mc_fd, &value);

		if ( quietMode ) {
			if ( retval < 0 )
				printf("%d", retval);
			else
				printf("%d", value);
		}
		else {
			if ( retval < 0 ) {
				printf("Not Support auto brightness control.\n");
			}
			else {
				/* bit 0 - 0: panel power-off; 1: panel power-on */
				if ( value & SYSTEM_STATUS_PANEL_POWER_ON )
					printf("Panel: On\n");
				else
					printf("Panel: Off\n");

				/* bit 1 - 0: light sensor error; 1: light sensor OK */
				if ( value & SYSTEM_STATUS_LIGHT_SENSOR_OK )
					printf("Lightsensor: On\n");
				else
					printf("Lightsensor: Off\n");

				/* bit 2 - 0: No display output; 1: display output OK */
				if ( value & SYSTEM_STATUS_DISPLAY_OUTPUT_OK )
					printf("Display output: On\n");
				else
					printf("Display output: Off\n");
			}
		}
	}

__exit1:
	close_brightness_control(mc_fd);
__exit2:
	return 0;
}
