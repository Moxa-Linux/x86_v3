
The Moxa misc driver supported to access the relay/DI/DO, on board uart interface and programable LED control on DA-720/DA-682B, MC-1100, MC-7400, MP-2070/2120/2121, ... series embedded computer.
The driver is based on GPIO sysfs, the Linux kernel shuld support thses features, CONFIG_GPIO_SYSFS=y and CONFIG_GPIOLIB=y.

## Check kernel features

    Check the kernel supports supports CONFIG_GPIOLIB=y and CONFIG_GPIO_SYSFS=y.
    This driver is implement based on Linux kernel CONFIG_GPIOLIB=y and CONFIG_GPIO_SYSFS=y function, you should check these value in /boot/config-$(uname -r).
    For exmampe, the offical Debian 8 default doesn't built with CONFIG_GPIOLIB=y and CONFIG_GPIO_SYSFS=y, you should install the latest backports kernel to support these feature.
    The steps to install the kernel headers and image.

    ### Add backports repository in /etc/apt/sources.list

        deb http://ftp.debian.org/debian/ jessie-backports main
        deb-src http://ftp.debian.org/debian/ jessie-backports main

    ### Update packages list and install the backport kernel

        $ apt-get update
        $ apt-get install -t jessie-backports linux-headers-`uname -r` linux-image-`uname -r`

    The steps to install the stretch kernel.

    ### Add stretch repository in /etc/apt/sources.list

        deb http://ftp.us.debian.org/debian/ stretch main
        deb-src http://ftp.us.debian.org/debian/ stretch main

    ### Update packages list and install the backport kernel

        $ apt-get update
        $ apt-get install linux-headers-`uname -r` linux-image-`uname -r`

## Install the drivers

    You should configure the CONFIG_PRODUCT in Makefile as one of these model, mc1100, mpc21xx, da720 or mc7400 before making this driver.

    $ vi Makefile

    EX: MC-1100
    CONFIG_PRODUCT?=mc1100 
    EX: MC-7400
    CONFIG_PRODUCT?=mc7400 
    EX: DA-720/DA-682B
    CONFIG_PRODUCT?=da720
    EX: MPC-2070/2120/2101/2121
    CONFIG_PRODUCT?=mpc21xx

    $ make

    Then install the driver and update the modules.dep file

    $ make install
    $ depmod -a

    The driver should be installed at /lib/modules/`uname -r`/kernel/drivers/misc/ and /lib/modules/`uname -r`/modules.dep should be updated.

    Create a blacklist in /etc/modprobe.d/ to prevent the device driver confliction issue.

    $ vi /etc/modprobe.d/iTCO-blacklist.conf

    blacklist iTCO_wdt
    blacklist iTCO_vendor_support
    blacklist 8250_fintek

    Load the driver it87, it87_wdt and misc-moxa-xxx in /etc/modules

    $ vi /etc/modules
    ...
    it87
    it87_wdt
    misc-moxa-mc1100
    # It might be misc-moxa-mc7400, misc-moxa-mc1100, misc-moxa-da720, misc-moxa-mpc21xx
    #misc-moxa-mpc21xx
    #misc-moxa-mc7400
    #misc-moxa-da720
    
    Add module option for it87_wdt watchdog driver in /lib/modprobe.d/it87_wdt.conf
    options it87_wdt nocir=1

## Export/unexport GPIO

    To use sysfs/gpio, we have prepare a convent script, mx_gpio_export, to export/unexport GPIO port in systemd service.

    Note: The mx_gpio_export.service is systemd-based service. If the Linux doesn't support systemd, you cannot use it.

    ### Manage GPIO from command line or script:

    From the user level side this "operation" for reserve the GPIO is called "export" the GPIO.
    For make this export operation you simply need to echo the GPIO number you are interested to a special path as follow (change XX with the GPIO number you need):

    ### Export script:

    #!/bin/sh
    export_gpio() {
        local TARGET_GPIOCHIP=moxa-gpio
        local GPIOCHIP_NAME=gpiochip
        local GPIO_FS_PATH=/sys/class/gpio
        local GPIO_EXPORT="export"

        if [ x"$1" == x"unexport" ]; then
            GPIO_EXPORT="unexport"
        fi

        #
        # Export GPIOs
        #
        ls $GPIO_FS_PATH | grep $GPIOCHIP_NAME | while read -r chip ; do
            GPIO_LABEL=$(cat $GPIO_FS_PATH/$chip/label)
            if [[ "$GPIO_LABEL" != *"$TARGET_GPIOCHIP"* ]]; then
                continue
            fi

            pinstart=$(echo $chip | sed s/$GPIOCHIP_NAME/\\n/g)
            count=$(cat $GPIO_FS_PATH/$chip/ngpio)
            for (( i=0; i<${count}; i++ )); do
                echo $((${pinstart}+${i})) > $GPIO_FS_PATH/$GPIO_EXPORT 2>/dev/null
            done
        done
    }

    case "$1" in
        start)
        # export
        export_gpio
        ;;
    stop)
        # unexport
        export_gpio unexport
        ;;
    *)
        echo "Usage: $0 start|stop" >&2
        exit 3
        ;;
    esac

    To manually export the GPIO port

    $ mx_gpio_export start

    To manually unexport the GPIO port

    $ mx_gpio_export stop

    *** To export the GPIO ports automatically at boot.
    You should put the mx_gpio_export into /sbin/

    $ cp -a mx_gpio_export /sbin/

    ** Create /lib/systemd/system/mx_gpio_export.service

    $ cp -a mx_gpio_export.service /lib/systemd/system/

    $ cat /lib/systemd/system/mx_gpio_export.service

        [Unit]
        Description=Moxa platform initial service
        
        [Service]
        Type=oneshot
        ExecStart=/sbin/mx_gpio_export start
        ExecStop=/sbin/mx_gpio_export stop
        RemainAfterExit=yes
        
        [Install]
        WantedBy=multi-user.target
   
    ### Enable the systemd service

    $ systemctl enable mx_gpio_export.service

    After that, enable the mx_gpio_export.service systemd service. All the ports should be exported in next boot.

    $ systemctl enable mx_gpio_export.service

## Control the GPIO

    These GPIO features are dependent on different platform feature. Not all the fetures designed in one embedded computer.
    EX: DA-720, DA-682B has programmable LED, relay, power status (pwrin), UART interface control
    EX: MC-1100 has programmable LED, DO, DI, UART interface control

    ### Example to turn on/off programmable LED:
    $ echo 1 > /sys/class/gpio/pled1/value
    $ echo 0 > /sys/class/gpio/pled1/value

    ### Example to turn on/off relay:
    $echo 1 > /sys/class/gpio/relay1/value
    echo 0 > /sys/class/gpio/relay1/value

    ### Example to turn on/off DO:
    echo 1 > /sys/class/gpio/do1/value
    echo 0 > /sys/class/gpio/do1/value

    ### Example get the current value:
    cat /sys/class/gpio/pled1/value
    cat /sys/class/gpio/di1/value

    ### The convenience setinterface script
    setinterface.da720, setinterface.mc1100, setinterface.mpc21xx and setinterface.mc7400, sets /dev/ttySn, n=1,2, ... as RS-232/422/485 mode
    You can put this script according to your model to /usr/local/bin/setinterface:

    # setinterface -h
    Usage: setinterface device-node [interface-no]
	device-node     - /dev/ttySn, n = 0, 1, ... . 
	interface-no    - following:
	none - to view now setting
	0 - set to RS232 interface
	1 - set to RS485-2WIRES interface
	2 - set to RS422 or RS485-4WIRES interface

    #### Set /dev/ttyS0 as RS485-2WIRES interface
    # setinterface /dev/ttyS0 1

    #### Get /dev/ttyS0 interface
    # setinterface /dev/ttyS0 
    Now setting is RS485-2WIRES interface.

    #### Manage GPIO in a C program:
    ##### Set Example:
    sprintf(buf, "/sys/class/gpio/pled1/value", gpio);

    fd = open(buf, O_WRONLY);

    // Set GPIO high status
    write(fd, "1", 1); 
    // Set GPIO low status 
    write(fd, "0", 1); 
    
    close(fd);

