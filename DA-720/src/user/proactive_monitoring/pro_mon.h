/*
 * Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 *
 * pro_mon.h
 *
 * Moxa Proactive Monitoring.
 *
 * 2016-08-01 Holsety Chen
 *   fixed some coding style
 *
 * 2015-08-05 Wes Huang
 *   new release
 */

#ifndef _PRO_MON_H_
#define _PRO_MON_H_

#define NUM_OF_MAIN_BAR_ITEMS		7
#define NUM_OF_SETTING_BAR_ITEMS	8

#define MAIN_BAR	1
#define SETTING_BAR	2
#define SETTING_MENU	3
#define SAVE_MENU	4
#define EDIT_OK_MENU	5

#define SAVE		1
#define CANCEL		2
#define OK_OPTION	3

#define NUM_OF_CPU_ALARM	2

#define NUM_OF_CPU_ALARM_SUB_ITEMS		6
#define NUM_OF_MEMORY_ALARM_SUB_ITEMS		6
#define NUM_OF_DISK_ALARM_SUB_ITEMS		6
#define NUM_OF_MAINBOARD_ALARM_SUB_ITEMS	6
#define NUM_OF_NETWORK_ALARM_SUB_ITEMS		4

#define STATUS_MODE	99
#define ALARM_MODE	100
#define UI_MODE		101
#define DAEMON_MODE	102

#define BLACK_WHITE	1
#define MAGENTA_WHITE	2
#define CYAN_WHITE	3
#define BLUE_WHITE	4
#define RED_WHITE	5
#define WHITE_BLUE	6
#define MAGENTA_BLUE	7
#define BLACK_BLUE	8
#define RED_BLUE	9
#define WHITE_RED	10
#define BLUE_RED	11
#define BLACK_RED	12
#define WHITE_BLACK	13
#define YELLOW_BLACK	14
#define MAGENTA_BLACK	15
#define BLUE_YELLOW	16
#define BLACK_GREEN	17

#define FALSE	0
#define TRUE	1

#define CPU		0
#define MEMORY		1
#define DISK		2
#define MAINBOARD	3
#define SERIAL		4
#define NETWORK		5
#define SETTING		6
#define INFO		7

#define NAME		1
#define VALUE		2
#define STATUS		3
#define SELECT		4
#define SUB_ITEMS	5
#define TYPE		6
#define ALARM		7
#define SPEED		8
#define USAGE		9
#define AVAIL_SIZE	10
#define TOTAL_SIZE	11
#define DEVICE_NAME	12
#define COUNT		13

#define MAIN		1
#define U_VALUE		2
#define L_VALUE		3
#define SET_ALARM	4
#define LOG		5
#define TIME_PERIOD	6

#define UPPER		10
#define LOWER		11

#define MOVE_UP		1
#define MOVE_DOWN	2
#define MOVE_LEFT	3
#define MOVE_RIGHT	4

#define STATUS_MENU		99
#define ALARM_MENU		100
#define TIME_PERIOD_MENU	101

#define KEY_EDIT_NUM	101
#define ENTER		10
#define ESCAPE		27
#define SPACE		32
#define TAB		9

#undef KEY_DOWN
#define KEY_DOWN 0402
#undef KEY_UP
#define KEY_UP 0403
#undef KEY_LEFT
#define KEY_LEFT 0404
#undef KEY_RIGHT
#define KEY_RIGHT 0405

#define HEIGHT		98
#define WIDTH		98
#define SETTING_HEIGHT	98
#define SETTING_WIDTH	98

#define SAVE		1
#define CANCEL		2
#define OK_OPTION	3
#define INVALID_OVER	4
#define INVALID_BELOW	5
#define INVALID		6

#define EDIT_PERCENT	1
#define EDIT_TEMP	2
#define EDIT_SCAN	3
#define EXIT_OPTION	4
#define SMALL_SIZE	5
#define EDIT_PERIOD	6

#define TEST_ALARM	1
#define CLEAR_ALARM	2
#define SHOW_MESSAGE	3

#define UI_PID		"pro_mon.pid"
#define DAEMON_PID	"pro_mond.pid"
#define LOG_PATH	"/var/log/pro_mon/"
#define CONFIG_FILE	"/etc/pro_mon/pro_mon.ini"
#define DAEMON		"pro_mond"
#define UI		"pro_mon"
#define IMPL_FILE	"/usr/bin/mx_pro_mon_imp"

#define BUF_SIZE	128

typedef struct cpu_status {
	char name[BUF_SIZE];
	double value;
	int status;
	int select;
	int alarm;
} cpu_status_t;

typedef struct info_struct {
	int current_mode;
	int menu_mode;
	int cpu_core_count;
	int eth_status_count;
	int uart_status_count;
	int cpu_status_count;
	int disk_status_count;
	int mainboard_status_count;
	int memory_status_count;
	int setting_status_count;
	int info_status_count;
	int eth_alarm_count;
	int disk_alarm_count;
	int cpu_alarm_count;
	int mainboard_alarm_count;
	int memory_alarm_count;
	char disk_name[BUF_SIZE];
	int max_row_count;
	int max_window_text;
} info_t;

typedef struct dispaly_struct {
	int total_page;
	int current_page;
	int half_x;
	int half_y;
	int mainbar_width;
	int mainbar_height;
	int mainbar_item_width;
	int mainbar_x;
	int mainbar_y;
	int window_height;
	int window_width;
	int setting_page_height;
	int setting_page_width;
	int settingbar_item_width;
	int setting_page_x;
	int setting_page_y;
	int setting_menu_height;
	int setting_menu_width;
	int setting_menu_x;
	int setting_menu_y;
	int save_menu_height;
	int save_menu_width;
	int save_menu_x;
	int save_menu_y;
	int page_flag_x;
	int page_flag_y;
	char *title;
	char *setting_page_title;
	int title_x;
	int title_y;
	int title_len;
	int setting_page_title_len;
	int eth_count;
	int uart_count;
	int cpu_count;
	int scan_interval;
	int small_width;
	int small_height;
} display_t;

typedef struct memory_status {
	char name[BUF_SIZE];
	double value;
	int status;
	int select;
	int alarm;
} memory_status_t;

typedef struct disk_status {
	char name[BUF_SIZE];
	char device_name[BUF_SIZE];
	int value;
	int avail_size;
	int total_size;
	int status;
	int select;
	int alarm;
} disk_status_t;

typedef struct mainboard_status {
	char name[BUF_SIZE];
	double value;
	int status;
	int select;
	int alarm;
} mainboard_status_t;

typedef struct serial_status {
	double value;
	char name[BUF_SIZE];
	int status;
	int select;
	int alarm;
} serial_status_t;

typedef struct network_status {
	char name[BUF_SIZE];
	double value;
	int speed;
	int usage;
	int status;
	int select;
	int alarm;
} network_status_t;

typedef struct setting_status {
	char name[BUF_SIZE];
	double value;
	int select;
} setting_status_t;


typedef struct info_status {
	char name[BUF_SIZE];
	char content[BUF_SIZE];
} info_status_t;

typedef struct cpu_alarm {
	char name[NUM_OF_CPU_ALARM_SUB_ITEMS][BUF_SIZE];
	int value[NUM_OF_CPU_ALARM_SUB_ITEMS];
	int status[NUM_OF_CPU_ALARM_SUB_ITEMS];
	int select[NUM_OF_CPU_ALARM_SUB_ITEMS];
	int type[NUM_OF_CPU_ALARM_SUB_ITEMS];
	int count[NUM_OF_CPU_ALARM_SUB_ITEMS];
	int num_of_sub_items;
} cpu_alarm_t;

typedef struct memory_alarm {
	char name[NUM_OF_MEMORY_ALARM_SUB_ITEMS][BUF_SIZE];
	int value[NUM_OF_MEMORY_ALARM_SUB_ITEMS];
	int status[NUM_OF_MEMORY_ALARM_SUB_ITEMS];
	int select[NUM_OF_MEMORY_ALARM_SUB_ITEMS];
	int type[NUM_OF_MEMORY_ALARM_SUB_ITEMS];
	int count[NUM_OF_MEMORY_ALARM_SUB_ITEMS];
	int num_of_sub_items;
} memory_alarm_t;

typedef struct disk_alarm {
	char name[NUM_OF_DISK_ALARM_SUB_ITEMS][BUF_SIZE];
	int value[NUM_OF_DISK_ALARM_SUB_ITEMS];
	int status[NUM_OF_DISK_ALARM_SUB_ITEMS];
	int select[NUM_OF_DISK_ALARM_SUB_ITEMS];
	int type[NUM_OF_DISK_ALARM_SUB_ITEMS];
	int count[NUM_OF_DISK_ALARM_SUB_ITEMS];
	int num_of_sub_items;
} disk_alarm_t;


typedef struct mainboard_alarm {
	char name[NUM_OF_MAINBOARD_ALARM_SUB_ITEMS][BUF_SIZE];
	int value[NUM_OF_MAINBOARD_ALARM_SUB_ITEMS];
	int status[NUM_OF_MAINBOARD_ALARM_SUB_ITEMS];
	int select[NUM_OF_MAINBOARD_ALARM_SUB_ITEMS];
	int type[NUM_OF_MAINBOARD_ALARM_SUB_ITEMS];
	int count[NUM_OF_MAINBOARD_ALARM_SUB_ITEMS];
	int num_of_sub_items;
} mainboard_alarm_t;


typedef struct network_alarm {
	char name[NUM_OF_NETWORK_ALARM_SUB_ITEMS][BUF_SIZE];
	int value[NUM_OF_NETWORK_ALARM_SUB_ITEMS];
	int status[NUM_OF_NETWORK_ALARM_SUB_ITEMS];
	int select[NUM_OF_NETWORK_ALARM_SUB_ITEMS];
	int type[NUM_OF_NETWORK_ALARM_SUB_ITEMS];
	int count[NUM_OF_NETWORK_ALARM_SUB_ITEMS];
	int num_of_sub_items;
} network_alarm_t;

typedef struct main_bar_struct {
	char *name;
	int select;
	int status;
	int display;
} main_bar_t;


typedef struct setting_bar_struct {
	char *name;
	int select;
	int status;
} setting_bar_t;

typedef struct collect_struct {
	cpu_status_t *cpu_status;
	memory_status_t *memory_status;
	disk_status_t *disk_status;
	mainboard_status_t *mainboard_status;
	serial_status_t *serial_status;
	network_status_t *network_status;
	setting_status_t *setting_status;
	info_status_t *info_status;

	cpu_alarm_t *cpu_alarm;
	memory_alarm_t *memory_alarm;
	disk_alarm_t *disk_alarm;
	mainboard_alarm_t *mainboard_alarm;
	network_alarm_t *network_alarm;
} collect_t;

void get_num_of_alarm(int, int *, info_t *);

void get_num_of_items(int, int *, info_t *);

void set_alarm(int, int, int, int, int, collect_t);

void set_status(int, int, int, double, collect_t);

void get_alarm(int, int, int, int, void *, collect_t);

void get_status(int, int, int, void *, collect_t);

void init_info(info_t *);

void init_alarm(collect_t, info_t);

void init_collect(collect_t *, info_t);

void clear_status_alarm(collect_t);

void init_item(collect_t, info_t, main_bar_t *, setting_bar_t *);

void update_network_value(collect_t, info_t);

void update_serial_value(collect_t, info_t);

void update_mainboard_value(collect_t, info_t);

void update_disk_value(collect_t, info_t);

void update_mem_value(collect_t, info_t);

void update_cpu_value(collect_t, info_t);

int check_alarm(collect_t, info_t, int);

void update_hardware_info(int, collect_t, info_t);

int exec_cmd(char *, char *);

FILE *create_log_fd();

#endif // _PRO_MON_H_
