#!/bin/sh

FIRMWARE_FILE=skl_dmc_ver1_26.bin

# The $KERNEL_FIRMWARE_DIR varies on different distribution.
# On Linux it is typically /lib/firmware.
# While on Android, it could be /etc/firmware.
KERNEL_FIRMWARE_DIR="/lib/firmware"

mkdir -p ${KERNEL_FIRMWARE_DIR}/i915
cp  ${FIRMWARE_FILE} $KERNEL_FIRMWARE_DIR/i915

if [ ! -f "$KERNEL_FIRMWARE_DIR/i915/${FIRMWARE_FILE}" ]; then
    echo "ERROR: Couldn't install new firmware file"
    exit -1
else
    echo "Success: ${KERNEL_FIRMWARE_DIR}/i915/${FIRMWARE_FILE} installed!"
fi

echo "Forcing initrd/initramfs update..."

# Update initrd/initramfs
# For distros with Dracut: (Fedora)
DRACUT=$(which dracut 2> /dev/null)

# Update initrd/initramfs
# For distros with Initramfs: (Ubuntu)
UPDATEINITRAMFS=$(which update-initramfs 2> /dev/null)

if [ $DRACUT ]; then
    INITRD=/boot/"initramfs-$(uname -r).img"
elif [ $UPDATEINITRAMFS ]; then
    INITRD=/boot/"initrd.img-$(uname -r)"
else
    echo "FAIL: No such file: This script depends on Dracut or Update-iniramfs."
    exit -1
fi

echo "Trying to backup $INITRD"
rm -rf $INITRD.i915-fw.backup

if ! cp $INITRD $INITRD.i915-fw.backup; then
    echo "Created a backup of your current initramfs $INITRD.i915-fw.backup"
else
    echo "WARNING: Couldn't create a backup of your current initramfs"
fi

echo "Trying to update $INITRD"
if [ $DRACUT ]; then
    cp i915-dracut.conf /etc/dracut.conf.d/i915-${FIRMWARE_FILE}.conf
    dracut $INITRD $(uname -r) --force
else
    cp i915-update-initramfs /usr/share/initramfs-tools/hooks/i915-${FIRMWARE_FILE}
    chmod +x /usr/share/initramfs-tools/hooks/i915-${FIRMWARE_FILE}
    update-initramfs -k $(uname -r) -u
fi

echo "Success: Please reboot your machine!"
