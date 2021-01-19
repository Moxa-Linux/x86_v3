#!/bin/sh


while [ 1 ]; do
	date >> /var/log/networking-check.log
	ping -q -w 1 192.168.3.127
	if [ $? -eq  0 ]; then
		echo "Network is available" >> /var/log/networking-check.log
	else
		echo "Network is not available" >> /var/log/networking-check.log
	fi
	sleep 1
done
