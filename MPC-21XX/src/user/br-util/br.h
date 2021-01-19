#ifndef __BRIGHTNESS_CONTROL__
#define __BRIGHTNESS_CONTROL__

#define CMD_LEN 5
#define SELECT_TIMEOUT 1
#define MAPPING_NUM 8
#define MIN_BRIGHTNESS_VALUE 0
#define MAX_BRIGHTNESS_VALUE 10
#define MIN_HOLDTIME_VALUE 1
#define MAX_HOLDTIME_VALUE 30
#define MIN_LIGHTSENSOR_VALUE 1
#define MAX_LIGHTSENSOR_VALUE 10

/* bit 0 - 0: panel power-off; 1: panel power-on */
#define SYSTEM_STATUS_PANEL_POWER_ON 0x01 
/* bit 1 - 0: light sensor error; 1: light sensor OK */
#define SYSTEM_STATUS_LIGHT_SENSOR_OK 0x02
/* bit 2 - 0: No display output; 1: display output OK */
#define SYSTEM_STATUS_DISPLAY_OUTPUT_OK 0x04

#define BRIGHTNESS_DEVICE "/dev/ttyS2"

#ifdef DEBUG
#define dbg_printf(x, args...) printf("%s[%d] "x, __FUNCTION__, __LINE__, ##args)
#else
#define dbg_printf(x, args...)
#endif

void sleep_ms( int milliseconds);

/*
 * Open the brightness control device, /dev/ttyS2, in 9600n81.
 * return: the device descriptor.
 */
int open_brightness_control(void);

/*
 * Close the brightness control device, /dev/ttyS2.
 */
void close_brightness_control(int fd);

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
int get_auto_brightness_control_status(int fd);

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
int set_auto_brightness_control(int fd, int on_off);

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
int get_lightsensor_level(int fd, int *lightsensor_level);

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
int set_brightness(int fd, int br_value);

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
int get_brightness(int fd, int *br_value);

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
int set_auto_brightness_control_holdtime(int fd, int holdtime);

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
int get_auto_brightness_control_holdtime(int fd, int *holdtime);

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
int set_auto_brightness_level(int fd, int lightsensor_level, int br_value);

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
int get_auto_brightness_level(int fd, int lightsensor_level, int *br_level);

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
int get_firmware_version(int fd, int *frm_ver);

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
int start_firmware_upgrade(int fd);

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
int get_brightness_in_brkey_pressed(int fd, int *value);

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
int get_brightness_system_status(int fd, int *value);

#endif //__BRIGHTNESS_CONTROL__
