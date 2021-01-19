#!/bin/bash
set -e
### BEGIN INIT INFO
# Provides:          Start Moxa Poactive Monitoring daemon
# Required-Start:
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO

. /lib/lsb/init-functions
PIDFILE=/var/run/pro_mond.pid
udevadm trigger
case "$1" in

start)
        echo "Starting Moxa Poactive Monitoring daemon..."
        sleep 1
        if [ -f /var/run/pro_mond.pid ] ; then
                echo "WARNING! Moxa Poactive Monitoring daemon is running, remove /var/run/pro_mond.pid if it terminated"
                exit 0
        fi
        #Add parameter if necessary
        pro_mond &
        ;;
stop)
        echo "Stopping Moxa Poactive Monitoring daemon..."
        rm -f /var/run/pro_mond.pid
        killall -w pro_mond
        ;;
restart)
        echo "Restarting Moxa Poactive Monitoring daemon..."
        killall -w pro_mond &> /dev/null
        if [ -f /var/run/pro_mond.pid ] ; then
                echo "WARNING! Moxa Poactive Monitoring daemon is running, remove /var/run/pro_mond.pid if it is no longer running"
                exit 0
        fi
        pro_mond &
        ;;
*)
        exit 2
        ;;
esac

exit 0
