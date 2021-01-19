/*
 *  GPIO interface for Moxa Computer - IT8786 Super I/O chip
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/gpio.h>

#define GPIO_NAME		"moxa-it87-gpio"
#define SIO_CHIP_ID_8786	0x8786
#define SIO_CHIP_ID_8783	0x8783
#define CHIP_ID_HIGH_BYTE	0x20
#define CHIP_ID_LOW_BYTE	0x21

#define GPIO_BA_HIGH_BYTE	0x62
#define GPIO_BA_LOW_BYTE	0x63
#define GPIO_IOSIZE		8

#define GPIO_GROUP_1		0
#define GPIO_GROUP_2		1
#define GPIO_GROUP_3		2
#define GPIO_GROUP_4		3
#define GPIO_GROUP_5		4
#define GPIO_GROUP_6		5
#define GPIO_GROUP_7		6
#define GPIO_GROUP_8		7
#define GPIO_BIT_0		(1<<0)
#define GPIO_BIT_1		(1<<1)
#define GPIO_BIT_2		(1<<2)
#define GPIO_BIT_3		(1<<3)
#define GPIO_BIT_4		(1<<4)
#define GPIO_BIT_5		(1<<5)
#define GPIO_BIT_6		(1<<6)
#define GPIO_BIT_7		(1<<7)

static u8 ports[1] = { 0x2e };
static u8 port;
 
static DEFINE_SPINLOCK(sio_lock);
static u16 gpio_ba;

static u8 read_reg(u8 addr, u8 port)
{
	outb(addr, port);
	return inb(port + 1);
}

static void write_reg(u8 data, u8 addr, u8 port)
{
	outb(addr, port);
	outb(data, port + 1);
}

static int enter_conf_mode(u8 port)
{
	/*
	 * Try to reserve REG and REG + 1 for exclusive access.
	 */
	if (!request_muxed_region(port, 2, GPIO_NAME))
		return -EBUSY;

	outb(0x87, port);
	outb(0x01, port);
	outb(0x55, port);
	outb((port == 0x2e) ? 0x55 : 0xaa, port);

	return 0;
}

static void exit_conf_mode(u8 port)
{
	outb(0x2, port);
	outb(0x2, port + 1);
	release_region(port, 2);
}

static void enter_gpio_mode(u8 port)
{
	write_reg(0x7, 0x7, port);
}

static void enter_uart_mode(u8 index, u8 port)
{
	u8 ldn = 0;
	if (index == 0) {
		ldn = 0x1;
	}
	else if (index == 1) {
		ldn = 0x2;
	}
	else if (index == 2) {
		ldn = 0x8;
	}
	else if (index == 3) {
		ldn = 0x9;
	}
	else if (index == 4) {
		ldn = 0xb;
	}
	else if (index == 5) {
		ldn = 0xc;
	}
	else {
		return ;
	}

	write_reg(ldn, 0x7, port);
}

void write_gpio(u8 *pindef, unsigned num, int val)
{
	u16 reg;
	u8 bit;
	u8 curr_vals;

	reg = gpio_ba + pindef[num*2];
	bit = pindef[num*2+1];

	spin_lock(&sio_lock);
	curr_vals = inb(reg);
	if (val)
		outb(curr_vals | bit, reg);
	else
		outb(curr_vals & ~bit, reg);
	spin_unlock(&sio_lock);
}

int read_gpio(u8 *pindef, unsigned num)
{
	u16 reg;
	u8 bit;

	reg = gpio_ba + pindef[num*2];
	bit = pindef[num*2+1];

	return !!(inb(reg) & bit);
}

int gpio_init(void)
{
	int err;

	int i, id;

	/* chip and port detection */
	for (i = 0; i < ARRAY_SIZE(ports); i++) {
		spin_lock(&sio_lock);
		err = enter_conf_mode(ports[i]);
		if (err) {
			spin_unlock(&sio_lock);
			return err;
		}

		id = (read_reg(CHIP_ID_HIGH_BYTE, ports[i]) << 8) +
			read_reg(CHIP_ID_LOW_BYTE, ports[i]);
		exit_conf_mode(ports[i]);
		spin_unlock(&sio_lock);

		switch ( id ) {
			case SIO_CHIP_ID_8783:
			case SIO_CHIP_ID_8786:
				port = ports[i];
			break;
		}
	}

	if (!port)
		return -ENODEV;

	/* fetch GPIO base address */
	spin_lock(&sio_lock);
	err = enter_conf_mode(port);
	if (err) {
		spin_unlock(&sio_lock);
		return err;
	}

	enter_gpio_mode(port);
	gpio_ba = (read_reg(GPIO_BA_HIGH_BYTE, port) << 8) +
			read_reg(GPIO_BA_LOW_BYTE, port);
	exit_conf_mode(port);
	spin_unlock(&sio_lock);

	if (!request_region(gpio_ba, GPIO_IOSIZE, GPIO_NAME)) {
		printk("request_region address 0x%x failed!\n", gpio_ba);
		gpio_ba = 0;
		return -EBUSY;
	}

	return 0;
}

void gpio_exit(void)
{
	if (gpio_ba) {
		release_region(gpio_ba, GPIO_IOSIZE);
		gpio_ba = 0;
	}
}

