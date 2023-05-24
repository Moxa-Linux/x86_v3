#!/bin/sh
set -x

#SRC=/media/ramdisk/os_image/sda1.ext4-ptcl-img.gz.aa
SRC=$1
#DST=/media/ramdisk/sda1.img
DST=$2
# IMAGE COMPRESSION 0: gzip, 1: bzip2, 2: xz
COMPRESSION=0

MNT=mnt_point

install_packages() {
	echo "Install the required packages"
	apt-get update
	apt-get install -y partclone
}

extract_clonezilla_image() {

	echo "Extract clonezilla image"

	case "$COMPRESSION" in
		"0" )
		#Method 1
		#cat $1|gzip -d -c|partclone.restore -C -s - -O $2
	
		#Method 2
		cat $1* | gzip -d -c > $2

		#cat $1|bzip2 -d -c|partclone.restore -C -s - -O $2
		#cat $1|xz -d -c|partclone.restore -C -s - -O $2
		;;
		"1" )
		cat $1* | bzip2 -d -c > $2
		;;
		"2" )
		cat $1* | xz -d -c > $2
		;;
		*)
		cat $1* | gzip -d -c > $2
		;;
	esac

	partclone.extfs -r -s $2 -o $2-restored.img --restore_raw_file
	e2fsck -f $2-restored.img
	resize2fs $2-restored.img

	mv $2-restored.img $2
}

extract_target_platform_dependent_packages() {

	echo "Nothing to do"
	# Originally, I want to dpkg -i to install mc1100-misc_1.0_amd64.deb but it's seems not install.
	# So I extract it directly
	#cp -a mc1100-misc_1.0_amd64.deb $MNT_PATH/
	#dpkg -x mc1100-misc_1.0_amd64.deb $MNT_PATH/

}

mount_modify_rootfs() {

	IMG=$1
	MNT_PATH=$2
	
	echo "Mount and update the image"
	# Mount the image
	mkdir -p $MNT
	mount -t ext4 -o loop $IMG $MNT_PATH

	# Update the image
	mount -o bind /proc $MNT_PATH/proc
	mount -o bind /dev $MNT_PATH/dev
	mount -o bind /sys $MNT_PATH/sys


#Create the script
cat <<GLOBALEOF >$MNT_PATH/update.sh
set -x
# Remove the not necessary packages
remove_target_packages() {
	apt-get -y --purge remove ed
	apt-get -y --purge remove eject
	apt-get -y --purge remove exim4
	apt-get -y --purge remove exim4-base
	apt-get -y --purge remove exim4-config
	apt-get -y --purge remove exim4-daemon-light
	apt-get -y --purge remove nfs-common
	apt-get -y --purge remove ppp
	apt-get -y --purge remove modemmanager
	apt-get -y --purge clean
}
# Install the extra software packages
install_target_common_packages() {
	echo "nameserver 10.128.8.5" >/etc/resolv.conf
	apt-get update
	DEBIAN_FRONTEND=noninteractive apt-get upgrade -yq
	apt -y autoremove
	# MPC-2070/2120 and MPC-2101/2121 default doesn't support openvpn
	#apt-get -y install openvpn
	# Remove due to non-secured
	#apt-get install -y proftpd-basic # add, RootLogin on, into /etc/proftpd/proftpd.conf let root login
	# Install the telnetd/tftpd but default not to start it.
	#apt-get install -y tftpd
	#sed -i 's/^telnet/#telnet/g' /etc/inetd.conf
	#apt-get install -y telnetd
	#apt-get install -y telnet
	#sed -i 's/^tftp/#tftp/g' /etc/inetd.conf
	#apt-get install -y ppp
	#apt-get install -y pppoe
	#apt-get install -y pppoeconf
	apt-get install -y bridge-utils # for openvpn bridge mode
	apt-get install -y iproute      # for openvpn bridge mode
	apt-get install -y ifenslave-2.6
	apt-get install -y ntpdate
	apt-get install -y openssl
	apt-get install -y ssh
	apt-get install -y ssl-cert
	apt-get install -y etherwake
	apt-get install -y man-db
	apt-get install -y libperl-dev
	apt-get install -y ethtool	# This is important for wol feature
	#apt-get install -y dmidecode	# Get serial number of product in OS level
	apt-get install -y pm-utils	# Power management utility
	apt-get install -y sysstat	# For reading the system loading
	apt-get install -y usbutils
	#apt-get install -y usbmount	# For USB storage auto mounting
	# The platform has watchdog timer
	apt-get install -y watchdog
	# For web application development, MPC-2000 default does not support web applications
	#apt-get install -y apache2      # for apache mode
	#apt-get install -y php5 libapache2-mod-php5  # for PHP support
	#apt-get install -y sqlite3
	#apt-get install -y libsqlite3-dev
	# Install the lm-sensors for hardware monitor
	apt-get install -y lm-sensors
	# Install the toolchain
	apt-get install -y build-essential vim libncurses-dev bzip2
	apt-get install -y binutils-multiarch gcc-multilib g++-multilib
	#apt-get install -y libqt4-dev qt4-dev-tools libqt4-designer
	apt-get install -y qt5-default qtdeclarative5-dev qtbase5-dev qtbase5-dev-tools qtcreator
	apt-get install -y dialog strace
	apt-get install -y snmp
	#apt-get install -y snmpd
	apt-get install -y minicom
	apt-get install -y tcpdump
	#apt-get install -y ifrename	# For renaming a network interface
	#apt-get install -y gdb 
	#apt-get install -y efibootmgr	#
}
install_target_platform_dependent_packages() {
	# Install the backport kernel which fixes the VGA display issue
	apt-get install -y linux-headers-4.9.0-8-amd64 linux-image-4.9.0-8-amd64 linux-manual-4.9
	# Install Gnome3
	#apt-get install -y task-gnome-desktop
	# Install XFCE
	apt-get install -y xfce4
	# Install the slim. However the florence only supports florence.
	# apt-get install -y slim
	# Install the gdm3
	# apt-get install -y gdm3
	# Install the lightdm, http://wiki.debian.org/lightdm
	apt-get install -y lightdm lightdm-gtk-greeter
	# Install 
	# Install virtual keyboard - onboard and numlockx
	apt-get install -y onboard numlockx
	# Auto start onboard virtual keyboard after login
	cp -a /usr/share/applications/onboard.desktop /etc/xdg/autostart/
	# Install virtual keyboard - florence
	#apt-get install -y florence
	# Auto start florence virtual keyboard in gdm3
	#cp -a /usr/share/applications/florence.desktop /usr/share/gdm/greeter/autostart/
	# Auto start florence virtual keyboard after login
	#mkdir -p /home/moxa/.config/autostart
	#cp -a /usr/share/applications/florence.desktop /home/moxa/.config/autostart/
	# The platform has audio device
	#apt-get install -y alsa-utils
	#echo "options snd-hda-intel model=moxa position_fix=2" >> /etc/modprobe.d/alsa-base
	# The platform has wifi module
	#apt-get install -y iw wpasupplicant
	# The platform has TPM
	#apt-get install -y tpm-tools
	# Moxa's packages
	# libqmi for Talit LE910 LTE module, 
	#1. Import verification key with:
	#wget -q http://debian-jenkins.syssw.moxa.com/public.gpg -O- | sudo apt-key add -
	#2. Add this line into apt source list
	#echo "deb http://debian-jenkins.syssw.moxa.com/utility/develop ./" >> /etc/apt/sources.list
	#3. Install moxa-cellular-utils from jenkins server
	apt-get update
	apt-get install -y moxa-cellular-utils
	# Install the extra packages
	wget -q http://127.0.0.1/x86/MPC-2000/mpc2000-br-util_1.1_amd64.deb
	dpkg -i --force-overwrite ./mpc2000-br-util_1.1_amd64.deb
	wget -q http://127.0.0.1/x86/MPC-2000/mpc21xx-misc_1.0_amd64.deb
	dpkg -i --force-overwrite ./mpc21xx-misc_1.0_amd64.deb
	rm -rf ./mpc2000-br-util_1.1_amd64.deb ./mpc21xx-misc_1.0_amd64.deb
}
# Stop some running daemon during system customization
stop_services() {
	/etc/init.d/nfs-common stop
	/etc/init.d/cron stop
}
# Customize the root file system
customize_target_rootfs() {
cat > /etc/modprobe.d/bluetooth-blacklist.conf <<BBEOF
blacklist joydev
blacklist floppy
blacklist btbcm
blacklist btqca
blacklist btintel
blacklist hci_uart
blacklist bluetooth
BBEOF

cat > /etc/modprobe.d/alsa-base.conf <<ALSAEOF
options snd-hda-intel position_fix=2
ALSAEOF

cat > /etc/modprobe.d/iTCO-blacklist.conf <<ITCOEOF
#blacklist snd-pcsp
#blacklist pcspkr
blacklist iTCO_wdt
blacklist iTCO_vendor_support
#blacklist parport_pc
#blacklist parport
blacklist 8250_fintek
#blacklist 8250_dw
# tusb3410 is disabled by moxa
blacklist ti_usb_3410_5052
ITCOEOF

cat > /etc/modprobe.d/it87_wdt.conf <<IT87EOF
options it87_wdt nocir=1 test_mode=1
IT87EOF

cat > /etc/modprobe.d/it87_wdt.conf <<RT28000USBEOF
blacklist rt2800usb
RT28000USBEOF

cat > /etc/modules <<MODULESEOF
8021q
it87
it87_wdt
misc-moxa-mpc21xx
MODULESEOF

cat > /etc/fstab <<FSTABEOF
LABEL=root /               ext4    noatime,errors=remount-ro 0       1
#/dev/sdb1	/mnt	vfat	defaults,nofail	0	0
#/var/swapfile	none	swap	sw	0	0
FSTABEOF

cat > /etc/network/interfaces <<INTERFACESEOF
source /etc/network/interfaces.d/*

# The loopback network interface
auto lo
iface lo inet loopback

auto enp1s0
iface enp1s0 inet static
	address 192.168.3.127
	network 192.168.3.0
	netmask 255.255.255.0

# Tag the VLAN ID, 10, of the Ethernet interface, enp1s0.
#auto enp1s0.10
#iface enp1s0.10 inet static
#	address 192.168.5.127
#	netmask 255.255.255.0
#	vlan-raw-device enp1s0

auto enp2s0
iface enp2s0 inet static
	address 192.168.4.127
	network 192.168.4.0
	netmask 255.255.255.0

#auto ra0
#iface ra0 inet dhcp
	#wpa-ssid dlink_24GHz_luke
	#wpa-psk 12345678
#	wpa-conf /etc/wpa_supplicant/wpa2_aes.conf
INTERFACESEOF

# Change timezone
#cp -a /usr/share/zoneinfo/Asia/Taipei /etc/localtime
#echo "Asia/Taipei" > /etc/timezone

sed -i 's/^MountFlags=slave/#MountFlags=slave/' /lib/systemd/system/systemd-udevd.service

# Renaming the Ethernet port
cat > /etc/udev/rules.d/11-media-by-label-auto-mount.rules <<MOUNTUSBEOF
KERNEL!="mmcblk[0-9]p[0-9]", GOTO="media_by_label_auto_mount_end"  
# Import FS infos  
IMPORT{program}="/sbin/blkid -o udev -p %N"  
# Get a label if present, otherwise specify one  
ENV{ID_FS_LABEL}!="", ENV{dir_name}="%E{ID_FS_LABEL}"  
ENV{ID_FS_LABEL}=="", ENV{dir_name}="sd-%k"  
# Global mount options  
ACTION=="add", ENV{mount_options}="relatime"  
# Filesystem-specific mount options  
ACTION=="add", ENV{ID_FS_TYPE}=="vfat|ntfs", ENV{mount_options}="$env{mount_options},utf8,gid=100,umask=002"  
# Mount the device  
ACTION=="add", RUN+="/bin/mkdir -p /media/%E{dir_name}", RUN+="/bin/mount -o $env{mount_options} /dev/%k /media/%E{dir_name}"  
# Clean up after removal  
ACTION=="remove", ENV{dir_name}!="", RUN+="/bin/umount -l /media/%E{dir_name}", RUN+="/bin/rmdir /media/%E{dir_name}"  
# Exit  
LABEL="media_by_label_auto_mount_end"
MOUNTUSBEOF

cat > /etc/sensors.d/sensor3.conf << MPC2000SENSORSEOF 
# libsensors configuration file -----------------------------
#
# This default configuration file only includes statements which do not 
# differ from one mainboard to the next. Only label, compute and set 
# statements for internal voltage and temperature sensors are included.
#
# In general, local changes should not be added to this file, but rather 
# placed in custom configuration files located in /etc/sensors.d. This 
# approach makes further updates much easier.
#
# Such custom configuration files for specific mainboards can be found 
# at http://www.lm-sensors.org/wiki/Configurations
#
# It is recommended not to modify this file, but to drop your local 
# changes in /etc/sensors.d/. File with names that start with a dot are 
# ignored.
#
# For Moxa MPC-2000
#
chip "it8783-*"
    label in0 "V_CPU"
    set in0_min 0
    set in0_max 2

    label in1 "V_GPU"
    set in1_min 0
    set in1_max 3

    label in2 "V_DRAM"
    #compute in2 @*2, @/2
    set in2_min 1.35*0.95
    set in2_max 1.35*1.05

    ignore in3

    label in4 "V5.0"
    compute in4 @*2, @/2
    set in4_min 5.0*0.9
    set in4_max 5.0*1.1
    
    ignore in5

    ignore in6

    ignore in7

    # VBat
    ignore in8

    ignore fan1
    ignore fan2

    label temp1 "SYSTEM"
    set temp1_min -40
    set temp1_max 85

    label temp2 "SYSTEM2"
    set temp2_min -40
    set temp2_max 85

    ignore temp3
    
    ignore intrusion0
MPC2000SENSORSEOF

	sed -i '/sysLocation/ a sysObjectid .1.3.6.1.4.1.8691.12.2121 ' /etc/snmp/snmpd.conf
	sed -i 's/sysLocation    Sitting on the Dock of the Bay/sysLocation Fl.4, No.135, Lane 235, Baoquao Rd., Xindian Dist., New Taipei City, Taiwan, R.O.C./' /etc/snmp/snmpd.conf
	sed -i 's/sysContact     Me <me@example.org>/sysContact Moxa Inc., Embedded Computing Business. <www.moxa.com>/' /etc/snmp/snmpd.conf
	sed -i 's/ifup -a/ifup -a --force/g' /etc/init.d/networking

	# Not start the systemd-timesyncd at boot time for security concern.
	systemctl disable systemd-timesyncd
	# Disable the keyboard-setup.service
	systemctl disable keyboard-setup.service
	
	depmod -ae

	#----------------Clean up the data on writable partition ------------
	apt-get clean
	apt-get purge

	# Update kversion
	sed -i 's/0.9.2/1.0/' /usr/bin/kversion
	# Update the firmware build date
	date +'%y%m%d%H' > /etc/.builddate
	# Update the firmware build date

	dpkg -l > /packages_list.log

	# Update grub menu to with GRUB_DISABLE_LINUX_UUID=true for the image clone to multiple machines with different UUID issue.
	## Enable GRUB_DISABLE_LINUX_UUID=true in /etc/default/grub
	sed -i 's/^#GRUB_DISABLE_LINUX_UUID/GRUB_DISABLE_LINUX_UUID/' /etc/default/grub
	sed -i 's/^GRUB_TIMEOUT=5/GRUB_TIMEOUT=0/' /etc/default/grub
	sed -i 's/fsck.mode=force fsck.repair=yes acpi_enforce_resources=no/fsck.mode=force fsck.repair=yes acpi_enforce_resources=no net.ifnames=0 biosdevname=0/' /etc/default/grub
	chmod a+w /boot/grub/grub.cfg
	sed -i 's/fsck.mode=force fsck.repair=yes acpi_enforce_resources=no/fsck.mode=force fsck.repair=yes acpi_enforce_resources=no net.ifnames=0 biosdevname=0/' /boot/grub/grub.cfg
	chmod a-w /boot/grub/grub.cfg
	#e2label /dev/sda1 root
	#sed -i '/GRUB_CMDLINE_LINUX/ a GRUB_DEVICE=LABEL=root' /etc/default/grub 
	#update-grub2

	# Remove internal source list
	sed -i '/debian-jenkins.syssw.moxa.com/d' /etc/apt/sources.list.d/moxa.list

	echo Cleanup logs and bash_history
	for i in \`ls /var/log\`; do :>/var/log/\$i; done
	:> /root/bash_history
	:> /home/moxa/.bash_history
	:> /etc/reslov.conf

	find /var/ -name *-old|xargs rm -rf
}

remove_target_packages
install_target_common_packages
#install_target_platform_dependent_packages
customize_target_rootfs

echo "DONE."
exit
GLOBALEOF

	#extract_target_platform_dependent_packages

	chmod a+x $MNT_PATH/update.sh

	# chroot to modify the rootfs
	chroot $MNT_PATH /update.sh

	rm -rf $MNT_PATH/update.sh
	mv $MNT_PATH/packages_list.log .

	umount $MNT_PATH/sys \
	$MNT_PATH/dev \
	$MNT_PATH/proc \
	$MNT_PATH

	# umount the image
	rm -rf $MNT_PATH
}


make_clonezilla() {

	TARGET=`echo $1|cut -f1 -d.`

	echo -n "Making clonezilla image "

	case "$COMPRESSION" in
		"0" )
			echo "compressed by gzip"
			partclone.ext4 -L /var/log/partclone.log -c -s $TARGET.img --output - | gzip -c -f --best --rsyncable | split -a 2 -b 1000000MB - $TARGET.ext4-ptcl-img.gz.

			#partclone.ext4 -L /var/log/partclone.log -c -s $TARGET --output $TARGET-new.img
			#gzip -c -f --fast --rsyncable $TARGET-new.img | splite -a 2 -b 1000000MB - $TARGET.ext4-ptcl-img.gz.
			;;
		"1" )
			echo "compressed by bzip2"
			partclone.ext4 -L /var/log/partclone.log -c -s $TARGET.img --output - | bzip2 -c -f --fast | split -a 2 -b 1000000MB - $TARGET.ext4-ptcl-img.bz2.
			;;
		"2" )
			echo "compressed by xz"
			partclone.ext4 -L /var/log/partclone.log -c -s $TARGET.img --output - | xz -c -f --fast | split -a 2 -b 1000000MB - $TARGET.ext4-ptcl-img.xz.
			;;
		*)
			echo "compressed by xz default"
			partclone.ext4 -L /var/log/partclone.log -c -s $TARGET.img --output - | xz -c -f --fast | split -a 2 -b 1000000MB - $TARGET.ext4-ptcl-img.xz.
			;;
	esac

}

usage_update_clonezilla() {
	echo "usage: update_clonezilla.sh SRC.gz.aa DST.img"
	echo "EX: update_clonezilla.sh /PATH/TO/os_image/sda1.ext4-ptcl-img.gz.aa sda1.img"
	exit 0
}

if [ -z "$DST" ]; then
	echo "Please input the destination file"
	usage_update_clonezilla
fi
if [ -z "$SRC" ]; then
	echo "Please input the source file"
	usage_update_clonezilla
fi

install_packages
extract_clonezilla_image $SRC $DST
mount_modify_rootfs $DST $MNT
make_clonezilla $DST

