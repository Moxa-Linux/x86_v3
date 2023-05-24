#!/bin/bash

usage_upgrade_mcfrm() {
	echo "Please input the micro-controller firmware in the argument"
	echo "EX: upgrade_mcfrm.sh ./FWR_MCU_MPC-2121_V1.20S00"
}

if [ -z "$1" ] ; then
	usage_upgrade_mcfrm
	exit 0
fi

# Check the model name from /sys/class/dmi/id/product_name
PRODUCT_NAME="`cat /sys/class/dmi/id/product_name|cut -f2 -d-`"

# Check the model name from /sys/class/dmi/id/board_name
[ -z "$PRODUCT_NAME" ] && PRODUCT_NAME="`cat /sys/class/dmi/id/board_name|cut -f2 -d-`"

# If all are null, set "MPC-2120" as default model name
[ -z "$PRODUCT_NAME" ] && PRODUCT_NAME="2120"

if [ "$PRODUCT_NAME" != "2121" ]; then
	echo "Sorry! This model doesn't not support firmware upgrade."
	exit 0
fi

MC_FRM=$1

if [ -f "$MC_FRM" ]; then
	echo "Upgrad the firmware, $MC_FRM."
else
	echo "$MC_FRM not exist"
	exit 0
fi


MC_FRM_MAGIC1=`od -An -j3 -N4 -tc $MC_FRM|tr -d "[:space:]"`
if [ "$MC_FRM_MAGIC1" != "1000" ]; then
	echo "The firmware is incorrect with $MC_FRM_MAGIC1 patern. Please check the firmware."
	exit 0
fi


MC_FRM_MAGIC2=`od -An -j28 -N32 -tc $MC_FRM|tr -d "[:space:]"`
if [ "$MC_FRM_MAGIC2" != "53D9BF75FF8075EF02787FE4F6D8FD75" ]; then
	echo "The firmware is incorrect with $MC_FRM_MAGIC2 patern. Please check the firmware."
	exit 0
fi

# The Micro-controller UART port should be /dev/ttyS2
MC_PORT="/dev/ttyS2"

trap '' INT
echo "Start to upgrade firmware."

# Initialized UART, /dev/ttyS2, with 9600 n81 and no flow control
stty -F $MC_PORT 9600 cs8 -cstopb cread clocal -crtscts raw echo

# Upgrade firmware: sent and response
#
#        X86                             uC
#  0xF0 0x02 0xCD 0x01 0xCE  --->
#                            <---  0xF0 0x01 0xCD 0xCD
#
#echo -n -e "\\xF0\\x02\\xCD\\x01\\xCE"> $MC_PORT
#
# Skip to read and check the response
#read -s -N 4 -t 1 response < $MC_PORT

br-util -u -q
if [ "$?" != "0" ]; then
	echo "Brightness controller cannot start the firmware upgrade. $?"
	exit 0
fi

echo -n "Wait for the firmware upgrade procedure..."

# The firmware upgrade steps
# Step 1. please input 1 to erase the NAND flash.
# Step 2. press Ctrl-a-s to start micro-controller's firmware upgrade.
# Step 3. press Ctrl-a-x to exit picocom after upgrade complete. 

/usr/bin/expect  <<END

#exp_internal -f debug_info.log 0

spawn screen $MC_PORT
expect "Enter a command >"
send "1\n"
#expect "finished ***"
expect "Enter a command >"
send "2\n"
expect "Ready to receive Hex file...\n"
exec clear
exit
END

# Note: Wait a moment, don't skip this waiting or firmware upgrade might fail.
sleep 1

#echo "Waiting for the firmware upgrade."
echo "!!!Please don't power-off or stop the upgrade before it finished!!!"
echo ""

# End the screen session
screen -X quit

# Start the ASCII transfer and firmware upgrade
ascii-xfr -c -n -e -v -s $MC_FRM > $MC_PORT

trap INT

echo "Firmware upgrade finished."
echo "You can check the firmware version by br-util -f."
