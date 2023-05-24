This source tree supports following x86 models.

## For DA-720/682B/682B-TCC:

	modules/moxa_misc
	DA-720/src/modules/moxa_da720_mxser	( The serial port device driver for Debian 8 - linux-3.x )
	DA-682B-TCC/src/modules/mxser	( For serial port device driver for Debian 9, 10, Ceont/Redhat OS 7, 8 - linux-4.x )
	The latest serial port device driver for Debian 11 is availabe from Moxa' multiport serial board
	https://www.moxa.com/en/products/industrial-edge-connectivity/multiport-serial-boards/pcie-upci-pci-serial-cards/cp-168u-series#resources
	DA-720/src/modules/moxa_IRIG-B
	DA-720/src/modules/moxa_it87_wdt

	DA-720/src/user/mxhsrprp
	DA-720/src/user/irigb

## MC-7400:

	MC-7400/src/modules/moxa_it87_wdt
	MC-7400/src/modules/skl_dmc_ver1_26
	modules/moxa_misc
	modules/r8168-8.046.00

## MC-1100:

	modules/moxa_misc
	modules/r8168-8.046.00

## MPC-21XX: MPC-2070/2120 and MPC-2101/2121

	modules/moxa_misc
	modules/r8168-8.046.00
	modules/DPO_RT5572_LinuxSTA_2.6.1.3_SparkLAN

	MPC-21XX/src/user/BrightnessControl
	MPC-21XX/src/user/br-util
	MPC-21XX/src/user/mtview

## EXPC-1519: EXPC-1519

	modules/moxa_misc
	EXPC-1519/romfs_debian9/etc/udev/rules.d/01-rename_uart_naming.rules

## Example

	user/system_service_example	Systemd service example
	user/gpio_example		tgpio.c is the DI/DO programming example

