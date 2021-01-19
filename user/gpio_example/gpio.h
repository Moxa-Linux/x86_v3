#ifndef __GPIO__LIB__
#define  __GPIO__LIB__

/*
 * Note:
 *
 * The MAX_DIN_PORT and MAX_DOUT_PORT definition is dependent on the platform.
 * If the platform has 4 DI/2 DO, you should define the macro as
 *
 * #define MAX_DIN_PORT 4
 * #define MAX_DOUT_PORT 2
 *
 * If the platform has 4 DI/4 DO, you should define the macro as
 *
 * #define MAX_DIN_PORT 4
 * #define MAX_DOUT_PORT 4
 *
 */
#define MAX_DIN_PORT	4
#define MAX_DOUT_PORT	2

/*
 * Set the DO port state
 * do_port: 1, 2,... , MAX_DOUT_PORT
 * state: '0' or '1'
 * return: 
 *    0: OK
 *   -1: the input state is not '0' or '1'
 *   -2: The do_port is not in 1, 2, ..., MAX_DOUT_PORT
 */
int set_do_state(int do_port, char state);

/*
 * Get the DO port state
 * do_port: 1, 2,... , MAX_DOUT_PORT
 * *state: The do value. It's should be '0' or '1'
 * return: 
 *    0: OK
 *   -2: The do_port is not in 1, 2, ..., MAX_DOUT_PORT
 */
int get_do_state(int do_port, char *state);

/*
 * Get the DI port state
 * di_port: 1, 2,... , MAX_DIN_PORT
 * *state: The di value. It's should be '0' or '1'
 * return: 
 *    0: OK
 *   -2: The di_port is not in 1, 2, ..., MAX_DIN_PORT
 */
int get_di_state(int di_port, char *state);

/*
 * Set the mini PCIE GPIO state
 * state: '0' or '1'
 * return: 
 *    0: OK
 *   -1: the input state is not '0' or '1'
 */
int set_minipcie_state(char state);

/*
 * Get the mini PCIE GPIO state
 * *state: The do value. It's should be '0' or '1'
 * return: 
 *    0: OK
 */
int get_minipcie_state(char *state);


#endif /* #define  __GPIO__LIB__ */

