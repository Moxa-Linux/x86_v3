/*
 * This driver is for Moxa embedded computer programmble led, relay, power
 * indicator driver. It based on IT87836 GPIO hardware.
 *
 * History:
 * Date         Aurhor          Comment
 * 2016/6/22    Holsety Chen    First create for DA-720.
 * 2016/7/19    Jared Wu        First create for MC-1100.
 * 2018/1/17    Jared Wu        Fix the warning message of type checking in compiling
 * 2020/12/22   Jared Wu        Create for DA-682B-TCC
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/io.h>

#include "dio_main.h"
#include "gpio-it87.c"

#define DRIVER_VERSION		"1.3"

#define LED_NUM			4
#define LED_PATTERN		"1111"
#define LED_ON			0
#define LED_OFF			1

#define RELAY_NUM		2
#define RELAY_PATTERN		"11"
#define RELAY_ON		1
#define RELAY_OFF		0

#define PWRIND_NUM		2
#define PWRIND_PATTERN		"11"
#define PWRIND_ON		1
#define PWRIND_OFF		0

#define SIF_NUM			2
#define SIF_PATTERN		"12"

/* mknod /dev/pled c 10 105 for this module */
#define MOXA_PLED_MINOR		105
#define MOXA_SIF_MINOR		(MOXA_PLED_MINOR+1)
#define MOXA_RELAY_MINOR	(MOXA_SIF_MINOR+1)
#define MOXA_PWRIND_MINOR	(MOXA_RELAY_MINOR+1)

#define PLED_NAME		"pled"
#define RELAY_NAME		"relay"

/* Ioctl number */
#define MOXA			0x400
#define MOXA_SET_OP_MODE	(MOXA + 66)
#define MOXA_GET_OP_MODE	(MOXA + 67)

/* Debug message */
#ifdef DEBUG
#define dbg_printf(x...)	printk(x)
#else
#define dbg_printf(x...)
#endif

#define RS232_MODE		0
#define RS485_2WIRE_MODE	1
#define RS422_MODE		2
#define RS485_4WIRE_MODE	3

/*
 * Customization for DA-682B-TCC
 */

/* Relay output pin, 0x306, bit 6 ~ 7 */
static u8 relay_pin_def[] = {
	GPIO_GROUP_7, GPIO_BIT_6,
	GPIO_GROUP_7, GPIO_BIT_7,
};

/* Programable LED, 0x306, bit 0 ~ 3 */
static u8 pled_pin_def[] = {
	GPIO_GROUP_7, GPIO_BIT_0,
	GPIO_GROUP_7, GPIO_BIT_1,
	GPIO_GROUP_7, GPIO_BIT_2,
	GPIO_GROUP_7, GPIO_BIT_3,
};

int pled_set(unsigned num, int val)
{
	if (num >= (sizeof(pled_pin_def)/2)) {
		return -EINVAL;
	}

	write_gpio(pled_pin_def, num, val);

	return 0;
}

int pled_get(unsigned num, int *val)
{
	if (num >= (sizeof(pled_pin_def)/2)) {
		return -EINVAL;
	}

	*val = read_gpio(pled_pin_def, num);

	return 0;
}

/*
 * module: Programable LED section
 */
static int pled_open (struct inode *inode, struct file *file)
{
	return 0;
}

static int pled_release (struct inode *inode, struct file *file)
{
	return 0;
}

/* Write function
 * Note: use echo 1111 > /dev/pled
 * The order is [pled1, pled2, pled3, pled4, ...\n]
 */
ssize_t pled_write (struct file *filp, const char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	char stack_buf[LED_NUM];

	/* check input value */
	if ( count != (sizeof(stack_buf)+1) ) {
		printk( KERN_ERR "Moxa pled error! paramter should be %lu digits\
, like \"%s\" \n", sizeof(stack_buf)/sizeof(char), LED_PATTERN);
		return -EINVAL;
	}

	if ( copy_from_user(stack_buf, buf, count-1) ) {
		return 0;
	}

	for (i = 0; i < sizeof(stack_buf); i++) {
		if (stack_buf[i] == '1') {
			pled_set(i, LED_ON);
		} else if (stack_buf[i] == '0') {
			pled_set(i, LED_OFF);
		} else {
			printk("pled: error, the input is %s", stack_buf);
			break;
		}
	}

	return count;
}

ssize_t pled_read (struct file *filp, char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	int ret;
	char stack_buf[LED_NUM];

	for (i = 0; (i < sizeof(stack_buf)) && (i < count); i++) {
		if ( !pled_get(i, &ret) ) {
			if ( ret ) {
				stack_buf[i] = '0' + LED_ON;
			} else {
				stack_buf[i] = '0' + LED_OFF;
			}
		} else {
			return -EINVAL;
		}
	}

	ret = copy_to_user((void*)buf, (void*)stack_buf, i);
	if( ret < 0 ) {
		return -ENOMEM;
	}

	return i;
}

/* define which file operations are supported */
struct file_operations pled_fops = {
	.owner		= THIS_MODULE,
	.write		= pled_write,
	.read		= pled_read,
	.open		= pled_open,
	.release	= pled_release,
};

/* register as misc driver */
static struct miscdevice pled_miscdev = {
	.minor = MOXA_PLED_MINOR,
	.name = PLED_NAME,
	.fops = &pled_fops,
};

int relay_set(unsigned num, int val)
{
	if (num >= (sizeof(relay_pin_def)/2)) {
		return -EINVAL;
	}

	write_gpio(relay_pin_def, num, val);

	return 0;
}

int relay_get(unsigned num, int *val)
{
	if (num >= (sizeof(relay_pin_def)/2)) {
		return -EINVAL;
	}

	*val = read_gpio(relay_pin_def, num);

	return 0;
}

/*
 * module: Relay output section
 */
static int relay_open (struct inode *inode, struct file *file)
{
	return 0;
}

static int relay_release (struct inode *inode, struct file *file)
{
	return 0;
}

/* Write function
 * Note: use echo 11 > /dev/relay
 * The order is [realy1, relay2, relay3, realy4, ...\n]
 */
ssize_t relay_write (struct file *filp, const char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	char stack_buf[RELAY_NUM];

	/* check input value */
	if ( count != (sizeof(stack_buf)+1) ) {
		printk( KERN_ERR "Moxa relay error! paramter should be %lu \
digits, like \"%s\" \n", sizeof(stack_buf), RELAY_PATTERN);
		return -EINVAL;
	}

	if ( copy_from_user(stack_buf, buf, count-1) ) {
		return 0;
	}

	for (i = 0; i < sizeof(stack_buf); i++) {
		if (stack_buf[i] == '1') {
			relay_set(i, RELAY_ON);
		} else if (stack_buf[i] == '0') {
			relay_set(i, RELAY_OFF);
		} else {
			printk("relay: error, you input is %s", stack_buf);
			break;
		}
	}

	return count;
}

ssize_t relay_read (struct file *filp, char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	int ret;
	char stack_buf[RELAY_NUM];

	for (i = 0; (i < sizeof(stack_buf)) && (i < count); i++) {
		if ( !relay_get(i, &ret) ) {
			if ( ret ) {
				stack_buf[i] = '0' + RELAY_ON;
			} else {
				stack_buf[i] = '0' + RELAY_OFF;
			}
		} else {
			return -EINVAL;
		}
	}

	ret = copy_to_user((void*)buf, (void*)stack_buf, sizeof(stack_buf));
	if( ret < 0 ) {
		printk( KERN_ERR "Moxa relay error! paramter should be %lu \
digits, like \"%s\" \n", sizeof(stack_buf), RELAY_PATTERN);
		return -ENOMEM;
	}

	return sizeof(stack_buf);
}

long relay_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {
	struct dio_set_struct   set;
	int i = 0;
	int do_state = 0;

	/* check data */
	if ( copy_from_user(&set, (struct dio_set_struct *)arg, sizeof(struct dio_set_struct)) == sizeof(struct dio_set_struct) )
		return -EFAULT;

	switch (cmd) {
		case IOCTL_SET_DOUT :
			dp("set dio:%x\n",set.io_number);
			if (set.io_number < 0 || set.io_number >= RELAY_NUM)
				return -EINVAL;

			if ( set.mode_data != DIO_HIGH && set.mode_data != DIO_LOW )
				return -EINVAL;

			relay_set(set.io_number, set.mode_data);

			break;

		case IOCTL_GET_DOUT :
			dp("get do io_number:%x\n",set.io_number);

			if (set.io_number == ALL_PORT) {
				set.mode_data = 0;
				for( i=0; i< RELAY_NUM; i++) {
					relay_get(set.io_number, &do_state);
					set.mode_data |= (do_state<<i);
				}
				dp(KERN_DEBUG "all port: %x", set.mode_data);
			} else { 
				if ( set.io_number < 0 || set.io_number >= RELAY_NUM ) 
					return -EINVAL;

				if( relay_get(set.io_number, &set.mode_data) < 0 )
					printk(KERN_ALERT "di_get(%d, %d) fail\n", set.io_number, set.mode_data);
			}

			if ( copy_to_user((struct dio_set_struct *)arg, &set, sizeof(struct dio_set_struct)) == sizeof(struct dio_set_struct) )
				return -EFAULT;

			dp("mode_data: %x\n", (unsigned int)set.mode_data);

			break;

		default :
			printk(KERN_DEBUG "ioctl:error\n");
			return -EINVAL;
	}

	return 0; /* if switch ok */
}


/* define which file operations are supported */
struct file_operations relay_fops = {
	.owner		= THIS_MODULE,
	.write		= relay_write,
	.read		= relay_read,
	.unlocked_ioctl	= relay_ioctl,
	.open		= relay_open,
	.release	= relay_release,
};

/* register as misc driver */
static struct miscdevice relay_miscdev = {
	.minor = MOXA_RELAY_MINOR,
	.name = RELAY_NAME,
	.fops = &relay_fops,
};

#ifdef CONFIG_GPIO_SYSFS
/*
 * Support sysfs
 */

static int moxa_gpio_relay_get(struct gpio_chip *gc, unsigned gpio_num)
{
	int val;

	if (0==relay_get(gpio_num, &val)) {
		if (RELAY_ON==val)
			return 1;
	}

	return 0;
}

static void moxa_gpio_relay_set(struct gpio_chip *gc, unsigned gpio_num, int val)
{
	if ( val )
		val = RELAY_ON;
	else
		val = RELAY_OFF;
	relay_set(gpio_num, val);
}

const char *gpio_relay_names[] = {
	"relay1",
	"relay2",
};

static struct gpio_chip moxa_gpio_relay_chip = {
	.label		= "moxa-gpio-relay",
	.owner		= THIS_MODULE,
	.get		= moxa_gpio_relay_get,
	.set		= moxa_gpio_relay_set,
	.base		= -1,
	.ngpio		= sizeof(relay_pin_def)/2,
	.names		= gpio_relay_names
};


static int moxa_gpio_pled_get(struct gpio_chip *gc, unsigned gpio_num)
{
	int val;

	if (0==pled_get(gpio_num, &val)) {
		if (LED_ON==val)
			return 1;
	}

	return 0;
}

static void moxa_gpio_pled_set(struct gpio_chip *gc, unsigned gpio_num, int val)
{
	if ( val )
		val = LED_ON;
	else
		val = LED_OFF;
	pled_set(gpio_num, val);
}

const char *gpio_pled_names[] = {
	"pled1",
	"pled2",
	"pled3",
	"pled4",
};

static struct gpio_chip moxa_gpio_pled_chip = {
	.label		= "moxa-gpio-pled",
	.owner		= THIS_MODULE,
	.get		= moxa_gpio_pled_get,
	.set		= moxa_gpio_pled_set,
	.base		= -1,
	.ngpio		= sizeof(pled_pin_def)/2,
	.names		= gpio_pled_names
};

#endif /* CONFIG_GPIO_SYSFS */


/* initialize module (and interrupt) */
static int __init moxa_misc_init_module (void)
{
	int retval=0;

	printk(KERN_INFO "Initializing MOXA misc. device version %s\n", DRIVER_VERSION );

	// register misc driver
	retval = misc_register(&pled_miscdev);
	if ( retval != 0 ) {
		printk("Moxa pled driver: Register misc fail !\n");
		goto moxa_misc_init_module_err1;
	}

	retval = misc_register(&relay_miscdev);
	if ( retval != 0 ) {
		printk("Moxa relay driver: Register misc fail !\n");
		goto moxa_misc_init_module_err2;
	}

	retval = gpio_init();
	if ( retval != 0 ) {
		printk("Moxa misc_moxa : gpio_init() fail !\n");
		goto moxa_misc_init_module_err3;
	}

#ifdef CONFIG_GPIO_SYSFS
	retval = gpiochip_add(&moxa_gpio_relay_chip);
	if ( retval < 0 ) {
		printk("Moxa uart interface driver: gpiochip_add(&moxa_gpio_relay_chip) fail !\n");
		goto moxa_misc_init_module_err4;
	}
	retval = gpiochip_add(&moxa_gpio_pled_chip);
	if ( retval < 0 ) {
		printk("Moxa uart interface driver: gpiochip_add(&moxa_gpio_pled_chip) fail !\n");
		goto moxa_misc_init_module_err5;
	}
#endif /* CONFIG_GPIO_SYSFS */

	return 0;

#ifdef CONFIG_GPIO_SYSFS
moxa_misc_init_module_err5:
	gpiochip_remove(&moxa_gpio_relay_chip);
#endif
moxa_misc_init_module_err4:
	gpio_exit();
moxa_misc_init_module_err3:
	misc_deregister(&relay_miscdev);
moxa_misc_init_module_err2:
	misc_deregister(&pled_miscdev);
moxa_misc_init_module_err1:
	return retval;
}

/* close and cleanup module */
static void __exit moxa_misc_cleanup_module (void)
{
	printk("cleaning up module\n");
	misc_deregister(&pled_miscdev);
	misc_deregister(&relay_miscdev);
#ifdef CONFIG_GPIO_SYSFS
	gpiochip_remove(&moxa_gpio_relay_chip);
	gpiochip_remove(&moxa_gpio_pled_chip);
#endif /* CONFIG_GPIO_SYSFS */
	gpio_exit();
}

module_init(moxa_misc_init_module);
module_exit(moxa_misc_cleanup_module);
MODULE_AUTHOR("holsety@moxa.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MOXA misc. device driver");

