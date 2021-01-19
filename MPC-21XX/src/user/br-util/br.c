#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/ioctl.h>
#include <sys/file.h>
#include <termios.h>
#include "br.h"

#if _POSIX_C_SOURCE >= 199309L
#include <time.h> // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

void sleep_ms( int milliseconds) {

#if _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	usleep(milliseconds * 1000);
#endif
}

/*
 * Open the brightness control device, /dev/ttyS2, in 9600n81.
 * return: the device descriptor.
 */
int open_brightness_control(void) {
	int fd;
	struct termios termio;

	fd = open(BRIGHTNESS_DEVICE, O_RDWR|O_NDELAY|O_CLOEXEC);
	if( fd < 0 ) {
		dbg_printf("%s opened fail\n", BRIGHTNESS_DEVICE);
		return -1;
	}
	
	// Exclusive the serial port accessing
	if(flock(fd, LOCK_EX|LOCK_NB) < 0) {
		dbg_printf("%s flock() lock fail\n", BRIGHTNESS_DEVICE);
		return -2;
	}

	// Initialize the serial
	termio.c_iflag = 0;
	termio.c_oflag = 0;
	termio.c_lflag = 0;
	termio.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
	termio.c_cc[VMIN] = 0;
	termio.c_cc[VTIME] = 0;

	tcsetattr(fd, TCSANOW, &termio);
	tcflush(fd, TCIOFLUSH);

	return fd;
}

/*
 * Close the brightness control device, /dev/ttyS2.
 */
void close_brightness_control(int fd) {
	// Exclusive the serial port accessing
	if( flock(fd, LOCK_UN) < 0 )
		dbg_printf("%s flock() unlock fail\n", BRIGHTNESS_DEVICE);

	close(fd);
}

/*
 *       X86                             uC
 * Get auto brightness control status
 *     0xF0 0x01 0xC2 0xC2 --->
 *                         <---  0xF0 0x02 0xC2 0x01 0xC3
 *
 * int fd: the device descriptor
 * return:
 *      0: Auto brightness control is disabled
 *      1: Auto brightness control is enabled
 *     -4: write brigthness command fail
 *     -5: read the response of brightness command fail
 *     -6: The response is not the expected value
 */

int get_auto_brightness_control_status(int fd) {
	unsigned char cmd[CMD_LEN]={0xF0, 0x01, 0xC2, 0, 0 };
	int retval = 0;

	cmd[3] += cmd[2];

	retval = write(fd, &cmd, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Queary auto brightness status command fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay (around 150ms) here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response of query auto brightness command fail. read() retval:%d\n", retval);
		return -5;
	}

	/* Check the response of brightness setting, the response should be 
	 * 0xF0, 0x02, 0xC2, auto brightness control status, checksum */
	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x02 && \
		cmd[2] != 0xC2 ) {
		dbg_printf("Get auto brightness control status fail: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
		return -6;
	}


	return (int) cmd[3];
}

/*
 *       X86                             uC
 * Enable auto brightness control
 *     0xF0 0x02 0xC3 0x01 0xC4  --->
 *                               <---  0xF0 0x01 0xC3 0xC3
 *
 * Disable auto brightness control
 *     0xF0 0x02 0xC3 0x00 0xC3  --->
 *                               <---  0xF0 0x00 0xC3 0xC3
 *                               <---  0xF0 0x01 0xC3 0xC3
 * int fd: the device descriptor
 * int on_off:
 *      0: disable the auto brightness control
 *      1: enable the auto brightness control
 *      2: enable the auto-sent brightness control (debugging purpose)
 * return:
 * 	0: success
 *     -2: on_off is not 0 or 1
 *     -4: write brigthness command fail
 *     -5: read the response of brightness command fail
 *     -6: The response is not the expected value
 */
int set_auto_brightness_control(int fd, int on_off) {
	/* Set brightness command is {0xF0, 0x02, 0xC3, 0/1, sum(cmd[4]+1) */
	unsigned char cmd[CMD_LEN]={0xF0, 0x02, 0xC3, 0, 0 };
	int retval = 0;

	if( on_off < 0 || on_off > 1) {
		printf("The on_off value should be 0 or 1\n");
		return -2;
	}

	/* The on_off value */
	cmd[3] = on_off;
	/* The checksum for command validate */
	cmd[4] = cmd[2]+cmd[3];

	retval = write(fd, &cmd, CMD_LEN);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Set brightness command fail: write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay (around 150ms) here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response of set auto brightness control command fail. read() retval:%d\n", retval);
		return -5;
	}

	/* 
	 * Check the response of brightness setting, the response should be 
	 * 0xF0, 0x01, 0xC3, 0xC3 */

	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x01 && \
		cmd[2] != 0xC3 && \
		cmd[3] != 0xC3 ) {
		dbg_printf("Set auto brightness control fail: %02x, %02x, %02x, %02x, %02x\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
		return -6;
	}

	return 0;
}

/*
 * Get current lightsensor level value.
 *
 *       X86                             uC
 *     0xF0 0x01 0xC5 0xC5  --->
 *                               <---  0xF0 0x02 0xC5 0x05 0xCA
 * int fd: the device descriptor
 * int *lightsensor_level: The current lightsensor level value.
 * return:
 *      0: Success
 *     -4: Write the GET BRIGHTNESS command fail
 *     -5: Read the response of set brightness command fail
 */
int get_lightsensor_level(int fd, int *lightsensor_level) {
	unsigned char cmd[CMD_LEN]={0xF0, 0x01, 0xC5, 0, 0 };
	int retval = 0;

	cmd[3] += cmd[2];

	retval = write(fd, &cmd, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Get brightness command fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of get lightsensor level command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response of get lightsensor level command fail. read() retval:%d\n", retval);
		return -5;
	}

	/* 
	 * Check the response of get lightsensor level, the response should be 
	 * 0xF0, 0x01, 0xC5, lightsensor_level */

	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x01 && \
		cmd[2] != 0xC5 ) {
		dbg_printf("Get lightsensor level response fail: %02x, %02x, %02x, %02x, %02x\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
		return -6;
	}

	dbg_printf("response: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);

	*lightsensor_level = (int) cmd[3];

	return 0;
}

/*
 * Set the brightness level
 *
 *       X86                             uC
 *     0xF0 0x02 0xC1 0x01 0xC2  --->
 *                               <---  0xF0 0x01 0xC1 0x01
 * int fd: the device descriptor
 * int br_value: The brightness level is in 0, 1, ..., 10.
 * return:
 *      0: Sucess
 *     -3: br_value is not in 1 ~ 10
 *     -4: Write brightness command fail
 *     -5: Read the response of set brightness command fail
 *     -6: Set brightness fail
 */
int set_brightness(int fd, int br_value) {

	/* Set brightness command is {0xF0, 0x02, 0xC1, brightness_value, sum(cmd[4]+1) */
	unsigned char cmd[CMD_LEN]={0xF0, 0x02, 0xC1, 0, 0 };
	int retval = 0;

	if( br_value > MAX_BRIGHTNESS_VALUE || br_value < MIN_BRIGHTNESS_VALUE ) {
		dbg_printf("The brightness value, %d, is not in 1  ~ %d\n", br_value, MAX_BRIGHTNESS_VALUE);
		return -3;
	}

	/* The brightness value is 0, 1, ... 10 */
	cmd[3] = br_value;
	/* The checksum for command validate in the light control module */
	cmd[4] = cmd[2]+cmd[3];

	retval = write(fd, &cmd, CMD_LEN);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Set brightness command fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response of set brightness command fail. read() retval:%d\n", retval);
		return -5;
	}

	/* 
	 * Check the response of brightness setting, the response should be 
	 * 0xF0, 0x01, 0xC1, brightness_value */

	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x01 && \
		cmd[2] != 0xC1 && \
		cmd[3] != br_value ) {
		dbg_printf("Set brightness response fail: %02x, %02x, %02x, %02x, %02x\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
		return -6;
	}

	return 0;
}

/*
 * Get the brightness level
 *
 *       X86                             uC
 *     0xF0 0x01 0xC0 0xC0  --->
 *                               <---  0xF0 0x02 0xC0 0x01
 * int fd: the device descriptor
 * int value: The brightness level is in 0, 1, ..., 10.
 * return:
 *      0: Success
 *     -4: Write the GET BRIGHTNESS command fail
 *     -5: Read the response of set brightness command fail
 */
int get_brightness(int fd, int *br_value) {
	unsigned char cmd[CMD_LEN]={0xF0, 0x01, 0xC0, 0, 0 };
	int retval = 0;

	cmd[3] += cmd[2];

	retval = write(fd, &cmd, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Get brightness command fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the brightness value fail. read() retval:%d\n", retval);
		return -5;
	}

	/* Check the response of brightness setting, the response should be 
	 * 0xF0, 0x01, 0xC0, brightness_value */

	dbg_printf("response: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);

	*br_value = (int) cmd[3];

	return 0;
}

/*
 * Set the hold time in auto brightness control mode.
 *
 *       X86                             uC
 *     0xF0 0x02 0xE9 0x0A 0xF3  --->
 *                               <---  0xF0 0x01 0xE9 0xE9
 * int fd: the device descriptor
 * int holdtime: The holdtime is the time interval to poll the brightness of the light source.
 * return:
 *      0: Sucess
 *     -3: holdtime is not in 1 ~ 10
 *     -4: Write brightness command fail
 *     -5: Read the response of set brightness command fail
 *     -6: Set brightness fail
 */
int set_auto_brightness_control_holdtime(int fd, int holdtime) {
	/* Set brightness command is {0xF0, 0x02, 0xE9, holdtime, sum(cmd[2], cmd[3]) */
	unsigned char cmd[CMD_LEN]={0xF0, 0x02, 0xE9, 0, 0 };
	int retval = 0;

	if( holdtime > MAX_HOLDTIME_VALUE || holdtime < MIN_HOLDTIME_VALUE ) {
		dbg_printf("The holdtime value, %d, should be %d, %d, ..., %d.\n", holdtime, MIN_HOLDTIME_VALUE, MIN_HOLDTIME_VALUE+1, MAX_HOLDTIME_VALUE);
		return -3;
	}

	/* The holdtime */
	cmd[3] = holdtime;
	/* The checksum for command validate in the light control module */
	cmd[4] = cmd[2]+cmd[3];

	retval = write(fd, &cmd, CMD_LEN);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Set holdtime command fail. write() retval:%d.\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response of set holdtime command fail. read() retval: %d.\n", retval);
		return -5;
	}

	/* 
	 * Check the response of brightness setting, the response should be 
	 * 0xF0 0x01 0xE9 0xE9 */

	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x01 && \
		cmd[2] != 0xE9 && \
		cmd[3] != 0xE9 ) {
		dbg_printf("Set holdtime response fail: %02x, %02x, %02x, %02x, %02x.\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
		return -6;
	}

	return 0;
}

/*
 * Get the hold time in auto brightness control mode.
 *
 *       X86                             uC
 *     0xF0 0x01 0xE9 0xE9  --->
 *                               <---  0xF0 0x02 0xE9 0x02 0xEB
 * int fd: the device descriptor
 * int *holdtime: The holdtime is the time interval to poll the brightness of the light source.
 * return:
 *      0: Success
 *     -4: Write the GET BRIGHTNESS command fail
 *     -5: Read the response of set brightness command fail
 */
int get_auto_brightness_control_holdtime(int fd, int *holdtime) {
	unsigned char cmd[CMD_LEN]={0xF0, 0x01, 0xE9, 0, 0 };
	int retval = 0;

	cmd[3] += cmd[2];

	retval = write(fd, &cmd, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval: %d.\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Get holdtime command fail. write() retval: %d.\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the holdtime fail. read() retval: %d.\n", retval);
		return -5;
	}

	/* Check the response of brightness setting, the response should be 
	 * 0xF0, 0x02, 0xE9, holdtime_value, checksum */
	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x02 && \
		cmd[2] != 0xE9 && \
	  	cmd[4] != (cmd[2]+cmd[3]) ) {
		dbg_printf("Get holdtime fail. response: %02x, %02x, %02x, %02x, %02x.\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
		return -6;
	}

	*holdtime = (int) cmd[3];

	return 0;
}

/*
 * Set the light sensor and brightness mapping for auto brightness control
 *
 *       X86                             uC
 *     0xF0 0x02 0xE1 0x02 0xE3  --->
 *                               <---  0xF0 0x01 0xE1 0xE1
 * int fd: the device descriptor
 * int lightsensor_level: It's in 0xE1 (Level 1), 0xE2, ..., 0xE8 (Level 8).
 * int br_value: It's in 1, 2, ..., 10.
 * return:
 *      0: Sucess
 *     -2: lightsensor_level is not in 1 ~ 8
 *     -3: br_value is not in 1 ~ 10
 *     -4: Set light sensor and brightness mapping fail
 *     -5: Read the response of set brightness command fail
 *     -6: Set brightness mapping resopnse fail
 */
int set_auto_brightness_level(int fd, int lightsensor_level, int br_value) {

	/* Set light sensor & brightness mapping command is {0xF0, 0x02, 0xE1~0xE8, 1~10, sum(cmd[4]+1) */
	unsigned char cmd[CMD_LEN]={0xF0, 0x02, 0xE1, 0, 0 };
	int retval = 0;

	if( lightsensor_level > MAPPING_NUM || lightsensor_level < 1 ) {
		dbg_printf("The lightsensor_level, %d, is not in 1 ~ %d\n", lightsensor_level, MAX_LIGHTSENSOR_VALUE);
		return -2;
	}
	
	if( br_value > MAX_BRIGHTNESS_VALUE || br_value < (MIN_BRIGHTNESS_VALUE+1) ) {
		dbg_printf("The brightness value, %d, is not in 1  ~ %d\n", br_value, MAX_BRIGHTNESS_VALUE);
		return -3;
	}

	/* The light sensor command is 0xE1, 0xE2,... ,0xE8 */
	cmd[2] = lightsensor_level | 0xE0;
	/* The brightness value is 1, 2, ... ,10 */
	cmd[3] = br_value;
	/* The checksum for command validate in the light control module */
	cmd[4] = cmd[2]+cmd[3];

	retval = write(fd, &cmd, CMD_LEN);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Set brightness command fail. write() retval:%d.\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay (around 150ms) here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response of set brightness command fail. read() retval:%d.\n", retval);
		return -5;
	}

	/* 
	 * Check the response of brightness setting, the response should be 
	 * 0xF0, 0x01, lightsensor_level, brightness value */

	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x01 && \
		cmd[2] != (lightsensor_level|0xE0) && \
		cmd[3] != (lightsensor_level|0xE0) ) {
		dbg_printf("Set light sensor/brightness mapping fail. response: %02x, %02x, %02x, %02x, %02x.\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
		return -6;
	}

	return 0;
}

/*
 * Get the light sensor and brightness mapping for auto brightness control
 *
 *       X86                             uC
 *     0xF0 0x01 0xE1 0xE1  --->
 *                               <---  0xF0 0x02 0xE1 0x02 0xE3
 * int fd: the device descriptor
 * int lightsensor_level: It's in 0xE1 (Level 1), 0xE2 (Level 2), ..., 0xE8 (Level 8).
 * int br_level: It's in 1, 2, ..., 10.
 * return:
 *      0: Sucess
 *     -2: lightsensor_level is not in 1 ~ 8
 *     -4: Set light sensor and brightness mapping fail
 *     -5: Read the response of set brightness command fail
 *     -6: Get light sensor/brightness mapping fail
 */
int get_auto_brightness_level(int fd, int lightsensor_level, int *br_level) {

	/* Set light sensor & brightness mapping command is {0xF0, 0x01, 0xE1~0xE8, sum(cmd[3]) */
	unsigned char cmd[CMD_LEN]={0xF0, 0x01, 0xE1, 0, 0 };
	int retval = 0;

	if( lightsensor_level > MAPPING_NUM || lightsensor_level < 1 ) {
		dbg_printf("The lightsensor_level, %d, is not in 1 ~ %d\n", lightsensor_level, MAX_LIGHTSENSOR_VALUE);
		return -2;
	}

	/* The light sensor command is 0xE1, 0xE2,... ,0xE8 */
	cmd[2] = lightsensor_level | 0xE0;
	/* The checksum for command validate in the light control module */
	cmd[3] = cmd[2];

	retval = write(fd, &cmd, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Set brightness command fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response of set brightness command fail. read() retval:%d\n", retval);
		return -5;
	}

	/* 
	 * Check the response of brightness setting, the response should be 
	 * 0xF0, 0x02, lightsensor_level, brightness_level, checksum */
	if ( cmd[0] != 0xF0 && \
		cmd[1] != 0x02 && \
		cmd[2] != (lightsensor_level|0xE0) ) {
		dbg_printf("Get light sensor/brightness mapping fail. response: %02x, %02x, %02x, %02x, %02x\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
		return -6;
	}

	*br_level = (int) cmd[3];

	return 0;
}

/*
 * Get the firmware version
 *
 *       X86                             uC
 *     0xF0 0x01 0xC7 0xC7  --->
 *                               <---  0xF0 0x02 0xC7 0x12 0xD9
 *     0xF0 0x01 0xCA 0xCA  --->
 *                               <---  0xF0 0x02 0xCA 0x22 0xEC
 *
 *     The firmware version is: 0x1222, V1.2.2 S02
 *
 * int fd: the device descriptor
 * int *frm_ver: The firmware version.
 * return:
 *      0: Sucess
 *     -4: Write the GET FIRMWARE VERSION command fail
 *     -5: Read the response of command fail
 *     -6: Set brightness fail
 */
int get_firmware_version(int fd, int *frm_ver) {
	unsigned char cmd1[CMD_LEN]={0xF0, 0x01, 0xC7, 0, 0 };
	unsigned char cmd2[CMD_LEN]={0xF0, 0x01, 0xCA, 0, 0 };
	int retval = 0;

	cmd1[3] += cmd1[2];

	retval = write(fd, &cmd1, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd1[0], cmd1[1], cmd1[2], cmd1[3], cmd1[4], retval);
	if ( retval < 0 ) {
		dbg_printf("GET FIRMWARE VERSION1 fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd1, 0, sizeof(cmd1));
	retval = read(fd, &cmd1, sizeof(cmd1));
	if ( retval < 0 ) {
		dbg_printf("GET FIRMWARE VERSION1 fail. read() retval:%d\n", retval);
		return -5;
	}

	/* Check the response of brightness setting, the response should be 
	 * 0xF0, 0x02, 0xC7, brightness_value */
	if ( ! ( (cmd1[0] == 0xF0) && (cmd1[1] == 0x02) && (cmd1[2] == 0xC7) ) ) {
		dbg_printf("GET FIRMWARE VERSION1 fail. response: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd1[0], cmd1[1], cmd1[2], cmd1[3], cmd1[4], retval);
		return -6;
	}

	cmd2[3] += cmd2[2];

	retval = write(fd, &cmd2, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd2[0], cmd2[1], cmd2[2], cmd2[3], cmd2[4], retval);
	if ( retval < 0 ) {
		dbg_printf("GET FIRMWARE VERSION2 fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd2, 0, sizeof(cmd2));
	retval = read(fd, &cmd2, sizeof(cmd2));
	if ( retval < 0 ) {
		dbg_printf("GET FIRMWARE VERSION2 fail. read() retval:%d\n", retval);
		return -5;
	}

	/* Check the response of brightness setting, the response should be 
	 * 0xF0, 0x02, 0xCA, brightness_value */
	if ( ! ( (cmd2[0] == 0xF0) && (cmd2[1] == 0x02) && (cmd2[2] == 0xCA) ) ) {
		dbg_printf("GET FIRMWARE VERSION2 fail. response: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd2[0], cmd2[1], cmd2[2], cmd2[3], cmd2[4], retval);
		return -6;
	}

	*frm_ver = ((int) cmd1[3]) << 8 | (int) cmd2[3];

	return 0;
}

/*
 * Start firmware upgrade
 *
 *       X86                             uC
 *     0xF0 0x02 0xCD 0x01 0xCE  --->
 *                       <---  0xF0 0x01 0xCD 0xCD
 *
 * int fd: the device descriptor
 * return:
 *      0: Sucess
 *     -4: Write the GET FIRMWARE VERSION command fail
 *     -5: Read the response of command fail
 *     -6: Set brightness fail
 */
int start_firmware_upgrade(int fd) {
	unsigned char cmd[CMD_LEN]={0xF0, 0x02, 0xCD, 0x01, 0x00 };
	int retval = 0;

	cmd[4] = cmd[2] + cmd[3];

	retval = write(fd, &cmd, 5);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("GET FIRMWARE UPGRADE fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the set brightness command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read FIRMWARE UPGRADE response fail. read() retval:%d\n", retval);
		return -5;
	}

	/* Check the response of brightness setting, the response should be 
	 * 0xF0, 0x01, 0xCD, 0xCD */
	if ( ! ( (cmd[0] == 0xF0) && (cmd[1] == 0x01) && (cmd[2] == 0xCD) && (cmd[3] == 0xCD) ) ) {
		dbg_printf("Read FIRMWARE UPGRADE response fail. retval:%d response: %02x, %02x, %02x, %02x\n", retval, cmd[0], cmd[1], cmd[2], cmd[3]);
		return -6;
	}

	return 0;
}

/*
 * Get the brightness value if brightness key were pressed.
 *
 *       X86                             uC
 *                                     brightness responds
 *                               <---  0xF0 0x02 0xD0 0x01
 *                                     light sensor responds
 *                               <---  0xF0 0x02 0xE0 0x01
 * int fd: the device descriptor
 * int *value: 
 *   The brightness should be in 0, 1, ..., 10.
 * return:
 *      0: Sucess
 *     -7: Read the brightness value fail
 */
int get_brightness_in_brkey_pressed(int fd, int *value) {
	int retval;
	unsigned char cmd[CMD_LEN];

	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the brightness value fail. read() retval:%d\n", retval);
		return -7;
	}

	if( cmd[0]== 0xF0 && cmd[1] == 0x02 && cmd[2] == 0xD0 ) {
		/* 
		 * Check the response of brightness value by UP/DOWN pressed.
		 * The response should be 0xF0, 0x02, 0xD0, brightness_value.
		 */
		dbg_printf("response: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);

		*value = (int) cmd[3];
	}
	else if( cmd[0]== 0xF0 && cmd[1] == 0x02 && cmd[2] == 0xE0 ) {
		/* 
		 * Check the response of brightness value by UP/DOWN pressed.
		 * The response should be 0xF0, 0x02, 0xD0, brightness_value.
		 */
		dbg_printf("response: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);

		*value = (int) cmd[3];
	}

	return 0;
}

/*
 *       X86                             uC
 * Get brightness system status
 *     0xF0 0x01 0xCE 0xCE --->
 *                         <---  0xF0 0x02 0xCE 0x03 0xE1
 *
 * int fd: the device descriptor
 * int *value: The brightness system status.
 * return:
 *     0: Success
 *    -4: Write the get brightness sysstem status command fail
 *    -5: Read the response command fail
 *    -6: Response command incorrect
 */
int get_brightness_system_status(int fd, int *value) {
	unsigned char cmd[CMD_LEN]={0xF0, 0x01, 0xCE, 0, 0 };
	int retval = 0;

	cmd[3] += cmd[2];

	retval = write(fd, &cmd, 4);
	dbg_printf("write(): %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
	if ( retval < 0 ) {
		dbg_printf("Queary brightness system status command fail. write() retval:%d\n", retval);
		return -4;
	}

	/* Important to wait the response with a small delay here */
	sleep_ms(100);

	/* Get the response of the brightness system status command */
	memset(&cmd, 0, sizeof(cmd));
	retval = read(fd, &cmd, sizeof(cmd));
	if ( retval < 0 ) {
		dbg_printf("Read the response command fail. read() retval:%d\n", retval);
		return -5;
	}

	/* Check the response of brightness setting, the response should be 
	 * 0xF0, 0x02, 0xCE, system status, checksum */

	if( cmd[0]!= 0xF0 && cmd[1] != 0x02 && cmd[2] != 0xCE ) {
		dbg_printf("response: %02x, %02x, %02x, %02x, %02x, retval:%d\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], retval);
		return -6;
	}

	*value = cmd[3];

	return 0;
}

