#ifndef __GPIO__LIB__
#define  __GPIO__LIB__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_DIN_PORT	4
#define MAX_DOUT_PORT	4

int gpio_write(char *gpio_sys_file, char state) {
	int fd;
	int retval = 0;

	fd = open(gpio_sys_file, O_WRONLY);
	if ( fd < 0 ) {
		perror("gpio_write() open fail");
		return fd;
	}

	/* Set GPIO high/low status */
	retval = write(fd, &state, sizeof(state));
	if ( retval < 0 ) {
		perror("gpio_write() write fail");
		return retval;
	}

	close(fd);

	return retval;
}

int gpio_read(char *gpio_sys_file, char *state) {
	int fd;
	int retval = 0;

	fd = open(gpio_sys_file, O_RDONLY);
	if ( fd < 0 ) {
		perror("gpio_read() open fail");
		return fd;
	}

	retval = read(fd, state, sizeof(*state));
	if ( retval < 0 ) {
		perror("gpio_read() read fail");
		return retval;
	}

	close(fd);

	return retval;
}

/*
 * Set the DO port state
 * do_port: 1, 2,... , MAX_DOUT_PORT
 * state: '0' or '1'
 * return: 
 *    0: OK
 *   -1: the input state is not '0' or '1'
 *   -2: The do_port is not in 1, 2, ..., MAX_DOUT_PORT
 */
int set_do_state(int do_port, char state) {
	int retval = 0;
	char gpio_sys_file[40];

	if( ! (state == '0' || state == '1') ) {
		printf("The input state is not '0' or '1'\n");
		return -1;
	}

	if( do_port < 1 || do_port > MAX_DOUT_PORT ) {
		printf("The do_port is not in 1, 2, ..., %d\n", MAX_DOUT_PORT);
		return -2;
	}

	sprintf(gpio_sys_file, "/sys/class/gpio/do%d/value", do_port);

	retval = gpio_write(gpio_sys_file, state);

	return retval;
}

/*
 * Get the DO port state
 * do_port: 1, 2,... , MAX_DOUT_PORT
 * *state: The do value. It's should be '0' or '1'
 * return: 
 *    0: OK
 *   -2: The do_port is not in 1, 2, ..., MAX_DOUT_PORT
 */
int get_do_state(int do_port, char *state) {
	int fd;
	int retval = 0;
	char gpio_sys_file[40];

	if( do_port < 1 || do_port > MAX_DOUT_PORT ) {
		printf("The do_port is not in 1, 2, ..., %d\n", MAX_DOUT_PORT);
		return -2;
	}

	sprintf(gpio_sys_file, "/sys/class/gpio/do%d/value", do_port);

	retval = gpio_read(gpio_sys_file, state);
	
	return retval;
}

/*
 * Get the DI port state
 * do_port: 1, 2,... , MAX_DIN_PORT
 * *state: The di value. It's should be '0' or '1'
 * return: 
 *    0: OK
 *   -2: The di_port is not in 1, 2, ..., MAX_DIN_PORT
 */
int get_di_state(int di_port, char *state) {
	int fd;
	int retval = 0;
	char gpio_sys_file[40];

	if( di_port < 1 || di_port > MAX_DIN_PORT ) {
		printf("The di_port is not in 1, 2, ..., %d\n", MAX_DIN_PORT);
		return -2;
	}

	sprintf(gpio_sys_file, "/sys/class/gpio/di%d/value", di_port);

	retval = gpio_read(gpio_sys_file, state);

	return retval;
}

/*
 * Set the mini PCIE GPIO state
 * state: '0' or '1'
 * return: 
 *    0: OK
 *   -1: the input state is not '0' or '1'
 */
int set_minipcie_state(char state) {
	char gpio_sys_file[40];
	int retval = 0;

	if( ! (state == '0' || state == '1') ) {
		printf("The input state is not '0' or '1'\n");
		return -1;
	}

	sprintf(gpio_sys_file, "/sys/class/gpio/minipcie/value");

	retval = gpio_write(gpio_sys_file, state);

	return retval;
}

/*
 * Get the mini PCIE GPIO state
 * *state: The do value. It's should be '0' or '1'
 * return: 
 *    0: OK
 */
int get_minipcie_state(char *state) {
	char gpio_sys_file[40];

	sprintf(gpio_sys_file, "/sys/class/gpio/minipcie/value");

	return gpio_read(gpio_sys_file, state);

}


#endif /* #define  __GPIO__LIB__ */

