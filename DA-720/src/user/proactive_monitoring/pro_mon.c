/*
 * Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 *
 * pro_mon.c
 *
 * Moxa Proactive Monitoring.
 *
 * 2016-08-01 Holsety Chen
 *   fixed some coding style
 *
 * 2015-08-05 Wes Huang
 *   new release
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#include "ini.h"
#include "pro_mon.h"


/* Global variables*/
WINDOW *main_bar, *a, *b, *c, *d, *setting_page, *setting_menu, *save_menu;
pthread_spinlock_t spinlock;

info_t info;

main_bar_t main_bar_item[NUM_OF_MAIN_BAR_ITEMS] = {
	[0].name = "CPU",
	[1].name = "Memory",
	[2].name = "Disk",
	[3].name = "Mainboard",
	[4].name = "Serial",
	[5].name = "Network",
	[6].name = "Setting"
};

setting_bar_t setting_bar_item[NUM_OF_SETTING_BAR_ITEMS] = {
	[0].name = "CPU",
	[1].name = "Memory",
	[2].name = "Disk",
	[3].name = "Mainboard",
	[4].name = "Serial",
	[5].name = "Network",
	[6].name = "Setting",
	[7].name = "Information"
};

display_t display = {
	.title = "[ Moxa Proactive Monitoring ]",
	.setting_page_title = "[ Moxa Proactive Monitoring Setting Page ]"
};

collect_t collect;

/* ini.parser's handle function */
static int handler(void* user, const char* section, const char* name,
	const char* value)
{
	#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	int i;
	int j;
	int k;
	char setting_bar_item_name[BUF_SIZE];
	char alarm_value[BUF_SIZE];
	char status_item_name[BUF_SIZE];
	char alarm_item_name[BUF_SIZE];
	int num_of_items;
	int num_of_alarm;
	int num_of_sub_items;
	char alarm_category[BUF_SIZE];
	char tmp[BUF_SIZE];

	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS - 1; i++) {
		strcpy(setting_bar_item_name, setting_bar_item[i].name);
		get_num_of_items(i, &num_of_items, &info);
		for(j = 0; j < num_of_items; j++) {
			get_status(i, j, NAME, status_item_name, collect);
			if (MATCH(setting_bar_item_name, status_item_name))
				set_status(i, j, STATUS, atoi(value), collect);

			if (strcmp(status_item_name, "Scan Interval")!=0)
				continue;

			sprintf(tmp, "%s(value)", status_item_name );
			if (MATCH(setting_bar_item_name, tmp)) {
				int v = atoi(value);
				if (v > 0 && v <= 3600) {
					collect.setting_status[0].value = v;
				} else {
					collect.setting_status[0].value = 10;
				}
			}
		}

		get_num_of_alarm(i, &num_of_alarm, &info);
		for (j = 0; j < num_of_alarm; j ++) {
			get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items,
				collect);
			for (k = 0; k < num_of_sub_items; k++) {
				get_alarm(i, j, k, NAME, alarm_item_name,
					collect);
				if (k == 0)
					strcpy(alarm_category, alarm_item_name);
				if (MATCH(alarm_category, alarm_item_name))
				{
					set_alarm(i, j, k, STATUS, atoi(value),
						collect);
				}
				sprintf(alarm_value, "%s(value)",
					alarm_item_name);
				if (MATCH(alarm_category, alarm_value))
				{
					int v = atoi(value);

					if (strcmp(alarm_item_name, "Grace Period")==0)
					{
						if (v > 0 && v <= 9999) {
							collect.setting_status[0].value = v;
						} else {
							collect.setting_status[0].value = 1;
							v=1;
						}
					}

					set_alarm(i, j, k, VALUE, v,
						collect);
				}
			}
		}
		
		if (MATCH("Display", setting_bar_item_name))
			main_bar_item[i].status = atoi(value);
	}

	return 0;  /* unknown section/name, error */
}

/* Using ini.parser to read config file */
void read_config()
{
	if (ini_parse(CONFIG_FILE, handler, NULL) < 0) {
		// do nothing
	}	
}
/* Write down all the config information*/
int write_config()
{
	int i, j, k;
	int num_of_items, num_of_alarm;
	int num_of_sub_items, status;
	int value;
	double status_value;
	char name[BUF_SIZE];

	FILE *file = fopen(CONFIG_FILE, "w");
	if (!file) {
		return 1;
	}

	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS - 1; i++) {
		fprintf(file, "[%s]\n", setting_bar_item[i].name);
		get_num_of_items(i, &num_of_items, &info);
		for(j = 0; j < num_of_items; j++) {
			get_status(i, j, NAME, name, collect);
			get_status(i, j, STATUS, &status, collect);
			fprintf(file, "%s = %d\n", name, status);
			if (!strcmp(name, "Scan Interval")) {
				get_status(i, j, VALUE, &status_value, collect);
				fprintf(file, "%s(value) = %d\n", name, (int)status_value);
			}
		}
		get_num_of_alarm(i, &num_of_alarm, &info);
		for (j = 0; j < num_of_alarm; j ++) {
			get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items, collect);
			for (k = 0; k < num_of_sub_items; k++) {
				get_alarm(i, j, k, NAME, name, collect);
				get_alarm(i, j, k, STATUS, &status, collect);
				get_alarm(i, j, k, VALUE, &value, collect);
				if (k == 0)
					fprintf(file, "[%s]\n", name);
				fprintf(file, "%s = %d\n", name, status);
				if (strstr(name, "Threshold") || strstr(name, "Period"))
					fprintf(file, "%s(value) = %d\n", name, value);
			}
		}
	}

	fprintf(file, "[Display]\n");
	for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS - 1; i++)
		fprintf(file, "%s = %d\n", main_bar_item[i].name, main_bar_item[i].status);
	fclose(file);

	return 0;
}

static void init_display_info()
{
	int max_height;
	int max_width;
	int border_height;
	int border_width;
	int border_x;
	int border_y;

	display.setting_page_title_len = strlen(display.setting_page_title);

	display.total_page = 1;
	display.current_page = 1;

	/* Calculate layout's border as the percentage of max window size */
	getmaxyx(stdscr, max_height, max_width);
	border_height = max_height * HEIGHT / 100;
	border_width = max_width * WIDTH / 100;

	/* Calculat the coordinate */
	border_y = (max_height - border_height) / 2;
	border_x = (max_width - border_width) / 2;
	display.half_x = max_width / 2;
	display.half_y = max_height / 2;
	display.title_x = display.half_x - (strlen(display.title) / 2);
	display.title_y = border_y;

	/* Set mainbar's height as 3 rows and width as border width */
	display.mainbar_height = 3;
	display.mainbar_width = border_width;

	/* If width is odd, adjust it in order to align layout */
	display.mainbar_width += (display.mainbar_width&1);
	display.mainbar_item_width =
		display.mainbar_width / NUM_OF_MAIN_BAR_ITEMS;
	display.mainbar_x = border_x;

	// Let mainbar and title remain 1 row distance
	display.mainbar_y = display.title_y + 2;

	/* Calculate 2*2 info windows size and coordinate */
	display.window_width = display.mainbar_width / 2;
	display.window_height =
		(border_height - (display.mainbar_height + 2)) / 2;

	// <PgUp>[1/2]<PgDn> totally len is 17
	display.page_flag_x = display.mainbar_x + display.mainbar_width - 17;
	display.page_flag_y = display.mainbar_y + display.mainbar_height +
		(display.window_height * 2);

	/* Calculate setting page's size as the percentage of 2*2 windows size */
	display.setting_page_height =
		(display.window_height * 2) * SETTING_HEIGHT / 100;
	display.setting_page_width =
		(display.window_width * 2) * SETTING_WIDTH / 100;
	display.settingbar_item_width =
		display.setting_page_width / NUM_OF_SETTING_BAR_ITEMS;
	display.setting_page_x =
		display.half_x - (display.setting_page_width / 2);
	display.setting_page_y = (display.mainbar_y + display.mainbar_height) +
		(display.window_height - (display.setting_page_height / 2));

	/* Setting Menu */
	// Remain 3 rows for setting bar
	display.setting_menu_height = display.setting_page_height -
		(display.mainbar_height + 3);
	display.setting_menu_width = display.setting_page_width;
	display.setting_menu_x = display.setting_page_x;
	display.setting_menu_y = display.setting_page_y +
		display.mainbar_height + 2;

	display.save_menu_height = 3;
	// Remain 1 column distance in left side and right side.
	display.save_menu_width = display.setting_menu_width - 2;
	display.save_menu_x = display.setting_menu_x + 1;
	display.save_menu_y = display.setting_menu_y +
		display.setting_menu_height - 4;

	display.small_width = FALSE;
	display.small_height = FALSE;
}

static void init_curses()
{
	initscr();
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_MAGENTA, COLOR_WHITE);
	init_pair(3, COLOR_CYAN, COLOR_WHITE);
	init_pair(4, COLOR_BLUE, COLOR_WHITE);
	init_pair(5, COLOR_RED, COLOR_WHITE);
	init_pair(6, COLOR_WHITE, COLOR_BLUE);
	init_pair(7, COLOR_MAGENTA, COLOR_BLUE);
	init_pair(8, COLOR_BLACK, COLOR_BLUE);
	init_pair(9, COLOR_RED, COLOR_BLUE);
	init_pair(10, COLOR_WHITE, COLOR_RED);
	init_pair(11, COLOR_BLUE, COLOR_RED);
	init_pair(12, COLOR_BLACK, COLOR_RED);
	init_pair(13, COLOR_WHITE, COLOR_BLACK);
	init_pair(14, COLOR_YELLOW, COLOR_BLACK);
	init_pair(15, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(16, COLOR_BLUE, COLOR_YELLOW);
	init_pair(17, COLOR_BLACK, COLOR_GREEN);
	curs_set(0);
	noecho();
	intrflush(stdscr, FALSE);
}

/* Set color pattern and print the string */
static void write_string(WINDOW *win, char *str, int color)
{
	wattron(win, COLOR_PAIR(color));
	waddstr(win, str);
	wattroff(win, COLOR_PAIR(color));
}

/* Search all the alarm items and then find out the item that was selected */
static int find_alarm_select(int *i, int *j, int *k)
{
	int x;
	int y;
	int z;
	int find = FALSE;
	int num_of_alarm;
	int num_of_sub_items;
	int select;

	for (x = 0; x < NUM_OF_SETTING_BAR_ITEMS - 2; x++) {
		if (setting_bar_item[x].status == FALSE)
			continue;

		get_num_of_alarm(x, &num_of_alarm, &info);
		for (y = 0; y < num_of_alarm; y++) {
			get_alarm(x, y, 0, SUB_ITEMS, &num_of_sub_items,
				collect);
			for (z = 0; z < num_of_sub_items; z++) {
				get_alarm(x, y, z, SELECT, &select, collect);
				if (select == FALSE)
					continue;

				*i = x;
				*j = y;
				*k = z;
				find = TRUE;
				break;
			}
		}

	}

	return find;
}

/* Search all the status items and then find out the item that was selected */
static int find_status_select(int *i, int *j)
{
	int x;
	int y;
	int find = FALSE;
	int num_of_items;
	int select;

	for (x = 0; x < NUM_OF_SETTING_BAR_ITEMS - 1; x++) {
		if (setting_bar_item[x].status == FALSE)
			continue;

		get_num_of_items(x, &num_of_items, &info);
		for (y = 0; y < num_of_items; y++) {
			get_status(x, y, SELECT, &select, collect);
			if (select == FALSE)
				continue;

			*i = x;
			*j = y;
			find = TRUE;
			break;
		}
	}

	return find;
}

/* Draw the mainbar */
static void draw_main_bar()
{
	int i;
	int len = 0;

	wbkgd(main_bar,COLOR_PAIR(BLUE_WHITE));
	for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS; i++) {
		/*
		 * The string length + "[ON]" length. In order to caculate the
		 * location of central.
		 */
		len = strlen(main_bar_item[i].name) + 4;
		wmove(main_bar, 1,
			(display.mainbar_item_width * i) + 
			(display.mainbar_item_width / 2) - (len / 2));
		if (main_bar_item[i].select == TRUE) {
			int str_color;
			if (!strcmp(main_bar_item[i].name, "Setting") &&
				(main_bar_item[i].status == TRUE))
				str_color = WHITE_RED;
			else
				str_color = WHITE_BLUE;
			write_string(main_bar,
				main_bar_item[i].name, str_color);
			
			if (strcmp(main_bar_item[i].name, "Setting")) {
				if (main_bar_item[i].status == TRUE)
					write_string(main_bar,
						"[ON]", RED_BLUE);
				else
					write_string(main_bar,
						"[OFF]", BLACK_BLUE);
			}
		} else {
			write_string(main_bar,
				main_bar_item[i].name, BLUE_WHITE);
			if (strcmp(main_bar_item[i].name, "Setting")) {
				if (main_bar_item[i].status == TRUE)
					write_string(main_bar,
						"[ON]", RED_WHITE);
				else
					write_string(main_bar,
						"[OFF]", BLACK_WHITE);
			}
		}
	}
	wrefresh(main_bar);
}

/* Draw the setting window's menu */
static void draw_setting_page()
{
	int i;

	werase(setting_page);
	wbkgd(setting_page, COLOR_PAIR(BLUE_WHITE));

	wmove(setting_page, 1, 
		(display.setting_page_width / 2) -
		(display.setting_page_title_len / 2));
	write_string(setting_page, display.setting_page_title, BLUE_YELLOW);

	/* Draw the setting page menu bar */
	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
		int str_color;
		wmove(setting_page, 3, display.settingbar_item_width * i +
			(display.settingbar_item_width / 2) -
			(strlen(setting_bar_item[i].name) / 2) - 2);
		if (setting_bar_item[i].status == TRUE &&
			strcmp(setting_bar_item[i].name, "Information")) {
			str_color = WHITE_RED;
		} else {
			if (setting_bar_item[i].select == TRUE)
				str_color = WHITE_BLUE;
			else
				str_color = BLUE_WHITE;
		}
		write_string(setting_page, setting_bar_item[i].name, str_color);
	}

	touchwin(setting_page);
	wrefresh(setting_page);
}

/* main window's layout */
static void layout()
{
	/* print out the title string and create the menu bar */
	bkgd(COLOR_PAIR(WHITE_BLUE));
	move(display.title_y, display.title_x);
	attrset(COLOR_PAIR(BLUE_YELLOW));
	addstr(display.title);
	refresh();
	main_bar = subwin(stdscr, display.mainbar_height,
		display.mainbar_width, display.mainbar_y, display.mainbar_x);
	draw_main_bar(main_bar);

	/* create four windows to fill the screen */
	a = subwin(stdscr, display.window_height, display.window_width,
		display.mainbar_y + display.mainbar_height, display.mainbar_x);
	b = subwin(stdscr, display.window_height, display.window_width,
		display.mainbar_y + display.mainbar_height,
		display.mainbar_x + display.window_width);
	c = subwin(stdscr, display.window_height, display.window_width,
		display.mainbar_y + display.mainbar_height +
		display.window_height, display.mainbar_x);
	d = subwin(stdscr, display.window_height, display.window_width,
		display.mainbar_y + display.mainbar_height +
		display.window_height,
		display.mainbar_x + display.window_width);

	refresh();
}

/* Control the move action(left, right) */
static void move_left_right(int mode, int type)
{
	int i;
	int j;
	int k;
	int num_of_alarm;
	int num_of_sub_items;
	int pos;

	/* Do the move action in different mode */
	/* Find out the item that was selected and then select the next one item */
	switch (mode) {
	case MAIN_BAR:
		for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS; i++) {
			if (main_bar_item[i].select == FALSE)
				continue;

			main_bar_item[i].select = FALSE;
			if (type == MOVE_RIGHT)
				pos = i + 1;
			else
				pos = i + NUM_OF_MAIN_BAR_ITEMS - 1;
			pos %= NUM_OF_MAIN_BAR_ITEMS;
			main_bar_item[pos].select = TRUE;
			break;
		}
		break;

	case SETTING_BAR:
		for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
			if (setting_bar_item[i].select == FALSE)
				continue;

			setting_bar_item[i].select = FALSE;
			if (type == MOVE_RIGHT)
				pos = i + 1;
			else
				pos = i + NUM_OF_SETTING_BAR_ITEMS - 1;
			pos %= NUM_OF_SETTING_BAR_ITEMS;
			setting_bar_item[pos].select = TRUE;
			break;
		}
		break;

	case SETTING_MENU:
		for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
			if (setting_bar_item[i].status == FALSE)
				continue;

			/* If the current menu is status menu */
			if (info.menu_mode == STATUS_MENU &&
				type == MOVE_RIGHT) {
				get_num_of_alarm(i, &num_of_alarm, &info);
				if (num_of_alarm > 0) {
					/*
					 * Set the first item in alarm menu as
					 * selected.
					 */
					set_alarm(i, 0, 0, SELECT, TRUE,
						collect);
					info.menu_mode = ALARM_MENU;
					if (find_status_select(&i, &j) == TRUE)
						set_status(i, j, SELECT, FALSE,
							collect);
				}
			}

			/* If the current menu is alarm menu */
			if (info.menu_mode == ALARM_MENU && type == MOVE_LEFT) {
				set_status(i, 0, SELECT, TRUE, collect);
				info.menu_mode = STATUS_MENU;
				get_num_of_alarm(i, &num_of_alarm, &info);
				for (j = 0; j < num_of_alarm; j++) {
					get_alarm(i, j, 0, SUB_ITEMS,
						&num_of_sub_items, collect);
					// Clean all the alarm menu status.
					for (k = 0; k < num_of_sub_items; k++) {
						set_alarm(i, j, k, SELECT,
							FALSE, collect);
					}
				}
			}
		}
		break;
	}
}

/* Control the move action(up, down) */
static void move_up_down(int mode, int type)
{
	int i;
	int j;
	int k;
	int num_of_alarm;
	int num_of_items;
	int num_of_sub_items;
	int status;
	int pos;

	if (info.menu_mode == STATUS_MODE) {
		if (find_status_select(&i, &j) == TRUE) {
			set_status(i, j, SELECT, FALSE, collect);
			get_num_of_items(i, &num_of_items, &info);
			if (type == MOVE_DOWN)
				pos = j + 1;
			else
				pos = j + num_of_items - 1;
			pos %= num_of_items;
			set_status(i, pos, SELECT, TRUE, collect);
		}
	} else {
		if (find_alarm_select(&i, &j, &k) == TRUE) {
			set_alarm(i, j, k, SELECT, FALSE, collect);
			if (type == MOVE_DOWN) {
				get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items, collect);
				get_num_of_alarm(i, &num_of_alarm, &info);
				get_alarm(i, j, 0, STATUS, &status, collect);	//Check this alarm item is [enable status] or not.
				if (status == TRUE) {				//It is enable so there are some sub items need to select.
					if ((k + 1) >= num_of_sub_items) {	//Check it is a last sub item or not.
						k = 0;				//next one will be the first one
						if ((j + 1) >= num_of_alarm)	//Check it is a last alarm item or not.
							j = 0;			//next one will be the first one
						else
							j++;
					} else {
						k++;				//go next sub item.
					}
				} else {					//It is disable so move down will go next alarm.
					k = 0;					//next one will be the first one.
					if ((j + 1) >= num_of_alarm)
						j = 0;
					else
						j++;
				}
				set_alarm(i, j, k, SELECT, TRUE, collect);
			} else if (type == MOVE_UP) {
				get_num_of_alarm(i, &num_of_alarm, &info);
				if (j == 0 && k == 0) {				//Check this item is the first alarm & first sub item or not.
					j = num_of_alarm - 1;			//Check the prev alarm is [enable status] or not.
					get_alarm(i, j, 0, STATUS, &status, collect);
					if(status == FALSE) {			//The prev alarm item is [disable status]
						k = 0;				//move up will go the prev alarm's first item.
					} else {				//It is [enable status], so move up will go prev alarm's last sub item.
						get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items, collect);
						k = num_of_sub_items - 1;
					}
				} else if (k == 0) {				//It is first sub item
					j--;
					get_alarm(i, j, 0, STATUS, &status, collect);
					if(status == FALSE) {
						k = 0;
					} else {
						get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items, collect);
						k = num_of_sub_items - 1;
					}
				} else {
					k--;
				}
				set_alarm(i, j, k, SELECT, TRUE, collect);
			}
		}
	}
}

/* Control the move action */
static void move_action(const int key, int mode)
{
	switch (key) {
	case KEY_LEFT:
		move_left_right(mode, MOVE_LEFT);
		break;
	case KEY_RIGHT:
		move_left_right(mode, MOVE_RIGHT);
		break;
	case KEY_DOWN:
		move_up_down(mode, MOVE_DOWN);
		break;
	case KEY_UP:
		move_up_down(mode, MOVE_UP);
	default:
		break;
	}
}

/* The inform window */
static void inform_page(int mode, int show_time, char *message)
{
	int height = 7;
	int width = strlen(message) + 5;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	const char *perform_alarm =
		"sh /sbin/mx_perform_alarm > /dev/null 2>&1 &";
	const char *stop_alarm =
		"sh /sbin/mx_stop_alarm > /dev/null 2>&1 &";

	WINDOW * inform = newwin(height, width, display.half_y - 4,
		display.half_x - width / 2);
	wbkgd(inform, COLOR_PAIR(BLUE_WHITE));
	mvwprintw(inform, 3, 3, message);
	wborder(inform, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
		ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	wrefresh(inform);

	switch (mode) {
	case TEST_ALARM:
		exec_cmd((char *) perform_alarm, result);
		sleep(show_time);
		exec_cmd((char *) stop_alarm, result);
		break;

	case CLEAR_ALARM:
		exec_cmd((char *) stop_alarm, result);
		sprintf(cmd, "ls /var/run/ | grep %s", DAEMON_PID);
		if (exec_cmd(cmd, result) == 0) {
			sprintf(cmd, "cat /var/run/%s", DAEMON_PID);
			if ( exec_cmd(cmd, result) == 0) {
				kill(atoi(result), SIGUSR2);
			}
		}
		sleep(show_time);
		break;

	case SHOW_MESSAGE:
		sleep(show_time);
		break;

	default:
		sleep(show_time);
		break;
	}

	delwin(inform);
}

/* Control the actions when press the enter button */
static void enter_action(int mode)
{
	int i;
	int j;
	int k;
	int num_of_sub_items;
	int status;
	char name[BUF_SIZE];

	/* Trun on/off the item */
	switch (mode) {
	case MAIN_BAR: 
		for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS; i++) {
			if (main_bar_item[i].select == FALSE)
				continue;

			if (main_bar_item[i].status == TRUE)
				main_bar_item[i].status = FALSE;
			else
				main_bar_item[i].status = TRUE;
			break;
		}
		break;

	case SETTING_BAR:
		for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
			if (setting_bar_item[i].select == FALSE)
				continue;

			setting_bar_item[i].status = TRUE;
			set_status(i, 0, SELECT, TRUE, collect);
		}
		break;

	case SETTING_MENU:
		if (find_status_select(&i, &j) == TRUE) {
			get_status(i, j, STATUS, &status, collect);
			if (status == TRUE)
				set_status(i, j, STATUS, FALSE, collect);
			else
				set_status(i, j, STATUS, TRUE, collect);

			/* Do some actions with the specific item */
			get_status(i, j, NAME, name, collect);
			if(strstr(name, "Terminate"))
				inform_page(CLEAR_ALARM, 3,
					"Terminate Alarm Action");
			if(strstr(name, "Test Alarm"))
				inform_page(TEST_ALARM, 3,
					"Test Alarm Action for 3 sec");
		}

		if (find_alarm_select(&i, &j, &k) == TRUE) {
			char alarm_item_name[BUF_SIZE];
			get_alarm(i, j, k, NAME, alarm_item_name, collect);
			if (strstr(alarm_item_name, "Grace Period")) {
				break;
			}
			get_alarm(i, j, k, STATUS, &status, collect);
			if (status == TRUE) {
				set_alarm(i, j, k, STATUS, FALSE, collect);
				if (k == 0) { 
					get_alarm(i, j, k, SUB_ITEMS,
						&num_of_sub_items, collect);
					set_alarm(i, j, num_of_sub_items - 1,
						STATUS, FALSE, collect);
				}
			} else {
				set_alarm(i, j, k, STATUS, TRUE, collect);
				if (k == 0) { 
					get_alarm(i, j, k, SUB_ITEMS,
						&num_of_sub_items, collect);
					set_alarm(i, j, num_of_sub_items - 1,
						STATUS, TRUE, collect);
				}
			}
		}
		break;
	}
}

/* Draw the setting menu's display part */
static void draw_setting_menu_display(int i)
{
	int j;
	int current_x = 0;
	int current_y = 0;
	int status;
	int select;
	char name[BUF_SIZE];
	char str[BUF_SIZE];
	int num_of_items;
	int color1;
	int color2;

	/* Show different string color with different status */
	/* setting menu and info menu with specific display */
	
	if (i == SETTING) {
		strcpy( str, "Proactive Monitoring Related\n");
	} else if (i == INFO){
		strcpy( str, "Information\n");
	} else {
		strcpy( str, "Display\n");
	}
	wattron(setting_menu, COLOR_PAIR(BLACK_GREEN));
	mvwprintw(setting_menu, 1, 1, str);
	wattroff(setting_menu, COLOR_PAIR(BLACK_GREEN));
	
	get_num_of_items(i, &num_of_items, &info);
	for (j = 0; j < num_of_items; j++) {
		get_status(i, j, NAME, name, collect);
		get_status(i, j, STATUS, &status, collect);
		get_status(i, j, SELECT, &select, collect);
		getyx(setting_menu, current_y, current_x);
		wmove(setting_menu, current_y + 1, 1);
		if (select == TRUE && info.current_mode != SAVE_MENU) {
			color1 = WHITE_BLUE;
			color2 = RED_BLUE;
		} else {
			color1 = BLUE_WHITE;
			color2 = RED_WHITE;
		}
		if (i == SETTING) {
			if (j == 0) {
				sprintf(str, "( %d Secs )",
					(int)collect.setting_status[j].value);
			} else if (j == 1) {
				wattron(setting_menu, COLOR_PAIR(BLACK_GREEN));
				getyx(setting_menu, current_y, current_x);
				mvwprintw(setting_menu, current_y + 1, 1,
					"Alarm Related\n");
				wattroff(setting_menu, COLOR_PAIR(BLACK_GREEN));
				getyx(setting_menu, current_y, current_x);
				wmove(setting_menu, current_y + 1, 1);
			} 
			if (j >= 1) {
				strcpy(str, "");
			}
		} else if (i == INFO) {
			strcpy(str, collect.info_status[j].content);
		} else {
			if (status == TRUE) {
				strcpy(str, "[Show]");
			} else {
				strcpy(str, "[Hidden]");
				if (color2 == RED_BLUE)
					color2 = BLACK_BLUE;
				else if (color2 == RED_WHITE)
					color2 = BLACK_WHITE;
			}
		}
		write_string(setting_menu, name, color1);
		write_string(setting_menu, str, color2);
	}
	
	get_status(SETTING, 0, SELECT, &select, collect);
	if (select == TRUE)
		mvwprintw(setting_menu, display.setting_menu_height - 5,
			1, "Press <e> to edit the value.");
}

/* Draw the setting menu's alarm part */
static void draw_setting_menu_alarm(int i)
{
	int j;
	int k;
	int current_x = 0;
	int current_y = 0;
	int num_of_sub_items;
	int num_of_alarm;
	int alarm_start_x;

	// Calculate the alarm menu's location.
	alarm_start_x = display.setting_menu_width / 2;

	get_num_of_alarm(i, &num_of_alarm, &info);
	for (j = 0; j < num_of_alarm; j++) {
		int color1;
		int color2;
		int color3;
		int status;
		int select;
		int type;
		int first_status;
		int value;
		char name[BUF_SIZE];
		char str2[BUF_SIZE];
		char str3[BUF_SIZE];
		char alarm_name[BUF_SIZE];

		get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items, collect);
		for (k = 0; k < num_of_sub_items; k++) {
			if (j == 0 && k == 0) {
				wmove(setting_menu, 1 , alarm_start_x);
				write_string(setting_menu,
					"Alarm\n", BLACK_GREEN);
			}

			/*
			 * If the alarm item is [disable status] ignore to
			 * display sub items.
			 */
			if (k != 0) {
				get_alarm(i, j, 0, STATUS, &first_status,
					collect);
				if (first_status == FALSE)
					continue;
			}

			getyx(setting_menu, current_y, current_x);
			wmove(setting_menu, current_y + 1, alarm_start_x);
			get_alarm(i, j, k, SELECT, &select, collect);
			get_alarm(i, j, k, STATUS, &status, collect);
			get_alarm(i, j, k, NAME, name, collect);
			get_alarm(i, j, 0, NAME, alarm_name, collect);
			get_alarm(i, j, k, TYPE, &type, collect);

			/* Indent sub items */
			if (k != 0)
				wprintw(setting_menu, "  ");

			/* Check user whether press <tab> or not */
			if (select == TRUE && info.current_mode != SAVE_MENU) {
				color1 = WHITE_BLUE;
				color2 = MAGENTA_BLUE;
				color3 = RED_BLUE;
			} else {
				color1 = BLUE_WHITE;
				color2 = MAGENTA_WHITE;
				color3 = RED_WHITE;
			}

			if (strstr(name, "Grace Period")) {
				strcpy(str3, "");
			} else if (status == TRUE) {
				strcpy(str3, "[Enable]");
			} else {
				strcpy(str3, "[Disable]");
				if (color3 == RED_WHITE)
					color3 = BLACK_WHITE;
				if (color3 == RED_BLUE)
					color3 = BLACK_BLUE;
			}
			if (type == U_VALUE || type == L_VALUE) {
				getyx(setting_menu, current_y, current_x);
				if (select == TRUE)
					mvwprintw(setting_menu,
					display.setting_menu_height - 5, 1,
					"Press <e> to edit the value.");
				wmove(setting_menu, current_y, current_x);
				get_alarm(i, j, k, VALUE, &value, collect);
				if (strstr(alarm_name, "Temperature"))
					sprintf(str2, "( %d Degrees C )",
						value);
				if (strstr(alarm_name, "Usage"))
					sprintf(str2, "( %d %% )", value);
			} else if (type == TIME_PERIOD ) {
				getyx(setting_menu, current_y, current_x);
				if (select == TRUE)
					mvwprintw(setting_menu,
					display.setting_menu_height - 5, 1,
					"Press <e> to edit the value.");
				wmove(setting_menu, current_y, current_x);
				get_alarm(i, j, k, VALUE, &value, collect);
				sprintf(str2, "( %d )", value);
			} else {
				strcpy(str2, "");
			}
			write_string(setting_menu, name, color1);
			write_string(setting_menu, str2, color2);
			write_string(setting_menu, str3, color3);
		}
	}
}

/* Draw setting menu */
static void draw_setting_menu()
{
	int i;

	werase(setting_menu);
	wbkgd(setting_menu, COLOR_PAIR(BLUE_WHITE));
	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
		if (setting_bar_item[i].select == FALSE)
			continue;

		draw_setting_menu_display(i);
		draw_setting_menu_alarm(i);
	}
	wborder(setting_menu, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
		ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	touchwin(setting_menu);
	wrefresh(setting_menu);
}

/* Draw save menu */
static void draw_save_menu(int option)
{
	int i;
	int k;
	int color1;
	int color2;

	werase(save_menu);
	wbkgd(save_menu, COLOR_PAIR(BLUE_WHITE));
	wborder(save_menu, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
		ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);

	if (info.current_mode != SAVE_MENU) {
		color1 = BLUE_WHITE;
		color2 = BLUE_WHITE;
	} else if (info.current_mode == SAVE_MENU && option == SAVE){
		color1 = WHITE_BLUE;
		color2 = BLUE_WHITE;
	} else if (info.current_mode == SAVE_MENU && option == CANCEL){
		color1 = BLUE_WHITE;
		color2 = WHITE_BLUE;
	} else {
		color1 = -1;
	}

	if ( color1 != -1 ) {
		wmove(save_menu, 1 , display.save_menu_width / 3 - 3);
		write_string(save_menu, "<Save>", color1);
		wmove(save_menu, 1 , display.save_menu_width / 3 * 2 - 4);
		write_string(save_menu, "<Cancel>", color2);
	}

	touchwin(save_menu);
	wrefresh(save_menu);
}

static void signal_to_daemon()
{
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	pid_t pid;

	sprintf(cmd, "ls /var/run/ | grep %s", DAEMON_PID);
	if (exec_cmd(cmd, result) == 0) {
		sprintf(cmd, "cat /var/run/%s", DAEMON_PID);
		if (exec_cmd(cmd, result) == 0) {
			pid = atoi(result);
			kill(pid, SIGUSR1);
		}
	}
}

static void setting_page_resize()
{
	wresize(setting_page, display.setting_page_height,
		display.setting_page_width);
	mvwin(setting_page, display.setting_page_y, display.setting_page_x);
	werase(setting_page);
}

static void setting_menu_resize()
{
	wresize(setting_menu, display.setting_menu_height,
		display.setting_menu_width);
	mvwin(setting_menu, display.setting_menu_y, display.setting_menu_x);
	werase(setting_menu);
}

static void save_menu_resize()
{
	wresize(save_menu, display.save_menu_height, display.save_menu_width);
	mvwin(save_menu, display.save_menu_y, display.save_menu_x);
	werase(save_menu);
}

/* Update the result information */
static void update_windows(WINDOW *win_ptr)
{
	int i;
	int current_item_num = 0;
	int count = 0;
	int display_on_window = FALSE;

	/* Decide the item's information display on which window */
	for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS; i++) {
		/* If now is at page2, ignore the first 4 items */
		if (display.current_page > 1) {
			if (main_bar_item[i].status == TRUE)
				count++;
			if (count <= (4 * (display.current_page - 1)))
				continue;
		}

		/* Find the the item need to show on this window */
		if (main_bar_item[i].status == TRUE &&
			main_bar_item[i].display == FALSE) {
			main_bar_item[i].display = TRUE;
			current_item_num = i;
			display_on_window = TRUE;
			break;
		} else if (main_bar_item[i].status == FALSE &&
			main_bar_item[i].display == TRUE) {
			main_bar_item[i].display = FALSE;
		}
	}

	wbkgd(win_ptr, COLOR_PAIR(WHITE_BLACK));
	werase(win_ptr);

	/* Write to each window */
	if (display_on_window == TRUE) {
		int status;
		int select;
		int alarm;
		int speed;
		int usage;
		int avail_size;
		int total_size;
		double value;
		int num_of_items;
		char name[BUF_SIZE];

		wattron(win_ptr, COLOR_PAIR(BLACK_GREEN));
		mvwprintw(win_ptr, 1, 1, "%s\n",
			main_bar_item[current_item_num].name);
		wattroff(win_ptr, COLOR_PAIR(BLACK_GREEN));
		get_num_of_items(current_item_num, &num_of_items, &info);
		for (i = 0; i < num_of_items; i++) {
			get_status(current_item_num, i, NAME, name, collect);
			get_status(current_item_num, i, VALUE, &value, collect);
			get_status(current_item_num, i, STATUS, &status,
				collect);
			get_status(current_item_num, i, ALARM, &alarm, collect);

			if (status != TRUE)
				continue;

			if (alarm != FALSE) 
				wattron(win_ptr, COLOR_PAIR(WHITE_RED));

			if (value == -99) {
				wprintw(win_ptr, " %s : Loading...\n", name);
			} else {
				if (current_item_num == DISK) {
					get_status(current_item_num, i,
						AVAIL_SIZE, &avail_size,
						collect);
					get_status(current_item_num, i,
						TOTAL_SIZE, &total_size,
						collect);
					wprintw(win_ptr,
				" %s : Total:%dMB, Available:%dMB, Usage:%.0f%%\n",
						name, total_size, avail_size,
						value);
				}
				if (strstr(name, "Usage"))
					wprintw(win_ptr, " %s : %.2f %%\n",
						name, value);
				if (strstr(name, "Count"))
					wprintw(win_ptr, " %s : %.0f\n",
						name, value);
				if (strstr(name, "Size"))
					wprintw(win_ptr, " %s : %.0f Mbytes\n",
						name, value);
				if (strstr(name, "Voltage"))
					wprintw(win_ptr, " %s : %.3f V\n",
						name, value);
				if (strstr(name, "Temperature"))
					wprintw(win_ptr, " %s : %.0f Degree C\n",
						name, value);
				if (strstr(name, "Serial")) {
					if (value == 0)
						wprintw(win_ptr,
							" %s : Available\n",
							name);
					else
						wprintw(win_ptr,
							" %s : In Use\n",
							name);
				}
				if (strstr(name, "Eth")) {
					get_status(current_item_num, i, SPEED,
						&speed, collect);
					get_status(current_item_num, i, USAGE,
						&usage, collect);
					if (value == 1)
						wprintw(win_ptr,
				" %s : Connected, Speed:%dMb/s, Usage:%d%%\n",
						name, speed, usage);
					else
						wprintw(win_ptr,
							" %s : Disconnectd\n",
							name);
				}
				if (strstr(name, "Power")) {
					if (value == 1)
						wprintw(win_ptr,
							" %s : Activated\n",
							name);
					else
						wprintw(win_ptr,
							" %s : No Activated\n",
							name);
				}
			}

			if (alarm != FALSE) 
				wattroff(win_ptr, COLOR_PAIR(WHITE_RED));
		}
	}

	wborder(win_ptr, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
		ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	touchwin(win_ptr);
	wrefresh(win_ptr);
}

/* Whether needs two pages to show the results */
static void check_page()
{
	int i;
	int count = 0;

	for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS - 1; i++) {
		if (main_bar_item[i].status == TRUE )
			count++;
	}

	if (count > 4) {
		display.total_page = count / 4;
		if (count % 4 > 0)
			display.total_page++;
		if (display.current_page > display.total_page)
			display.current_page--;
	} else {
		display.total_page = 1;
		display.current_page = 1;
	}

	move(display.page_flag_y, display.page_flag_x);
	printw("<PgUp>[%d/%d]<PgDn>",
		display.current_page, display.total_page);
	refresh();
}

static void update_all_windows()
{
	int i;

	check_page();
	update_windows(a);
	update_windows(b);
	update_windows(c);
	update_windows(d);
	for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS - 1; i++)
		main_bar_item[i].display = FALSE;
}

/* Control the action when user resize the window */
static void resize_display(int mode, int resize)
{
	if (resize == TRUE) {
		werase(stdscr);
		init_display_info();
		layout();
	}

	switch (mode) { 
	case MAIN_BAR:
		if (resize == TRUE) {
			/*
			 * if user is in setting mode then stop to update four
			 * info windows
			 */
			pthread_spin_lock(&spinlock);
			update_all_windows();
			pthread_spin_unlock(&spinlock);
		}
		werase(main_bar);
		draw_main_bar(main_bar);
		break;

	case SETTING_BAR:
		if (resize == TRUE) {
			setting_page_resize();
			setting_menu_resize();
			update_all_windows();
			draw_main_bar(main_bar);
		}
		draw_setting_page(setting_page);
		draw_setting_menu(setting_menu);
		break;

	case ALARM_MODE:
	case STATUS_MODE:
	case SETTING_MENU:
		if (resize == TRUE) {
			setting_page_resize();
			setting_menu_resize();
			save_menu_resize();
			update_all_windows();
			draw_main_bar(main_bar);
			draw_setting_page(setting_page);
		}
		draw_setting_menu(setting_menu);
		draw_save_menu(0);
		break;

	case EDIT_OK_MENU:
	case SAVE_MENU:
		if (resize == TRUE) {
			setting_page_resize();
			setting_menu_resize();
			save_menu_resize();
			update_all_windows();
			draw_main_bar(main_bar);
			draw_setting_page(setting_page);
			draw_setting_menu(setting_menu);
		}
		info.current_mode = SAVE_MENU;
		break;
	}
}

static void check_window_size()
{
	int current_max_height;
	int current_max_width;
	int current_half_y;
	int current_half_x;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	char name[BUF_SIZE];
	int num_of_items;
	int str_len = 0;
	int i;

	getmaxyx(stdscr, current_max_height, current_max_width);

	current_half_y = current_max_height / 2;
	current_half_x = current_max_width / 2;

	if (display.half_y != current_half_y ||
		display.half_x != current_half_x) {
		resize_display(info.current_mode, TRUE);
	} else {
		resize_display(info.current_mode, FALSE);
	}

	/* Caculate the smallest height size of the display window */
	if (current_max_height < info.max_row_count + 20)
		display.small_height = TRUE;
	else
		display.small_height = FALSE;

	if ((display.window_height < info.cpu_status_count + 3) &&
		(display.small_height == FALSE))
		display.small_height = TRUE;

	/* Caculate the smallest width size of the display window */
	get_num_of_items(DISK, &num_of_items, &info);
	for (i = 0; i < num_of_items; i++) {
		get_status(DISK, i, NAME, name, collect);
		if (strlen(name) > str_len)
			str_len = strlen(name);	
	}

	if (display.window_width < str_len + 44)
		display.small_width = TRUE;
	else
		display.small_width = FALSE;

	if (display.small_height) {
		if (info.current_mode == MAIN_BAR)
			pthread_spin_lock(&spinlock);
		inform_page(SHOW_MESSAGE, 0,
	"The height of the window's size is too small. Please adjust it.");
		if (info.current_mode == MAIN_BAR)
			pthread_spin_unlock(&spinlock);

	}

	if (display.small_width) {
		if (info.current_mode == MAIN_BAR)
			pthread_spin_lock(&spinlock);
		inform_page(SHOW_MESSAGE, 0,
	"The width of the window's size is too small. Please adjust it.");
		if (info.current_mode == MAIN_BAR)
			pthread_spin_unlock(&spinlock);
	}
}

static int save_menu_mode()
{
	int option = SAVE;
	int is_tab = FALSE;
	int key;

	info.current_mode = SAVE_MENU;
	draw_setting_menu(setting_menu);

	keypad(save_menu, TRUE);
	do {
		int i;

		info.current_mode = SAVE_MENU;
		check_window_size();
		draw_save_menu(option);
		key = getch();
		switch (key) {
		case SPACE:
		case ENTER:
			if (option == SAVE) {
				inform_page(SHOW_MESSAGE, 1, "Saving...\n");
				write_config();
				read_config();
				signal_to_daemon();
			}
			key = ESCAPE;
			break;

		case KEY_LEFT:
		case KEY_RIGHT:
		case KEY_UP:
		case KEY_DOWN:
			if (option == SAVE)
				option = CANCEL;
			else
				option = SAVE;
			draw_save_menu(option);
			break;

		case TAB:
			is_tab = TRUE;
			key = ESCAPE;
			break;

		case ESCAPE:
			break;

		default:
			break;
		}
	} while (key != ESCAPE);

	keypad(setting_menu, TRUE);
	return is_tab;
}

static void draw_check_button(WINDOW *check_button, int current_option)
{
	int i;
	int k;
	int color1;
	int color2;

	werase(check_button);
	wbkgd(check_button, COLOR_PAIR(BLUE_WHITE));
	wborder(check_button, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
		ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);

	if (info.current_mode != EDIT_OK_MENU) {
		color1 = BLUE_WHITE;
		color2 = BLUE_WHITE;
	} else if (info.current_mode == EDIT_OK_MENU &&
			current_option == OK_OPTION){
		color1 = WHITE_BLUE;
		color2 = BLUE_WHITE;
	} else if (info.current_mode == EDIT_OK_MENU &&
			current_option == CANCEL){
		color1 = BLUE_WHITE;
		color2 = WHITE_BLUE;
	} else {
		color1 = -1;
	}
	

	if (color1 != -1) {
		wmove(check_button, 1 , 46 / 3 - 2);
		write_string(check_button, "<OK>", color1);
		wmove(check_button, 1 , 46 / 3 * 2 - 4);
		write_string(check_button, "<Cancel>", color2);
	}

	touchwin(check_button);
	wrefresh(check_button);
}

static int check_input(char *input, int mode)
{
	char *tmp = input;
	int first = 0;

	switch (mode) {
	case EDIT_PERCENT:
	case EDIT_SCAN:
	case EDIT_PERIOD:
	case EDIT_TEMP:
		while (*tmp != '\0') {
			if (isdigit(*tmp)) {
				first++;
			} else if (*tmp == '-') {
				if (first != 0) {
					return FALSE;
				}
			} else {
				return FALSE;
			}
			tmp++;
		}

		/* no any number */
		if (first == 0)
			return FALSE;
		break;

	defalut:
		return FALSE;
		break;
	}

	return TRUE;
}

static int edit_value_mode(int i, int j, int k, int *value, int edit_type)
{
	int key;
	char name[BUF_SIZE];
	int height = 10;
	int width = 50;
	int window_x = display.half_x - width / 2;
	int window_y = display.half_y - height / 2;
	int option = 0;
	int current_option = 0;
	char message[BUF_SIZE];
	char input_message[16];
	int tmp_value;
	int new_value;
	WINDOW *edit_value_page;
	WINDOW *input;
	WINDOW *check_button;

	if (edit_type == ALARM_MENU) {
		get_alarm(i, j, 0, NAME, name, collect);
		if (strstr(name, "Usage")) {
			option = EDIT_PERCENT;
			strcpy(message, "Please, key in a value (0 ~ 99)%");
		} else if (strstr(name, "Temperature")) {
			option = EDIT_TEMP;
			strcpy(message,
				"Please, key in a value (-50 ~ 200)degrees C");
		}
	} else if (edit_type == STATUS_MENU) {
		option = EDIT_SCAN;
		strcpy(message, "Please, key in a value (1 ~ 3600) secs");
	} else if (edit_type == TIME_PERIOD_MENU) {
		option = EDIT_PERIOD;
		strcpy(message, "Please, key in a value (1 ~ 9999)");
	} else if (edit_type == EXIT_OPTION) {
		strcpy(message, "   Exit ?   ");
		option = EXIT_OPTION;
	}

	edit_value_page = newwin(height, width, window_y, window_x);
	wbkgd(edit_value_page,COLOR_PAIR(BLUE_WHITE));
	wborder(edit_value_page, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
		ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	wmove(edit_value_page, 3, width / 2 - strlen(message) / 2);
	write_string(edit_value_page, message, BLUE_WHITE);
	wrefresh(edit_value_page);

	check_button = subwin(edit_value_page, 3, width - 4, window_y + 6,
		window_x + 2);
	wbkgd(check_button, COLOR_PAIR(BLUE_WHITE));
	wrefresh(check_button);
	draw_check_button(check_button, current_option);

	switch (option) {
	case EDIT_PERCENT:
		input = subwin(edit_value_page, 1, 3, window_y + 5,
			window_x + width / 2 - 2);
		break;

	case EDIT_TEMP:
		input = subwin(edit_value_page, 1, 4, window_y + 5,
			window_x + width / 2 - 2);
		break;

	case EDIT_SCAN:
	case EDIT_PERIOD:
		input = subwin(edit_value_page, 1, 5, window_y + 5,
			window_x + width / 2 - 3);
		break;

	case EXIT_OPTION:
		break;
	}

	if (option != EXIT_OPTION) {
		wbkgd(input, COLOR_PAIR(WHITE_BLUE));
		wrefresh(input);
		keypad(input, TRUE);
		curs_set(1);
		echo();
	}

	memset(input_message, '\0', sizeof(input_message));
	switch (option) {
	case EDIT_PERCENT:
		wgetnstr(input, input_message, 2);
		break;

	case EDIT_TEMP:
		wgetnstr(input, input_message, 3);
		break;

	case EDIT_SCAN:
	case EDIT_PERIOD:
		wgetnstr(input, input_message, 4);
		break;

	case EXIT_OPTION:
		break;
	}

	if (option != EXIT_OPTION) {
		noecho();
		curs_set(0);
	}

	if (option == EXIT_OPTION) {
		keypad(edit_value_page, TRUE);
	} else {
		keypad(edit_value_page, TRUE);
	}
	
	current_option = OK_OPTION;
	info.current_mode = EDIT_OK_MENU;
	do {
		draw_check_button(check_button, current_option);
		key = getch();
		switch (key) {
		case SPACE:
		case ENTER:
			if (current_option == OK_OPTION) {
				if (check_input(input_message, option) == TRUE) {
					new_value = atoi(input_message);
					switch (option) {
					case EDIT_PERCENT:
						if(new_value < 100 && new_value >= 0) {
							if ( k == 1 ) {
								get_alarm(i, j, k + 1, VALUE, &tmp_value, collect);
								if (new_value <= tmp_value)
									current_option = INVALID_BELOW;
								else
									*value = new_value;
							} else if (k == 2) {
								get_alarm(i, j, k - 1, VALUE, &tmp_value, collect);
								if (new_value >= tmp_value)
									current_option = INVALID_OVER;
								else
									*value = new_value;
							}
						} else {
							current_option = INVALID;
						}
						break;
					case EDIT_TEMP:
						if(new_value <= 200 && new_value >= -50) {
							if (k == 1 ) {
								get_alarm(i, j, k + 1, VALUE, &tmp_value, collect);
								if (new_value <= tmp_value)
									current_option = INVALID_BELOW;
								else
									*value = new_value;
							} else if (k == 2) {
								get_alarm(i, j, k - 1, VALUE, &tmp_value, collect);
								if (new_value >= tmp_value)
									current_option = INVALID_OVER;
								else
									*value = new_value;
							}
						} else {
							current_option = INVALID;
						}
						break;
					case EDIT_SCAN:
						if(new_value <= 3600 && new_value > 0)
							*value = new_value;
						else
							current_option = INVALID;
						break;
					case EDIT_PERIOD:
						if(new_value <= 9999 && new_value > 0)
							*value = new_value;
						else
							current_option = INVALID;
						break;
					}
				} else {
					current_option = INVALID;
				}
			}

			switch (current_option) {
			case INVALID_OVER:
				inform_page(SHOW_MESSAGE, 3, "The value should be below upper threshold.\n");
				current_option = CANCEL;
				break;
			case INVALID_BELOW:
				inform_page(SHOW_MESSAGE, 3, "The value should be over lower threshold.\n");
				current_option = CANCEL;
				break;
			case INVALID:
				inform_page(SHOW_MESSAGE, 3, "The value is invalid.\n");
				current_option = CANCEL;
				break;
			}
			key = ESCAPE;
			break;

		case KEY_LEFT:
		case KEY_RIGHT:
		case KEY_UP:
		case KEY_DOWN:
			if (current_option == OK_OPTION)
				current_option = CANCEL;
			else
				current_option = OK_OPTION;
			break;
		case ESCAPE:
			if (option == EXIT_OPTION)
				key = SPACE;
			break;
		default:
			break;
		}
	} while (key != ESCAPE);

	if (option == EXIT_OPTION) {
		info.current_mode = MAIN_BAR;
		keypad(stdscr, TRUE);
	} else {
		info.current_mode = SETTING_MENU;
		keypad(setting_menu, TRUE);
	}

	delwin(input);
	delwin(check_button);
	delwin(edit_value_page);
	return current_option;
}

/* Handle the actions when user is in setting menu mode */
static void setting_menu_mode()
{
	int i;
	int j;
	int k;
	int key;
	int select;
	int num_of_items;
	int num_of_alarm;
	int num_of_sub_items;
	int ret;
	int value = 0;
	int type;
	char name[BUF_SIZE];

	save_menu = newwin(display.save_menu_height, display.save_menu_width,
		display.save_menu_y, display.save_menu_x);
	wbkgd(save_menu, COLOR_PAIR(BLUE_WHITE));
	wrefresh(save_menu);
	info.menu_mode = STATUS_MODE;

	do {
		info.current_mode = SETTING_MENU;
		check_window_size();
		keypad(setting_menu, TRUE);
		key = getch();
		keypad(setting_menu, FALSE);
		switch (key) {
		case SPACE:
		case ENTER:
			enter_action(SETTING_MENU);
			break;

		case KEY_EDIT_NUM:
			/* Find out the item thar was selected */
			if (find_alarm_select(&i, &j , &k) == TRUE) {
				get_alarm(i, j, k, TYPE, &type, collect);
				// If the item is threshold item.
				if (type == U_VALUE || type == L_VALUE) {
					// edit the value
					ret = edit_value_mode(i, j, k, &value,
						ALARM_MENU);
					if (ret == OK_OPTION)
						set_alarm(i , j, k, VALUE,
							value, collect);
				} else if (type == TIME_PERIOD) {
					ret = edit_value_mode(i, j, k, &value,
						TIME_PERIOD_MENU);
					if (ret == OK_OPTION)
						set_alarm(i , j, k, VALUE,
							value, collect);
				}
			} else if (find_status_select(&i, &j) == TRUE) {
				get_status(i, j, NAME, name, collect);
				// If the item is scan interval.
				if(!strcmp(name, "Scan Interval")) {
					// edit value
					ret = edit_value_mode(i, j, 0, &value,
						STATUS_MENU);
				}
				if (ret == OK_OPTION)
					set_status(i , j, VALUE, value,
						collect);
			}
			break;

		case KEY_LEFT:
		case KEY_RIGHT:
			move_action(key, SETTING_MENU);
			break;

		case KEY_UP:
		case KEY_DOWN:
			move_action(key, SETTING_MENU);
			break;

		case TAB:
			ret = save_menu_mode();
			if (ret == FALSE)
				key = ESCAPE;
			break;
		}
	} while (key != ESCAPE);

	// Clear all the item status 
	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
		if (setting_bar_item[i].status == TRUE) {
			setting_bar_item[i].status = FALSE;
			get_num_of_items(i, &num_of_items, &info);
			for (j = 0; j < num_of_items; j++) {
				get_status(i, j, SELECT, &select, collect);
				if (select == TRUE) {
					set_status(i, j, SELECT, FALSE,
						collect);
				}
			}
			get_num_of_alarm(i, &num_of_alarm, &info);
			for (j = 0; j < num_of_alarm; j++) {
				get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items,
					collect);
				for (k = 0; k < num_of_sub_items; k++) {
					set_alarm(i, j, k, SELECT, FALSE,
						collect);
				}
			}
		}
	}
	keypad(setting_page, TRUE);
	delwin(save_menu);
}

/* Handle the actions when user is in setting mode */
static void setting_page_mode()
{
	int i;
	int key;

	setting_page = subwin(stdscr, display.setting_page_height,
		display.setting_page_width, display.setting_page_y,
		display.setting_page_x);
	wbkgd(setting_page, COLOR_PAIR(BLUE_WHITE));
	wrefresh(setting_page);

	setting_menu = subwin(setting_page, display.setting_menu_height,
		display.setting_menu_width, display.setting_menu_y,
		display.setting_menu_x);
	wbkgd(setting_menu, COLOR_PAIR(BLUE_WHITE));
	wrefresh(setting_menu);

	keypad(setting_page, TRUE);

	do {
		read_config();
		info.current_mode = SETTING_BAR;
		check_window_size();
		key = getch();
		switch (key) {
		case SPACE:
		case ENTER:
			enter_action(SETTING_BAR);
			draw_setting_menu();
			//If it is info menu, do nothing.
			if (setting_bar_item[NUM_OF_SETTING_BAR_ITEMS - 1].select == TRUE)
				break;
			draw_setting_page(setting_page);
			setting_menu_mode();
			break;

		case KEY_LEFT:
		case KEY_RIGHT:
			move_action(key, SETTING_BAR);
			break;

		default:
			break;
		}
	} while (key != ESCAPE);

	delwin(setting_page);
	delwin(setting_menu);
	keypad(stdscr, TRUE);
	write_config();
}

/* Create a pthread to polling update */
static void *update_value()
{
	int i;
	int count;

	while (1) {
		for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS - 1; i ++)
			update_hardware_info(i, collect, info);
		check_alarm(collect, info, UI_MODE);
		if(!pthread_spin_trylock(&spinlock)) {
			update_all_windows();
			pthread_spin_unlock(&spinlock);
		}

		for (count = 0; count < collect.setting_status[0].value; count++) {
			sleep(1);
			if (count >= collect.setting_status[0].value)
				break;
		}
	}
	pthread_exit(0);
}

/* Check whether the process is running or not */
static void check_process()
{
	FILE *read;
	FILE *nohup;
	FILE *write;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	char pid_file[BUF_SIZE];
	int ret = 0;
	int ret2 = 0;
	pid_t pid;

	/* Check pid file */
	sprintf(cmd, "ls /var/run/ | grep %s", UI_PID);
	ret = exec_cmd(cmd, result);

	/* There is a pid file */
	if (ret == 0) {
		sprintf(cmd, "cat /var/run/%s", UI_PID);
		ret2 = exec_cmd(cmd, result);
		if (ret2 == 0) 
			pid = atoi(result);
	
		sprintf(cmd, "ps aux|grep -v grep|grep %d|grep %s", pid, UI);
		ret2 = exec_cmd(cmd, result);
		
		/* The process is running or it's dead */
		if (ret2 == 0) {
			printf("Moxa Proactive Monitoring is running.\n");
			exit(0);
		} else {
			sprintf(cmd, "rm -f /var/run/%s", UI_PID);
			exec_cmd(cmd, result);
		}
	}

	/* Create a pid file */
	pid = getpid();
	sprintf(pid_file, "/var/run/%s", UI_PID);
	write = fopen(pid_file, "w");
	if (!write) {
		printf("Fail to Create a pid file.");
		exit(1);
	}
	fprintf(write, "%d", pid);
	fclose(write);

	sprintf(cmd, "nohup %s > /dev/null 2>&1 &", DAEMON);
	exec_cmd(cmd, result);
}

static void sigroutine(int sig)
{
	FILE *read;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	switch(sig){
	case SIGINT:
	case SIGQUIT:
	case SIGKILL:
		/* del all the windows and remove pid file */
		pthread_spin_destroy(&spinlock);
		sprintf(cmd, "rm -f /var/run/%s", UI_PID);
		exec_cmd(cmd, result);
		clear_status_alarm(collect);
		delwin(main_bar);
		delwin(a);
		delwin(b);
		delwin(c);
		delwin(d);
		endwin();
		exit(1);
		break;

	case SIGUSR1:
		break;

	case SIGUSR2:
		break;
	}
}

int main(int argc, char* argv[])
{
	int key;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	int ret = 0;

	signal(SIGKILL, sigroutine);
	signal(SIGINT, sigroutine);
	signal(SIGQUIT, sigroutine);
	signal(SIGUSR1, sigroutine);
	signal(SIGUSR2, sigroutine);

	/* Check whether the process is running or not */
	check_process();
	
	/* Initial functions */
	init_curses();
	init_display_info();
	init_info(&info);
	init_collect(&collect, info);
	init_alarm(collect, info);
	init_item(collect, info, main_bar_item, setting_bar_item);
	read_config();
	layout();

	/* create a pthread to update four windows */
	pthread_t update_thread;
	pthread_spin_init(&spinlock, 0);
	pthread_create(&update_thread, NULL, update_value, NULL);

	keypad(stdscr, TRUE);

	do {
		if (key != KEY_LEFT && key != KEY_RIGHT) {
			pthread_spin_lock(&spinlock);
			update_all_windows();
			pthread_spin_unlock(&spinlock);
		}
		info.current_mode = MAIN_BAR;
		check_window_size();
		key = wgetch(stdscr);
		switch (key) {
		case SPACE:
		case ENTER:
			enter_action(MAIN_BAR);
			draw_main_bar(main_bar);
			if (main_bar_item[NUM_OF_MAIN_BAR_ITEMS - 1].status == TRUE) {
				/*
				 * If user is in setting mode then stop to
				 * update four info windows
				 */
				pthread_spin_lock(&spinlock);
				setting_page_mode();
				pthread_spin_unlock(&spinlock);
				main_bar_item[NUM_OF_MAIN_BAR_ITEMS - 1].status = FALSE;
			}
			break;

		case KEY_PPAGE:
			if (display.current_page > 1)
				display.current_page--;
			break;

		case KEY_NPAGE:
			if (display.total_page > display.current_page)
				display.current_page++;	
			break;

		case KEY_LEFT:
		case KEY_RIGHT:
			move_action(key, MAIN_BAR);
			break;

		case ESCAPE:
			/*
			 * If user is in setting mode then stop to update
			 * four info windows
			 */
			pthread_spin_lock(&spinlock);
			ret = edit_value_mode(0, 0, 0, NULL, EXIT_OPTION);
			pthread_spin_unlock(&spinlock);
			if (ret == CANCEL)
				key = SPACE;
			break;

		default:
			break;
		}
	} while (key != ESCAPE);

	write_config();
	pthread_spin_destroy(&spinlock);
	sprintf(cmd, "rm -f /var/run/%s", UI_PID);
	exec_cmd(cmd, result);
	clear_status_alarm(collect);

	delwin(main_bar);
	delwin(a);
	delwin(b);
	delwin(c);
	delwin(d);
	endwin();

	return 0;
}
