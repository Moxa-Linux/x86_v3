/*
 * Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 *
 * pro_mon_fun.c
 *
 * Moxa Proactive Monitoring.
 *
 * 2016-08-01 Holsety Chen
 *   1. fixed some coding style
 *   2. remove mx_pro_mon releated functions
 *
 * 2015-08-05 Wes Huang
 *   new release
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "ini.h"
#include "pro_mon.h"

int g_alarm_occur = FALSE;

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

collect_t collect;

/* ini.parser's handle function */
static int handler(void* user, const char* section, const char* name,
	const char* value)
{
	#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	int i;
	int j;
	int k;
	char setting_bar_item_name[128];
	char alarm_value[128];
	char status_item_name[128];
	char alarm_item_name[128];
	int num_of_items;
	int num_of_alarm;
	int num_of_sub_items;
	char alarm_category[128];
	char tmp[128];

	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS - 1; i++) {
		strcpy(setting_bar_item_name, setting_bar_item[i].name);
		get_num_of_items(i, &num_of_items, &info);
		for(j = 0; j < num_of_items; j++) {
			get_status(i, j, NAME, status_item_name, collect);
			if (MATCH(setting_bar_item_name, status_item_name))
				set_status(i, j, STATUS, atoi(value), collect);
			if (0 != strcmp(status_item_name, "Scan Interval"))
				continue;

			sprintf(tmp, "%s(value)", status_item_name );
			if (MATCH(setting_bar_item_name, tmp)) {
				set_status(i, j, VALUE, atoi(value), collect);
				if (atoi(value) > 0 && atoi(value) <= 3600)
					collect.setting_status[0].value = atoi(value);
				else
					collect.setting_status[0].value = 10;
			}
		}

		get_num_of_alarm(i, &num_of_alarm, &info);
		for (j = 0; j < num_of_alarm; j ++) {
			get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items, collect);
			for (k = 0; k < num_of_sub_items; k++) {
				get_alarm(i, j, k, NAME, alarm_item_name, collect);
				if (k == 0)
					strcpy(alarm_category, alarm_item_name);
				if (MATCH(alarm_category, alarm_item_name)) {
					set_alarm(i, j, k, STATUS, atoi(value), collect);
				}
				sprintf(alarm_value, "%s(value)", alarm_item_name);
				if (MATCH(alarm_category, alarm_value))
					set_alarm(i, j, k, VALUE, atoi(value), collect);
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

/* Create a pthread to polling update */
void *update_value()
{
	int i;
	int count;

	while (1) {
		for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS - 1; i ++)
			update_hardware_info(i, collect, info);
		for (count = 0; count < collect.setting_status[0].value; count++) {
			sleep(1);
			if (count >= collect.setting_status[0].value)
				break;
		}
	}
	pthread_exit(0);
}

void create_pid_file()
{
	FILE *write;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	int ret = 0;
	int ret2 = 0;
	char file_path[BUF_SIZE];
	pid_t pid;
	memset(cmd, '\0', sizeof(cmd));

	/* Check pid file */
	sprintf(cmd, "ls /var/run/ | grep %s", DAEMON_PID);
	ret = exec_cmd(cmd, result);
	if (ret == 0) {
		sprintf(cmd, "cat /var/run/%s", DAEMON_PID);
		ret2 = exec_cmd(cmd, result);
		if (ret2 == 0) {
			pid = atoi(result);
		}

		sprintf(cmd, "ps aux|grep -v grep|grep %d|grep %s", pid, DAEMON);
		ret2 = exec_cmd(cmd, result);
		if (ret2 < 0)
			exit(ret);

		/* The process is running or it's dead */
		if (ret2 == 0) {
			exit(0);
		} else {
			sprintf(cmd, "rm -f /var/run/%s", DAEMON_PID);
			exec_cmd(cmd, result);
		}
	}

	pid = getpid();
	sprintf(file_path, "/var/run/%s", DAEMON_PID);
	write = fopen(file_path, "w");
	if ( write == NULL) {
		exit(1);
		return;
	}

	fprintf(write, "%d", pid);
	fclose(write);
}

int write_edit_log(int type)
{
	FILE *write = create_log_fd();
	char date[BUF_SIZE];

	exec_cmd("printf ""$(date)""", date);

	switch (type) {
	case 0:
		fprintf(write, "%s : Moxa Proactive Monitoring's setting was changed.\n", date);
	break;
	case 1:
		fprintf(write, "%s : Stop Alarm Action.\n", date);
		break;
	}

	fclose(write);
	return 0;
}

void sigroutine(int sig)
{
	FILE *read;
	char cmd[128];
	char result[128];

	switch (sig) {
	case SIGINT:
	case SIGQUIT:
	case SIGKILL:
		sprintf(cmd, "rm -f /var/run/%s", DAEMON_PID);
	        exec_cmd(cmd, result);
		clear_status_alarm(collect);
		exit(1);
		break;
	case SIGUSR1:
		read_config();
		write_edit_log(0);
		break;
	case SIGUSR2:
		write_edit_log(1);
		break;
	default:
		break;
	}
}

int main()
{
	int num_of_alarm, num_of_sub_items;
	int status, value;
	int i, j, k, count;
	char command[128];
	char result[128];

	signal(SIGKILL, sigroutine);
	signal(SIGINT, sigroutine);
	signal(SIGQUIT, sigroutine);
	signal(SIGUSR1, sigroutine);
	signal(SIGUSR2, sigroutine);
	create_pid_file();
	init_info(&info);
	init_collect(&collect, info);
	init_alarm(collect, info);
	init_item(collect, info, main_bar_item, setting_bar_item);
	read_config();
	pthread_t update_thread;
	pthread_create(&update_thread, NULL, update_value, NULL);
	while (1) {
		for (count = 0; count < collect.setting_status[0].value; count++) {
			sleep(1);
			if (count >= collect.setting_status[0].value)
				break;
		}
		check_alarm(collect, info, DAEMON_MODE);
	}

	sprintf(command, "rm -f /var/run/%s", DAEMON_PID);
	exec_cmd(command, result);
	clear_status_alarm(collect);

	return 0;
}
