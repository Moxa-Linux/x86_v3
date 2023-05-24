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
#include <string.h>
#include <stdlib.h>
#include "ini.h"
//#include "mx_pro_mon.h"
#include "pro_mon.h"

int exec_cmd(char *cmd, char *result)
{
	FILE *fp;
	int ret = 0;

	memset(result, '\0', BUF_SIZE);

	/* Open the command for reading. */
	fp = popen(cmd, "r");

	if (NULL == fp) {
		return -1;
	}

	/* Read the output a line at a time - output it. */
	while (fgets(result, BUF_SIZE - 1, fp) != NULL);

	/* close */
	ret = pclose(fp);

	if ( result[0] == '\0' ) {
		return -1;
	}

	return ret;
}

static void find_max_row_count(info_t *info)
{
	int max = 0;

	if ((info->cpu_alarm_count * NUM_OF_CPU_ALARM_SUB_ITEMS) > max)
		max = info->cpu_alarm_count * NUM_OF_CPU_ALARM_SUB_ITEMS;
	if (info->cpu_status_count > max)
		max = info->cpu_status_count;
	if ((info->disk_alarm_count * NUM_OF_DISK_ALARM_SUB_ITEMS) > max)
		max = info->disk_alarm_count * NUM_OF_DISK_ALARM_SUB_ITEMS;
#if 0 /* FIXME: Workaround to disable "height of the window's size is too small.
       *  Please adjust it." issue.
       */
	if ((info->eth_alarm_count * NUM_OF_NETWORK_ALARM_SUB_ITEMS) > max)
		max = info->eth_alarm_count * NUM_OF_NETWORK_ALARM_SUB_ITEMS;
	if (info->uart_status_count > max)
		max = info->uart_status_count;
#endif

	info->max_row_count = max;
}

void init_info(info_t *info)
{
	int ret1 = 0, ret2 = 0, ret3 = 0;
	int value_i = 0;
	unsigned int value_u = 0;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	int ret = 0;

	/* Initial disk info count */
	strcpy(cmd, "df | awk '{print $1}' |grep -vE 'tmpfs|rootfs|udev|Filesystem' | wc -l");
	info->disk_status_count = 0;
	if (0 == exec_cmd(cmd, result))
		info->disk_status_count = atoi(result);
	info->disk_alarm_count = info->disk_status_count;

	/* Initial mainboard info count */
	info->mainboard_status_count = 0;
	info->mainboard_alarm_count = 0;
	sprintf(cmd, "%s get_temperature 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		info->mainboard_status_count++;
		info->mainboard_alarm_count++;
	}

	sprintf(cmd, "%s get_milli_volt 2", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		info->mainboard_status_count++;
	}

	sprintf(cmd, "%s get_pwr_status 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		info->mainboard_status_count = info->mainboard_status_count + 2;
		info->mainboard_alarm_count = info->mainboard_alarm_count + 2;
	}

	/* Initial memory info count */
	info->memory_status_count = 3;
	sprintf(cmd, "%s get_milli_volt 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		info->memory_status_count++;
	}
	info->memory_alarm_count = 1;

	/* Initial cpu info count */
	sprintf(cmd, "%s get_cpu_count", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		info->cpu_core_count = atoi(result);
	}
	info->cpu_status_count = info->cpu_core_count + 4;
	info->cpu_alarm_count = 2;

	/* Initial eth info count */
	info->eth_status_count = 0;
	sprintf(cmd, "%s get_eth_count", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		info->eth_status_count = atoi(result);
	}
	info->eth_alarm_count = info->eth_status_count;

	/* Initial uart info count */
	info->uart_status_count = 0;
	sprintf(cmd, "%s get_uart_count", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		info->uart_status_count = atoi(result);
	}

	/* Initial mainbar info */
	info->current_mode = MAIN_BAR;

	info->setting_status_count = 3;
	info->info_status_count = 4;

	find_max_row_count(info);
}

static setting_status_t *init_setting_status(int setting_status_count)
{
	setting_status_t *setting_status;

	setting_status = (setting_status_t *)
		malloc(setting_status_count * sizeof(setting_status_t));
	strcpy(setting_status[0].name, "Scan Interval");
	strcpy(setting_status[1].name, "Terminate Alarm Action");
	strcpy(setting_status[2].name, "Test Alarm Action for 3 Secs");
	setting_status[0].value = 10;

	return setting_status;
}

static info_status_t *init_info_status(int info_status_count)
{
	info_status_t *info_status;

	info_status = (info_status_t *)
		malloc(info_status_count * sizeof(info_status_t));
	strcpy(info_status[0].name, "[ Moxa Proactive Monitoring ]:");
	strcpy(info_status[1].name, "[ Operating System ]:");
	strcpy(info_status[2].name, "[ Device Kversion ]:");
	strcpy(info_status[3].name, "[ BIOS Version ]:");
	strcpy(info_status[0].content, "Version ?.? Build YYMMDDHH\n");

	return info_status;
}

cpu_alarm_t *init_cpu_alarm(int cpu_alarm_count)
{
	cpu_alarm_t *cpu_alarm;	

	cpu_alarm = (cpu_alarm_t *)
		malloc(cpu_alarm_count * sizeof(cpu_alarm_t));
	strcpy(cpu_alarm[0].name[0], "CPU Total Usage Alarm");
	strcpy(cpu_alarm[0].name[1], "Upper Threshold");
	strcpy(cpu_alarm[0].name[2], "Lower Threshold");
	strcpy(cpu_alarm[0].name[3], "Perform Alarm Action");
	strcpy(cpu_alarm[0].name[4], "Log Message");
	strcpy(cpu_alarm[0].name[5], "Grace Period");
	cpu_alarm[0].num_of_sub_items = 6;
	strcpy(cpu_alarm[1].name[0], "CPU Temperature Alarm");
	strcpy(cpu_alarm[1].name[1], "Upper Threshold");
	strcpy(cpu_alarm[1].name[2], "Lower Threshold");
	strcpy(cpu_alarm[1].name[3], "Perform Alarm Action");
	strcpy(cpu_alarm[1].name[4], "Log Message");
	strcpy(cpu_alarm[1].name[5], "Grace Period");
	cpu_alarm[1].num_of_sub_items = 6;

	return cpu_alarm;
}

static memory_alarm_t *init_memory_alarm(int memory_alarm_count)
{
	memory_alarm_t *memory_alarm;

	memory_alarm = (memory_alarm_t *)
		malloc(memory_alarm_count * sizeof(memory_alarm_t));
	strcpy(memory_alarm->name[0], "Memory Usage Alarm");
	strcpy(memory_alarm->name[1], "Upper Threshold");
	strcpy(memory_alarm->name[2], "Lower Threshold");
	strcpy(memory_alarm->name[3], "Perform Alarm Action");
	strcpy(memory_alarm->name[4], "Log Message");
	strcpy(memory_alarm->name[5], "Grace Period");
	memory_alarm->num_of_sub_items = 6;

	return memory_alarm;
}

static network_status_t *init_network_status(int eth_status_count)
{
	int i;
	network_status_t *network_status;

	network_status = (network_status_t *)
		malloc(eth_status_count * sizeof(network_status_t));

	for (i = 0; i < eth_status_count; i++) {
		sprintf(network_status[i].name, "Eth%d", i);
		network_status[i].speed = 0;
		network_status[i].usage = 0;
	}

	return network_status;
}

static network_alarm_t *init_network_alarm(int eth_alarm_count)
{
	int i;
	network_alarm_t *network_alarm;

	network_alarm = (network_alarm_t *)
		malloc(eth_alarm_count * sizeof(network_alarm_t));
	for (i = 0; i < eth_alarm_count; i++) {
		sprintf(network_alarm[i].name[0],
			"Enable Eth%d Alarm When it is Disconnected", i);
		sprintf(network_alarm[i].name[1], "Perform Alarm Action");
		sprintf(network_alarm[i].name[2], "Log Message");
		sprintf(network_alarm[i].name[3], "Grace Period");
		network_alarm[i].num_of_sub_items = 4;
	}

	return network_alarm;
}

static serial_status_t *init_serial_status(int serial_status_count)
{
	int i;
	serial_status_t *serial_status;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	serial_status = (serial_status_t *)
		malloc(serial_status_count * sizeof(serial_status_t));
	for (i = 0; i < serial_status_count; i++) {
		sprintf(cmd, "%s get_uart_name %d | awk '{printf $0}'", IMPL_FILE, i);
		if (0 == exec_cmd(cmd, result)) {
			sprintf(serial_status[i].name, "Serial %s", result);
		} else {
			sprintf(serial_status[i].name, "Serial P%d", i + 1);
		}
	}

	return serial_status;
}

static disk_status_t *init_disk_status(int disk_status_count)
{
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	int i, count;
	disk_status_t *disk_status;

	disk_status = (disk_status_t *)
		malloc((disk_status_count) * sizeof(disk_status_t));
	for (i = 0; i < disk_status_count; i++) {
		sprintf(cmd, "printf $(df | awk '{print $1}' |grep -vE 'tmpfs|rootfs|udev|Filesystem' | head -n%d | tail -n1)", i + 1);
		if (0 == exec_cmd(cmd, result)) {
			sprintf(disk_status[i].name, "%s", result);
			sprintf(disk_status[i].device_name, "%s", result);
			disk_status[i].total_size = 0;
			disk_status[i].avail_size = 0;
		}
	}

	return disk_status;
}

static disk_alarm_t *init_disk_alarm(int disk_alarm_count)
{
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	int i;
	int count;
	char *tmp = NULL;
	disk_alarm_t *disk_alarm;

	disk_alarm = (disk_alarm_t *)
		malloc((disk_alarm_count) * sizeof(disk_alarm_t));
	for (i = 0; i < disk_alarm_count; i++) {
		sprintf(cmd, "printf $(df | awk '{print $1}' |grep -vE 'tmpfs|rootfs|udev|Filesystem' | head -n%d | tail -n1)", i + 1);
		if (0 == exec_cmd(cmd, result)) {
			sprintf(disk_alarm[i].name[0], "%s Usage Alarm", result);
			strcpy(disk_alarm[i].name[1], "Upper Threshold");
			strcpy(disk_alarm[i].name[2], "Lower Threshold");
			strcpy(disk_alarm[i].name[3], "Perform Alarm Action");
			strcpy(disk_alarm[i].name[4], "Log Message");
			strcpy(disk_alarm[i].name[5], "Grace Period");
			disk_alarm[i].num_of_sub_items = 6;
		}
	}

	return disk_alarm;
}

static cpu_status_t *init_cpu_status(int cpu_status_count, int cpu_core_count)
{
	int i;
	int count = 0;
	cpu_status_t *cpu_status;

	cpu_status = (cpu_status_t *)
		malloc((cpu_status_count) * sizeof(cpu_status_t));

	sprintf(cpu_status[count++].name, "CPU Total Usage");
	sprintf(cpu_status[count++].name, "CPU Logic Count");
	for (i = 0; i < cpu_core_count; i++) {
		sprintf(cpu_status[count++].name, "CPU Core%d Usage", i);
	}
	sprintf(cpu_status[count++].name, "CPU Temperature");
	sprintf(cpu_status[count].name, "CPU Voltage");

	return cpu_status;
}

static mainboard_status_t *init_mainboard_status(int mainboard_status_count)
{
	int status_count = 0;
	char result[BUF_SIZE];
	char cmd[BUF_SIZE];
	mainboard_status_t *mainboard_status;

	mainboard_status = (mainboard_status_t *)
		malloc((mainboard_status_count) * sizeof(mainboard_status_t));

	sprintf(cmd, "%s get_temperature 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		strcpy(mainboard_status[status_count++].name,
			"Mainboard Temperature");
	}

	sprintf(cmd, "%s get_milli_volt 2", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		strcpy(mainboard_status[status_count++].name,
			"Mainboard Voltage");
	}
	
	sprintf(cmd, "%s get_pwr_status 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		strcpy(mainboard_status[status_count++].name,
			"Mainboard Power1 Indicator");
		strcpy(mainboard_status[status_count].name,
			"Mainboard Power2 Indicator");
	}

	return mainboard_status;
}

static mainboard_alarm_t *init_mainboard_alarm(int mainboard_alarm_count)
{
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	int alarm_count = 0;
	mainboard_alarm_t *mainboard_alarm;

	mainboard_alarm = (mainboard_alarm_t *)
		malloc((mainboard_alarm_count) * sizeof(mainboard_alarm_t));

	sprintf(cmd, "%s get_temperature 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		strcpy(mainboard_alarm[alarm_count].name[0],
			"Mainboard Temperature Alarm");
		strcpy(mainboard_alarm[alarm_count].name[1],
			"Upper Threshold");
		strcpy(mainboard_alarm[alarm_count].name[2],
			"Lower Threshold");
		strcpy(mainboard_alarm[alarm_count].name[3],
			"Perform Alarm Action");
		strcpy(mainboard_alarm[alarm_count].name[4],
			"Log Message");
		strcpy(mainboard_alarm[alarm_count].name[5],
			"Grace Period");
		mainboard_alarm[alarm_count++].num_of_sub_items = 6;
	}

	sprintf(cmd, "%s get_pwr_status 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		strcpy(mainboard_alarm[alarm_count].name[0],
			"Mainboard Power1 Indicator Alarm");
		strcpy(mainboard_alarm[alarm_count].name[1],
			"Perform Alarm Action");
		strcpy(mainboard_alarm[alarm_count].name[2], "Log Message");
		strcpy(mainboard_alarm[alarm_count].name[3], "Grace Period");
		mainboard_alarm[alarm_count++].num_of_sub_items = 4;
		strcpy(mainboard_alarm[alarm_count].name[0],
			"Mainboard Power2 Indicator Alarm");
		strcpy(mainboard_alarm[alarm_count].name[1],
			"Perform Alarm Action");
		strcpy(mainboard_alarm[alarm_count].name[2], "Log Message");
		strcpy(mainboard_alarm[alarm_count].name[3], "Grace Period");
		mainboard_alarm[alarm_count++].num_of_sub_items = 4;
	}

	return mainboard_alarm;
}

static memory_status_t *init_memory_status(int memory_status_count)
{
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	memory_status_t *memory_status;

	memory_status = (memory_status_t *)
		malloc((memory_status_count) * sizeof(memory_status_t));
	strcpy(memory_status[0].name, "Memory Usage");
	strcpy(memory_status[1].name, "Total Memory Size");
	strcpy(memory_status[2].name, "Free Memory Size");

	sprintf(cmd, "%s get_milli_volt 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		strcpy(memory_status[3].name, "Memory Voltage");
	}

	return memory_status;
}

void init_collect(collect_t *collect, info_t info)
{
	collect->cpu_status =
		init_cpu_status(info.cpu_status_count, info.cpu_core_count);
	collect->memory_status = init_memory_status(info.memory_status_count);
	collect->disk_status = init_disk_status(info.disk_status_count);
        collect->mainboard_status =
		init_mainboard_status(info.mainboard_status_count);
	collect->serial_status = init_serial_status(info.uart_status_count);
	collect->network_status = init_network_status(info.eth_alarm_count);
	collect->setting_status =
		init_setting_status(info.setting_status_count);
	collect->info_status = init_info_status(info.info_status_count);

	collect->cpu_alarm = init_cpu_alarm(info.cpu_alarm_count);
	collect->memory_alarm = init_memory_alarm(info.memory_alarm_count);
	collect->disk_alarm = init_disk_alarm(info.disk_alarm_count);
	collect->mainboard_alarm =
		init_mainboard_alarm(info.mainboard_status_count);
	collect->network_alarm = init_network_alarm(info.eth_status_count);
}

void clear_status_alarm(collect_t collect)
{
	free(collect.cpu_status);
	free(collect.memory_status);
	free(collect.disk_status);
	free(collect.mainboard_status);
	free(collect.serial_status);
	free(collect.network_status);
	free(collect.setting_status);
	free(collect.info_status);

	free(collect.cpu_alarm);
	free(collect.memory_alarm);
	free(collect.disk_alarm);
	free(collect.mainboard_alarm);
	free(collect.network_alarm);
}

void get_num_of_alarm(int i, int *num_of_alarm, info_t *info)
{
	switch (i) {
	case CPU:
		*num_of_alarm = info->cpu_alarm_count;
		break;
	case MEMORY:
		*num_of_alarm = info->memory_alarm_count;
		break;
	case DISK:
		*num_of_alarm = info->disk_alarm_count;
		break;
	case MAINBOARD:
		*num_of_alarm = info->mainboard_alarm_count;
		break;
	case NETWORK:
		*num_of_alarm = info->eth_alarm_count;
		break;
	default:
		*num_of_alarm = 0;
		break;
	}
}

void get_num_of_items(int i, int *num_of_items, info_t *info)
{
	switch (i) {
	case CPU:
		*num_of_items = info->cpu_status_count;
		break;
	case MEMORY:
		*num_of_items = info->memory_status_count;
		break;
	case DISK:
		*num_of_items = info->disk_status_count;
		break;
	case MAINBOARD:
		*num_of_items = info->mainboard_status_count;
		break;
	case SERIAL:
		*num_of_items = info->uart_status_count;
		break;
	case NETWORK:
		*num_of_items = info->eth_status_count;
		break;
	case SETTING:
		*num_of_items = info->setting_status_count;
		break;
	case INFO:
		*num_of_items = info->info_status_count;
		break;
	default:
		*num_of_items = 0;
		break;
	}
}

void set_alarm(int i, int j, int k, int type, int value, collect_t collect)
{
	if (i == CPU) {
		if (type == TYPE) {
			collect.cpu_alarm[j].type[k] = value;
		} else if (type == VALUE) {
			collect.cpu_alarm[j].value[k] = value;
		} else if (type == STATUS) {
			collect.cpu_alarm[j].status[k] = value;
		} else if (type == SELECT) {
			collect.cpu_alarm[j].select[k] = value;
		} else if (type == COUNT) {
			collect.cpu_alarm[j].count[k] = value;
		}
	} else if (i == MEMORY) {
		if (type == TYPE) {
			collect.memory_alarm[j].type[k] = value;
		} else if (type == VALUE) {
			collect.memory_alarm[j].value[k] = value;
		} else if (type == STATUS) {
			collect.memory_alarm[j].status[k] = value;
		} else if (type == SELECT) {
			collect.memory_alarm[j].select[k] = value;
		} else if (type == COUNT) {
			collect.memory_alarm[j].count[k] = value;
		}
	} else if (i == DISK) {
		if (type == TYPE) {
			collect.disk_alarm[j].type[k] = value;
		} else if (type == VALUE) {
			collect.disk_alarm[j].value[k] = value;
		} else if (type == STATUS) {
			collect.disk_alarm[j].status[k] = value;
		} else if (type == SELECT) {
			collect.disk_alarm[j].select[k] = value;
		} else if (type == COUNT) {
			collect.disk_alarm[j].count[k] = value;
		}
	} else if (i == MAINBOARD) {
		if (type == TYPE) {
			collect.mainboard_alarm[j].type[k] = value;
		} else if (type == VALUE) {
			collect.mainboard_alarm[j].value[k] = value;
		} else if (type == STATUS) {
			collect.mainboard_alarm[j].status[k] = value;
		} else if (type == SELECT) {
			collect.mainboard_alarm[j].select[k] = value;
		} else if (type == COUNT) {
			collect.mainboard_alarm[j].count[k] = value;
		}
	} else if (i == NETWORK) {
		if (type == TYPE) {
			collect.network_alarm[j].type[k] = value;
		} else if (type == VALUE) {
			collect.network_alarm[j].value[k] = value;
		} else if (type == STATUS) {
			collect.network_alarm[j].status[k] = value;
		} else if (type == SELECT) {
			collect.network_alarm[j].select[k] = value;
		} else if (type == COUNT) {
			collect.network_alarm[j].count[k] = value;
		}
	}
}

void set_status(int i, int j, int type, double value, collect_t collect)
{
	if (i == CPU) {
		if (type == VALUE) {
			collect.cpu_status[j].value = value;
		} else if (type == STATUS) {
			collect.cpu_status[j].status = value;
		} else if (type == SELECT) {
			collect.cpu_status[j].select = value;
		} else if (type == ALARM) {
			collect.cpu_status[j].alarm = value;
		}
	} else if (i == MEMORY) {
		if (type == VALUE) {
			collect.memory_status[j].value = value;
		} else if (type == STATUS) {
			collect.memory_status[j].status = value;
		} else if (type == SELECT) {
			collect.memory_status[j].select = value;
		} else if (type == ALARM) {
			collect.memory_status[j].alarm = value;
		}
	} else if (i == DISK) {
		if (type == VALUE) {
			collect.disk_status[j].value = value;
		} else if (type == STATUS) {
			collect.disk_status[j].status = value;
		} else if (type == SELECT) {
			collect.disk_status[j].select = value;
		} else if (type == ALARM) {
			collect.disk_status[j].alarm = value;
		} else if (type == AVAIL_SIZE) {
			collect.disk_status[j].avail_size = value;
		} else if (type == TOTAL_SIZE) {
			collect.disk_status[j].total_size = value;
		}
	} else if (i == MAINBOARD) {
		if (type == VALUE) {
			collect.mainboard_status[j].value = value;
		} else if (type == STATUS) {
			collect.mainboard_status[j].status = value;
		} else if (type == SELECT) {
			collect.mainboard_status[j].select = value;
		} else if (type == ALARM) {
			collect.mainboard_status[j].alarm = value;
		}
	} else if (i == SERIAL) {
		if (type == VALUE) {
			collect.serial_status[j].value = value;
		} else if (type == STATUS) {
			collect.serial_status[j].status = value;
		} else if (type == SELECT) {
			collect.serial_status[j].select = value;
		} else if (type == ALARM) {
			collect.serial_status[j].alarm = value;
		}
	} else if (i == NETWORK) {
		if (type == VALUE) {
			collect.network_status[j].value = value;
		} else if (type == STATUS) {
			collect.network_status[j].status = value;
		} else if (type == SELECT) {
			collect.network_status[j].select = value;
		} else if (type == ALARM) {
			collect.network_status[j].alarm = value;
		} else if (type == SPEED) {
			collect.network_status[j].speed = value;
		} else if (type == USAGE) {
			collect.network_status[j].usage = value;
		}
	} else if (i == SETTING) {
		if (type == VALUE) {
			collect.setting_status[j].value = value;
		} else if (type == SELECT) {
			collect.setting_status[j].select = value;
		}
	}
}

void get_alarm(int i, int j, int k, int type, void *ptr, collect_t collect)
{
	char *name = (char *)ptr;
	int *value = (int *)ptr;

	if (i == CPU) {
		if (type == TYPE) {
			*value = collect.cpu_alarm[j].type[k];
		} else if (type == NAME) {
			strcpy(name, collect.cpu_alarm[j].name[k]);
		} else if (type == VALUE) {
			*value = collect.cpu_alarm[j].value[k];
		} else if (type == STATUS) {
			*value = collect.cpu_alarm[j].status[k];
		} else if (type == SELECT) {
			*value = collect.cpu_alarm[j].select[k];
		} else if (type == SUB_ITEMS) {
			*value = collect.cpu_alarm[j].num_of_sub_items;
		} else if (type == COUNT) {
			*value = collect.cpu_alarm[j].count[k];
		}
	} else if (i == MEMORY) {
		if (type == TYPE) {
			*value = collect.memory_alarm[j].type[k];
		} else if (type == NAME) {
			strcpy(name, collect.memory_alarm[j].name[k]);
		} else if (type == VALUE) {
			*value = collect.memory_alarm[j].value[k];
		} else if (type == STATUS) {
			*value = collect.memory_alarm[j].status[k];
		} else if (type == SELECT) {
			*value = collect.memory_alarm[j].select[k];
		} else if (type == SUB_ITEMS) {
			*value = collect.memory_alarm[j].num_of_sub_items;
		} else if (type == COUNT) {
			*value = collect.memory_alarm[j].count[k];
		}
	} else if (i == DISK) {
		if (type == TYPE) {
			*value = collect.disk_alarm[j].type[k];
		} else if (type == NAME) {
			strcpy(name, collect.disk_alarm[j].name[k]);
		} else if (type == VALUE) {
			*value = collect.disk_alarm[j].value[k];
		} else if (type == STATUS) {
			*value = collect.disk_alarm[j].status[k];
		} else if (type == SELECT) {
			*value = collect.disk_alarm[j].select[k];
		} else if (type == SUB_ITEMS) {
			*value = collect.disk_alarm[j].num_of_sub_items;
		} else if (type == COUNT) {
			*value = collect.disk_alarm[j].count[k];
		}
	} else if (i == MAINBOARD) {
		if (type == TYPE) {
			*value = collect.mainboard_alarm[j].type[k];
		} else if (type == NAME) {
			strcpy(name, collect.mainboard_alarm[j].name[k]);
		} else if (type == VALUE) {
			*value = collect.mainboard_alarm[j].value[k];
		} else if (type == STATUS) {
			*value = collect.mainboard_alarm[j].status[k];
		} else if (type == SELECT) {
			*value = collect.mainboard_alarm[j].select[k];
		} else if (type == SUB_ITEMS) {
			*value = collect.mainboard_alarm[j].num_of_sub_items;
		} else if (type == COUNT) {
			*value = collect.mainboard_alarm[j].count[k];
		}
	} else if (i == NETWORK) {
		if (type == TYPE) {
			*value = collect.network_alarm[j].type[k];
		} else if (type == NAME) {
			strcpy(name, collect.network_alarm[j].name[k]);
		} else if (type == VALUE) {
			*value = collect.network_alarm[j].value[k];
		} else if (type == STATUS) {
			*value = collect.network_alarm[j].status[k];
		} else if (type == SELECT) {
			*value = collect.network_alarm[j].select[k];
		} else if (type == SUB_ITEMS) {
			*value = collect.network_alarm[j].num_of_sub_items;
		} else if (type == COUNT) {
			*value = collect.network_alarm[j].count[k];
		}
	}
}

void get_status(int i, int j, int type, void *ptr, collect_t collect)
{
	char *name = (char *)ptr;
	int *i_value = (int *)ptr;
	double *d_value = (double *)ptr;

	if (i == CPU) {
		if (type == NAME) {
			strcpy(name, collect.cpu_status[j].name);
		} else if (type == VALUE) {
			*d_value = collect.cpu_status[j].value;
		} else if (type == STATUS) {
			*i_value = collect.cpu_status[j].status;
		} else if (type == SELECT) {
			*i_value = collect.cpu_status[j].select;
		} else if (type == ALARM) {
			*i_value = collect.cpu_status[j].alarm;
		}
	} else if (i == MEMORY) {
		if (type == NAME) {
			strcpy(name, collect.memory_status[j].name);
		} else if (type == VALUE) {
			*d_value = collect.memory_status[j].value;
		} else if (type == STATUS) {
			*i_value = collect.memory_status[j].status;
		} else if (type == SELECT) {
			*i_value = collect.memory_status[j].select;
		} else if (type == ALARM) {
			*i_value = collect.memory_status[j].alarm;
		}
	} else if (i == DISK) {
		if (type == NAME) {
			strcpy(name, collect.disk_status[j].name);
		} else if (type == VALUE) {
			*d_value = collect.disk_status[j].value;
		} else if (type == STATUS) {
			*i_value = collect.disk_status[j].status;
		} else if (type == SELECT) {
			*i_value = collect.disk_status[j].select;
		} else if (type == ALARM) {
			*i_value = collect.disk_status[j].alarm;
		} else if (type == AVAIL_SIZE) {
			*i_value = collect.disk_status[j].avail_size;
		} else if (type == TOTAL_SIZE) {
			*i_value = collect.disk_status[j].total_size;
		} else if (type == DEVICE_NAME) {
			strcpy(name, collect.disk_status[j].device_name);
		}
	} else if (i == MAINBOARD) {
		if (type == NAME) {
			strcpy(name, collect.mainboard_status[j].name);
		} else if (type == VALUE) {
			*d_value = collect.mainboard_status[j].value;
		} else if (type == STATUS) {
			*i_value = collect.mainboard_status[j].status;
		} else if (type == SELECT) {
			*i_value = collect.mainboard_status[j].select;
		} else if (type == ALARM) {
			*i_value = collect.mainboard_status[j].alarm;
		}
	} else if (i == SERIAL) {
		if (type == NAME) {
			strcpy(name, collect.serial_status[j].name);
		} else if (type == VALUE) {
			*d_value = collect.serial_status[j].value;
		} else if (type == STATUS) {
			*i_value = collect.serial_status[j].status;
		} else if (type == SELECT) {
			*i_value = collect.serial_status[j].select;
		} else if (type == ALARM) {
			*i_value = collect.serial_status[j].alarm;
		}
	} else if (i == NETWORK) {
		if (type == NAME) {
			strcpy(name, collect.network_status[j].name);
		} else if (type == VALUE) {
			*d_value = collect.network_status[j].value;
		} else if (type == STATUS) {
			*i_value = collect.network_status[j].status;
		} else if (type == SELECT) {
			*i_value = collect.network_status[j].select;
		} else if (type == ALARM) {
			*i_value = collect.network_status[j].alarm;
		} else if (type == SPEED) {
			*i_value = collect.network_status[j].speed;
		} else if (type == USAGE) {
			*i_value = collect.network_status[j].usage;
		}
	} else if (i == SETTING) {
		if (type == NAME) {
			strcpy(name, collect.setting_status[j].name);
		} else if (type == VALUE) {
			*d_value = collect.setting_status[j].value;
		} else if (type == SELECT) {
			*i_value = collect.setting_status[j].select;
		}
	} else if (i == INFO) {
		if (type == NAME) {
			strcpy(name, collect.info_status[j].name);
		}
	}
}

void init_alarm(collect_t collect, info_t info)
{
	int i;
	int j;
	int k;
	int num_of_alarm;
	int num_of_sub_items;
	char name[BUF_SIZE];

	for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS - 1; i++) {
		get_num_of_alarm(i, &num_of_alarm, &info);
		for(j = 0; j < num_of_alarm; j++) {
			get_alarm(i, j, 0, SUB_ITEMS, &num_of_sub_items,
				collect);
			for (k = 0; k < num_of_sub_items; k++) {
				get_alarm(i, j, k, NAME, name, collect);
				if(strstr(name, "Alarm")) {
					set_alarm(i, j, k, TYPE, MAIN, collect);
				} else if(strstr(name, "Upper")) {
					set_alarm(i, j, k, TYPE, U_VALUE,
						collect);
				} else if(strstr(name, "Lower")) {
					set_alarm(i, j, k, TYPE, L_VALUE,
						collect);
				} else if(strstr(name, "Set")) {
					set_alarm(i, j, k, TYPE, SET_ALARM,
						collect);
				} else if(strstr(name, "log")) {
					set_alarm(i, j, k, TYPE, LOG,
						collect);
				} else if(strstr(name, "Period")) {
					set_alarm(i, j, k, TYPE, TIME_PERIOD,
						collect);
				}
				set_alarm(i, j, k, STATUS, FALSE, collect);
				set_alarm(i, j, k, VALUE, 0, collect);
				set_alarm(i, j, k, SELECT, FALSE, collect);
				set_alarm(i, j, k, ALARM, FALSE, collect);
			}
		}
	}
}

void init_item(collect_t collect, info_t info, main_bar_t *main_bar_item, setting_bar_t *setting_bar_item)
{
	int i;
	int j;
	int num_of_items;
	char name[BUF_SIZE];
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];
	unsigned char data[BUF_SIZE];
	int ret = 0;

	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
		get_num_of_items(i, &num_of_items, &info);
		for (j = 0; j < num_of_items; j++) {
			set_status(i, j, VALUE, -99, collect);
			set_status(i, j, STATUS, TRUE, collect);
			set_status(i, j, ALARM, FALSE, collect);
			set_status(i, j, SELECT, FALSE, collect);
			get_status(i, j, NAME, name, collect);
			if (i < NUM_OF_SETTING_BAR_ITEMS - 2)
				set_status(i, j, ALARM, FALSE, collect);
			if (i == INFO) {
				if (strstr(name, "System")) {
					exec_cmd("uname -nsr", result);
					strncpy(collect.info_status[j].content,
						result, BUF_SIZE);
				} else if (strstr(name, "Kversion")) {
					exec_cmd("kversion -a", result);
					strncpy(collect.info_status[j].content,
						result, BUF_SIZE);
				} else if (strstr(name, "BIOS")) {
					sprintf(cmd, "%s get_bios_ver",
						IMPL_FILE);
					exec_cmd(cmd, result);
					strncpy(collect.info_status[j].content,
						result, BUF_SIZE);
				} else if (strstr(name, "Proactive")) {
					sprintf(cmd, "dpkg -l mxpromon | grep mxpromon | awk '{print $3}'");
					exec_cmd(cmd, result);
					sprintf(collect.info_status[j].content, "Version %s", result);
				}
			}
		}
	}

	/*
	 * Initial mainbar and setting bar status and the first one item was
	 * selected
	 */
	for (i = 0; i < NUM_OF_MAIN_BAR_ITEMS; i++) {
		// It is not setting item
		if (i != NUM_OF_MAIN_BAR_ITEMS - 1)
			main_bar_item[i].status = TRUE;
		else
			main_bar_item[i].status = FALSE;
		main_bar_item[i].select = FALSE;
		main_bar_item[i].display = FALSE;
	}
	main_bar_item[0].select = TRUE;

	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS; i++) {
		setting_bar_item[i].status = FALSE;
		setting_bar_item[i].select = FALSE;
	}
	setting_bar_item[0].select = TRUE;
}

/* Call Proactive Monitoring API to update hw info */
void update_cpu_value(collect_t collect, info_t info)
{
	double value_d = 0;
	int i;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	sprintf(cmd, "%s get_cpu_count", IMPL_FILE);
	if (0 == exec_cmd(cmd, result))
		set_status(CPU, 1, VALUE, atoi(result), collect);

	sprintf(cmd, "%s get_cpu_usage_all", IMPL_FILE, i);
	if (0 == exec_cmd(cmd, result)) {
		char *pEnd;
		value_d = strtod (result, &pEnd);
		set_status(CPU, 0, VALUE, value_d, collect);
		for (i = 0; i < info.cpu_core_count; i++) {
			value_d = strtod (pEnd, &pEnd);
			set_status(CPU, i + 2, VALUE, value_d, collect);
		}
	}

	sprintf(cmd, "%s get_temperature 0", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		set_status(CPU, 2 + info.cpu_core_count, VALUE, atoi(result), collect);
	}

	sprintf(cmd, "%s get_milli_volt 0", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		value_d = (double) atoi(result) / (double)1000;
		set_status(CPU, 3 + info.cpu_core_count, VALUE, value_d, collect);
	}
}

void update_mem_value(collect_t collect, info_t info)
{
	double value_d = 0;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	sprintf(cmd, "%s get_mem_usage", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		set_status(MEMORY, 0, VALUE, atoi(result), collect);
	}

	sprintf(cmd, "%s get_mem_total_size", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		set_status(MEMORY, 1, VALUE, atof(result), collect);
	}

	sprintf(cmd, "%s get_mem_avail_size", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		set_status(MEMORY, 2, VALUE, atof(result), collect);
	}

	sprintf(cmd, "%s get_milli_volt 1", IMPL_FILE);
	if (0 == exec_cmd(cmd, result)) {
		value_d = (double)atoi(result) / (double)1000;
		set_status(MEMORY, 3, VALUE, value_d, collect);
	}
}

void update_disk_value(collect_t collect, info_t info)
{
	double value_d = 0;
	char device_name[BUF_SIZE];
	int i;
	int num_of_items;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	get_num_of_items(DISK, &num_of_items, &info);

	for (i = 0; i < num_of_items; i++) {
		get_status(DISK, i, DEVICE_NAME, device_name, collect);
		sprintf(cmd, "%s get_disk_usage %s", IMPL_FILE, device_name);
		if (0 == exec_cmd(cmd, result)) {
			set_status(DISK, i, VALUE, atof(result), collect);
		}

		sprintf(cmd, "%s get_disk_total_size %s", IMPL_FILE,
			device_name);
		if (0 == exec_cmd(cmd, result)) {
			set_status(DISK, i, TOTAL_SIZE, atof(result), collect);
		}

		sprintf(cmd, "%s get_disk_avail_size %s", IMPL_FILE,
			device_name);
		if (0 == exec_cmd(cmd, result)) {
			set_status(DISK, i, AVAIL_SIZE, atof(result), collect);
		}
	}
}

void update_mainboard_value(collect_t collect, info_t info)
{
	double value_d = 0;
	int i;
	char name[BUF_SIZE];
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	for (i = 0; i < info.mainboard_status_count; i++) {
		get_status(MAINBOARD, i, NAME, name, collect);
		if (strstr(name, "Temperature")) {
			sprintf(cmd, "%s get_temperature 1", IMPL_FILE);
			if (0 == exec_cmd(cmd, result)) {
				set_status(MAINBOARD, i, VALUE, atoi(result),
					collect);
			}
		}

		if (strstr(name, "Voltage")) {
			sprintf(cmd, "%s get_milli_volt 2", IMPL_FILE);
			if (0 == exec_cmd(cmd, result)) {
				value_d = (double)atoi(result) / (double)1000;
				set_status(MAINBOARD, i, VALUE, value_d,
					collect);
			}
		}

		if (strstr(name, "Power1")) {
			sprintf(cmd, "%s get_pwr_status 0", IMPL_FILE);
			if (0 == exec_cmd(cmd, result)) {
				set_status(MAINBOARD, i, VALUE, atoi(result),
					collect);
			}
		}
		if (strstr(name, "Power2")) {
			sprintf(cmd, "%s get_pwr_status 1", IMPL_FILE);
			if (0 == exec_cmd(cmd, result)) {
				set_status(MAINBOARD, i, VALUE, atoi(result),
					collect);
			}
		}
	}
}

void update_serial_value(collect_t collect, info_t info)
{
	int i;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	for (i = 0; i < info.uart_status_count; i++) {
		sprintf(cmd, "%s get_uart_status %d", IMPL_FILE, i);
		if (0 == exec_cmd(cmd, result)) {
			set_status(SERIAL, i, VALUE, atoi(result), collect);
		}
	}
}

void update_network_value(collect_t collect, info_t info)
{
	int i;
	char cmd[BUF_SIZE];
	char result[BUF_SIZE];

	for (i = 0; i < info.eth_status_count; i++) {
		sprintf(cmd, "%s get_eth_link %d", IMPL_FILE, i);
		if (0 == exec_cmd(cmd, result)) {
			set_status(NETWORK, i, VALUE, atoi(result), collect);
		}

		sprintf(cmd, "%s get_eth_speed %d", IMPL_FILE, i);
		if (0 == exec_cmd(cmd, result)) {
			set_status(NETWORK, i, SPEED, atoi(result), collect);
		}

		sprintf(cmd, "%s get_eth_usage %d", IMPL_FILE, i);
		if (0 == exec_cmd(cmd, result)) {
			set_status(NETWORK, i, USAGE, atoi(result), collect);
		}
	}
}

FILE *create_log_fd()
{
	char day[BUF_SIZE];
	char file_path[BUF_SIZE];
	char file_name[BUF_SIZE];
	char cmd[BUF_SIZE];
	FILE *write;

	sprintf(cmd, "mkdir -p %s", LOG_PATH);
	exec_cmd(cmd, day);
	sprintf(cmd, "printf \"$(date +%%Y%%m%%d)\"");
	exec_cmd(cmd, day);

	sprintf(file_name, "%s_%s_log", day, UI);
	sprintf(file_path,"%s/%s", LOG_PATH, file_name);
	write = fopen(file_path, "a");
	if (!write) {
		return NULL;
	}

	return write;
}

int check_alarm(collect_t collect, info_t info, int mode) 
{
	int i, j;
	int x , y, z;
	int num_of_sub_items, num_of_items, num_of_alarm;
	char status_name[BUF_SIZE];
	char alarm_name[BUF_SIZE];
	char alarm_subitem_name[BUF_SIZE];
	int status, alarm_status = FALSE, main_alarm_status = FALSE, log_status = FALSE;
	double status_value;
	int alarm_value;
	int alarm_occur = FALSE;
	char result[BUF_SIZE];
	int alarm_trig;
	int alarm_count;
	int alarm_count_value;
	char cmd[BUF_SIZE];
	
	/* For write log */
	char date[BUF_SIZE];
	FILE *write = create_log_fd();

	sprintf(cmd, "printf \"$(date \"+%%Y/%%m/%%d %%H:%%M:%%S\")\"");
	exec_cmd(cmd, date);

	for (i = 0; i < NUM_OF_SETTING_BAR_ITEMS - 2; i++) {
		get_num_of_items(i, &num_of_items, &info);
		for(j = 0; j < num_of_items; j++) {
			get_status(i, j, NAME, status_name, collect);
			x = i;
			get_num_of_alarm(x, &num_of_alarm, &info);
			for (y = 0; y < num_of_alarm; y++) {
				get_alarm(x, y, 0, NAME, alarm_name, collect);
				if (!strstr(alarm_name, status_name))
					continue;

				get_alarm(x, y, 0, STATUS, &main_alarm_status,
					collect);
				if (main_alarm_status == FALSE) {
					set_status(i, j, ALARM, FALSE, collect);
					continue;
				}

				if (mode == DAEMON_MODE) {
					get_alarm(x, y, 0, SUB_ITEMS, &num_of_sub_items, collect);
					for (z = 1; z < num_of_sub_items; z++) {
						get_alarm(x, y, z, NAME, alarm_subitem_name, collect);
						if (strstr(alarm_subitem_name, "Perform"))
							get_alarm(x, y, z, STATUS, &alarm_status, collect);

						if (strstr(alarm_subitem_name, "Message"))
							get_alarm(x, y, z, STATUS, &log_status, collect);
					}
				}

				if (strstr(status_name, "Eth") ||
					strstr(status_name, "Power")) {

					get_status(i, j, VALUE, &status_value, collect);
					if (status_value == 0) {
						set_status(i, j, ALARM, TRUE, collect);
						alarm_occur = TRUE;
						if (mode == DAEMON_MODE) {
							if (log_status == TRUE) {
								fprintf(write, "%s : %s is lost.\n", date, status_name);
							}
							if (alarm_status == TRUE) {
								exec_cmd("sh /sbin/mx_perform_alarm > /dev/null 2>&1 &", result);
								if (strstr(status_name, "Power")) {
									int pwrstatus = 0;
									if (strstr(status_name, "Power1")) {
										pwrstatus |= 0x1;
									}
									if (strstr(status_name, "Power2")) {
										pwrstatus |= 0x2;
									}
									sprintf(cmd, "%s send_snmptrap power_ind 0 i %d", IMPL_FILE, (int) pwrstatus);
									exec_cmd(cmd, result);
								} else if (strstr(status_name, "Eth")) {
									sprintf(cmd, "%s send_snmptrap eth_link %d i %d", IMPL_FILE, j, (int) status_value);
									exec_cmd(cmd, result);
								}
							}
						}
						break;
					} else {
						set_status(i, j, ALARM, FALSE, collect);
					}
				}

				if (strstr(status_name, "Usage") ||
					strstr(status_name, "Temperature") ||
					strstr(status_name, "/dev")) {
					int upper_occur = 0;
					int lower_occur = 0;
					get_alarm(x, y, 1, STATUS, &status, collect);
					if(status == TRUE) {
						alarm_trig = 0;
						get_status(i, j, VALUE, &status_value, collect);
						get_alarm(x, y, 1, VALUE, &alarm_value, collect);

						if (status_value > alarm_value) {
							get_alarm(x, y, 5, VALUE, &alarm_count_value, collect);
							get_alarm(x, y, 5, COUNT, &alarm_count, collect);
							alarm_count += 1;
							if (alarm_count>=alarm_count_value) {
								set_status(i, j, ALARM, U_VALUE, collect);
								alarm_trig = 1;
								alarm_count = 0;
							}
							set_alarm(x, y, 5, COUNT, alarm_count, collect);
							upper_occur = 1;
						}

						if (alarm_trig) {
							alarm_occur = TRUE;
							if (mode == DAEMON_MODE) {
								if (log_status == TRUE) {
									if (strstr(status_name, "Temperature"))
										fprintf(write, "%s : %s (%.0f degrees C) is over the upper threshold (%d degrees C)\n", date, status_name, status_value, alarm_value);
									else
										fprintf(write, "%s : %s (%.2f %%) is over the upper threshold (%d %%)\n", date, status_name, status_value, alarm_value);
								}

								if (strstr(status_name, "CPU Temperature")) {
									sprintf(cmd, "%s send_snmptrap cpu_temp 0 i %d", IMPL_FILE, (int) status_value);
									exec_cmd(cmd, result);
								} else if (strstr(status_name, "CPU Total Usage")) {
									sprintf(cmd, "%s send_snmptrap cpu_usage 0 i %d", IMPL_FILE, (int) status_value);
									exec_cmd(cmd, result);
								} else if (strstr(status_name, "Memory Usage")) {
									sprintf(cmd, "%s send_snmptrap mem_usage 0 i %d", IMPL_FILE, (int) status_value);
									exec_cmd(cmd, result);
								} else if (strstr(status_name, "/dev")) {
									sprintf(cmd, "%s send_snmptrap disk_usage %d i %d", IMPL_FILE, j, (int) status_value);
									exec_cmd(cmd, result);
								}

								if (alarm_status == TRUE) {
									exec_cmd("sh /sbin/mx_perform_alarm > /dev/null 2>&1 &", result);
								}
							}
							break;
						}
					}

					get_alarm(x, y, 2, STATUS, &status, collect);
					if(status == TRUE) {
						alarm_trig = 0;
						get_status(i, j, VALUE, &status_value, collect);
						get_alarm(x, y, 2, VALUE, &alarm_value, collect);
						if (status_value < alarm_value) {
							get_alarm(x, y, 5, VALUE, &alarm_count_value, collect);
							get_alarm(x, y, 5, COUNT, &alarm_count, collect);
							alarm_count += 1;
							if (alarm_count>=alarm_count_value) {
								set_status(i, j, ALARM, L_VALUE, collect);
								alarm_trig = 1;
								alarm_count = 0;
							}
							set_alarm(x, y, 5, COUNT, alarm_count, collect);
							lower_occur = 1;
						}

						if (alarm_trig) {
							alarm_occur = TRUE;
							if (mode == DAEMON_MODE) {
								if (log_status == TRUE) {
									if (strstr(status_name, "Temperature"))
										fprintf(write, "%s : %s (%.0f degrees C) is below the lower threshold (%d degrees C)\n", date, status_name, status_value, alarm_value);
									else
										fprintf(write, "%s : %s (%.2f %%) is below the lower threshold (%d %%)\n", date, status_name, status_value, alarm_value);
								}

								if (strstr(status_name, "CPU Temperature")) {
									sprintf(cmd, "%s send_snmptrap cpu_temp 0 i %d", IMPL_FILE, (int) status_value);
									exec_cmd(cmd, result);
								} else if (strstr(status_name, "CPU Total Usage")) {
									sprintf(cmd, "%s send_snmptrap cpu_usage 0 i %d", IMPL_FILE, (int) status_value);
									exec_cmd(cmd, result);
								} else if (strstr(status_name, "Memory Usage")) {
									sprintf(cmd, "%s send_snmptrap mem_usage 0 i %d", IMPL_FILE, (int) status_value);
									exec_cmd(cmd, result);
								} else if (strstr(status_name, "/dev")) {
									sprintf(cmd, "%s send_snmptrap disk_usage %d i %d", IMPL_FILE, j, (int) status_value);
									exec_cmd(cmd, result);
								}

								if (alarm_status == TRUE) {
									exec_cmd("sh /sbin/mx_perform_alarm > /dev/null 2>&1 &", result);
								}
							}
							break;
						}
					}

					if ( upper_occur == 0 && lower_occur == 0 ) {
						set_alarm(x, y, 5, COUNT, 0, collect);
						set_status(i, j, ALARM, FALSE, collect);
					}
				}
			}
		}
	}

	fclose(write);
	return alarm_occur;
}

void update_hardware_info(int i, collect_t collect, info_t info)
{
	if ( i == CPU ) {
		update_cpu_value(collect, info);
	} else if ( i == MEMORY ) {
		update_mem_value(collect, info);
	} else if ( i == DISK ) {
		update_disk_value(collect, info);
	} else if ( i == MAINBOARD ) {
		update_mainboard_value(collect, info);
	} else if ( i == SERIAL ) {
		update_serial_value(collect, info);
	} else if ( i == NETWORK ) {
		update_network_value(collect, info);
	}
}

