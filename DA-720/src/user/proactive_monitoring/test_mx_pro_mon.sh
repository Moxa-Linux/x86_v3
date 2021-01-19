#!/bin/bash
#
# The program is unit test for mx_pro_mon_imp
#
# Copyright (C) MOXA Inc. All rights reserved.
#
# History:
#  2016/08/03 Holsety Chen <holsety.chen@moxa.com>
#    First created.
#

TEST_CNT=0
ERROR_CNT=0
function excmd() {
echo -ne "Execute $1 $2... "
TEST_CNT=$((TEST_CNT+1))
RESULT=$($EXECFILE $1 $2)
if [ x"$RESULT" == x"" ]; then
	echo "error: $1 $2"
        ERROR_CNT=$((ERROR_CNT+1))
        return
fi
echo " * Result=$RESULT"
}

# Check test file
if test -f ./mx_pro_mon_imp; then
	EXECFILE="./mx_pro_mon_imp"
elif test -f /usr/bin/mx_pro_mon_imp; then
	EXECFILE="/usr/bin/mx_pro_mon_imp"
elif [ x"$1" != x"" ]; then
	if test -f $1; then
		EXECFILE=$1
	else
		echo "Test file not found!!"
		exit 1
	fi
else
	echo "No test file!!"
	exit 1
fi

# CPU
excmd "get_cpu_count";
CPU_COUNT=$RESULT

excmd "get_average_cpu_usage"

for i in $(eval echo {1..$CPU_COUNT}); do
	excmd get_cpu_usage $((i-1))
done

# MEM
excmd "get_mem_usage"
excmd "get_mem_total_size"
excmd "get_average_cpu_usage"

# UART
UART_COUNT=$(ls /dev/tty* | grep -e ttyM -e ttyS | wc -l)
for i in $(eval echo {1..$UART_COUNT}); do
	excmd get_uart_status $((i-1))
done

# ETH
ETH_COUNT=$(ifconfig | grep eth | wc -l)
if [ x"$ETH_COUNT" != x"" ]; then
	for i in $(eval echo {1..$ETH_COUNT}); do
		excmd get_eth_speed $((i-1))
		excmd get_eth_link $((i-1))
		excmd get_eth_usage $((i-1))
	done
fi

# DISK
listdisk=$(df | grep sd | awk '{printf $1"\n"}')
for disk in $listdisk; do
  excmd get_disk_usage $disk
  excmd get_disk_total_size $disk
  excmd get_disk_avail_size $disk
done

# POWER STATUS
excmd get_pwr_status 1
excmd get_pwr_status 2

# RELAY
excmd set_relay 1

# SYSINFO
excmd get_device_name
excmd get_bios_ver
excmd get_ser_num

# TEMP.
excmd get_temperature 0
excmd get_temperature 1

# voltage
excmd get_milli_volt 0
excmd get_milli_volt 1
excmd get_milli_volt 2

echo "Test complete! Test count = $TEST_CNT, Un-success count = $ERROR_CNT"
exit 0

