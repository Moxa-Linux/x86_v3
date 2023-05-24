/*
 * This driver is for Moxa embedded computer di, do, power
 * indicator driver. It based on IT8783 GPIO hardware.
 *
 * History:
 * Date         Aurhor          Comment
 * 2016/07/19    Jared Wu        First create for MC-1100.
 * 2016/08/31    Jared Wu        Add di_ioctl(), do_ioctl() for compatible with old driver.
 * 2018/01/17    Jared Wu        Fix the warning message of type checking in compiling.
 * 2018/06/08    Jared Wu        Porting for MC-7200-DC-CP or MC-7200-MP.
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

#define DRIVER_VERSION		"1.0"
#define DI_NUM			8
#define DI_PATTERN		"11111111"
#define DI_ON			1
#define DI_OFF			0

#define DO_NUM			8
#define DO_PATTERN		"11111111"
#define DO_ON			1
#define DO_OFF			0

#define SIF_NUM			4	// 2
#define SIF_PATTERN		"0123"	// "12"

/* mknod /dev/di c 10 105 for this module */
#define MOXA_DI_MINOR		105
#define MOXA_SIF_MINOR		(MOXA_DI_MINOR+1)
#define MOXA_DO_MINOR		(MOXA_SIF_MINOR+1)
#define MOXA_MINIPCIE_RESET_MINOR	(MOXA_DO_MINOR+1)

#define DI_NAME                 "moxa-gpio-di"
#define SIF_NAME		"moxa-gpio-uart"
#define DO_NAME			"moxa-gpio-do"

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
 * Customization for MC-7200
 */

/* DI pin */
static u8 di_pin_def[] = {
	GPIO_GROUP_2, GPIO_BIT_0,	/* di1 */
	GPIO_GROUP_2, GPIO_BIT_1,	/* di2 */
	GPIO_GROUP_2, GPIO_BIT_2,	/* di3 */
	GPIO_GROUP_2, GPIO_BIT_3,	/* di4 */
	GPIO_GROUP_3, GPIO_BIT_0,	/* di5 */
	GPIO_GROUP_3, GPIO_BIT_1,	/* di6 */
	GPIO_GROUP_3, GPIO_BIT_2,	/* di7 */
	GPIO_GROUP_1, GPIO_BIT_4,	/* di8 */
};

/* DO pin */
static u8 do_pin_def[] = {
	GPIO_GROUP_2, GPIO_BIT_4,	/* do1 */
	GPIO_GROUP_2, GPIO_BIT_5,	/* do2 */
	GPIO_GROUP_2, GPIO_BIT_6,	/* do3 */
	GPIO_GROUP_1, GPIO_BIT_5,	/* do4 */
	GPIO_GROUP_6, GPIO_BIT_6,	/* do5 */
	GPIO_GROUP_5, GPIO_BIT_5,	/* do6 */
	GPIO_GROUP_3, GPIO_BIT_3,	/* do7 */
	GPIO_GROUP_3, GPIO_BIT_7,	/* do8 */
};

/* UART interface */
static u8 uartif_pin_def[] = {
	0xFF, 0xFF, /* UART1 RS232 only */
	0xFF, 0xFF,
	0xFF, 0xFF,
	0xFF, 0xFF,
	0xFF, 0xFF, /* UART2 RS232 only */
	0xFF, 0xFF,
	0xFF, 0xFF,
	0xFF, 0xFF,
	GPIO_GROUP_5, GPIO_BIT_1, /* UART3 RS232 */
	GPIO_GROUP_5, GPIO_BIT_0, /* UART3 RS485 2 Wire */
	GPIO_GROUP_5, GPIO_BIT_3, /* UART3 RS422/RS485 4 Wire */
	0xFF, 0xFF,
	GPIO_GROUP_5, GPIO_BIT_4, /* UART4 RS232 */
	GPIO_GROUP_5, GPIO_BIT_5, /* UART4 RS485 2 Wire */
	GPIO_GROUP_5, GPIO_BIT_2, /* UART4 RS422/RS485 4 Wire */
	0xFF, 0xFF,
};

int di_get(unsigned num, int *val)
{
	if (num >= (sizeof(di_pin_def)/2)) {
		return -EINVAL;
	}

	*val = read_gpio(di_pin_def, num);

	return 0;
}

int do_set(unsigned num, int val)
{
	if (num >= (sizeof(do_pin_def)/2)) {
		return -EINVAL;
	}

	write_gpio(do_pin_def, num, val);

	return 0;
}

int do_get(unsigned num, int *val)
{
	if (num >= (sizeof(do_pin_def)/2)) {
		return -EINVAL;
	}

	*val = read_gpio(do_pin_def, num);

	return 0;
}

int uartif_set(unsigned num, int val)
{
	u16 reg;
	u8 bit;
	u8 curr_vals;
	int i;

	if (num >= ((sizeof(uartif_pin_def)/2)/4)) {
		return -EINVAL;
	}

	if (val >= 4 || val < 0) {
		return -EINVAL;
	}

	/* Set output mode */
	for (i = 0; i < 4; i++) {
		reg = gpio_ba + uartif_pin_def[(num*4+i)*2];
		bit = uartif_pin_def[(num*4+i)*2+1];
		if ( reg==0xff && bit==0xff ) {
			continue;
		}

		spin_lock(&sio_lock);
		curr_vals = inb(reg);
		if (i==val)
			outb(curr_vals | bit, reg);
		else
			outb(curr_vals & ~bit, reg);
		spin_unlock(&sio_lock);
	}

	/* Switch on/off RS485DCE */
	spin_lock(&sio_lock);
	if (enter_conf_mode(port)) {
		spin_unlock(&sio_lock);
		return -EINVAL;
	}

	enter_uart_mode(num, port);
	curr_vals = read_reg(0xf0, port);
	if ( val == RS485_2WIRE_MODE) {
		curr_vals |= 0x80;
	} else {
		curr_vals &= ~0x80;
	}
	write_reg(curr_vals, 0xf0, port);
	exit_conf_mode(port);
	spin_unlock(&sio_lock);

	return 0;
}

int uartif_get(unsigned num, int *val)
{
	u16 reg;
	u8 bit;
	int i;

	if (num >= ((sizeof(uartif_pin_def)/2)/4)) {
		return -EINVAL;
	}

	/* Get output mode */
	for (i = 0; i < 4; i++) {
		reg = gpio_ba + uartif_pin_def[(num*4+i)*2];
		bit = uartif_pin_def[(num*4+i)*2+1];
		if ( reg==0xff && bit==0xff ) {
			continue;
		}

		if (read_gpio(uartif_pin_def, num*4+i)) {
			*val = i;
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * module: DI section
 */
static int di_open (struct inode *inode, struct file *file)
{
	return 0;
}

/* Write function
 * Note: use echo 1111 > /dev/do
 * The order is [realy1, do2, do3, realy4, ...\n]
 */
ssize_t di_write (struct file *filp, const char __user *buf, size_t count,
	loff_t *pos)
{
	printk("<1> %s[%d]\n", __FUNCTION__, __LINE__);

	return 0;
}

static int di_release (struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t di_read (struct file *filp, char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	int ret;
	char stack_buf[DI_NUM];

	for (i = 0; (i < sizeof(stack_buf)) && (i < count); i++) {
		if ( !di_get(i, &ret) ) {
			if ( ret ) {
				stack_buf[i] = '0' + DI_ON;
			} else {
				stack_buf[i] = '0' + DI_OFF;
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

long di_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {
	struct dio_set_struct   set;
	int i = 0;
	int di_state = 0;

	/* check data */
	if ( copy_from_user(&set, (struct dio_set_struct *)arg, sizeof(struct dio_set_struct)) == sizeof(struct dio_set_struct) )
		return -EFAULT;

	switch (cmd) {
		case IOCTL_GET_DIN :
			dp("get din io_number:%x\n",set.io_number);

			if (set.io_number == ALL_PORT) {
				set.mode_data = 0;
				for( i=0; i< DI_NUM; i++) {
					di_get(set.io_number, &di_state);
					set.mode_data |= (di_state<<i);
				}
				dp(KERN_DEBUG "all port: %x", set.mode_data);
			} else { 
				if ( set.io_number < 0 || set.io_number >= DI_NUM ) 
					return -EINVAL;

				if( di_get(set.io_number, &set.mode_data) < 0 )
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
struct file_operations di_fops = {
	.owner		= THIS_MODULE,
	.read		= di_read,
	.open		= di_open,
	.write		= di_write,
	.unlocked_ioctl	= di_ioctl,
	.release	= di_release,
};

/* register as misc driver */
static struct miscdevice di_miscdev = {
	.minor = MOXA_DI_MINOR,
	.name = DI_NAME,
	.fops = &di_fops,
};

/*
 * module: DO section
 */
static int do_open (struct inode *inode, struct file *file)
{
	return 0;
}

static int do_release (struct inode *inode, struct file *file)
{
	return 0;
}

/* Write function
 * Note: use echo 1111 > /dev/do
 * The order is [realy1, do2, do3, realy4, ...\n]
 */
ssize_t do_write (struct file *filp, const char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	char stack_buf[DO_NUM];

	/* check input value */
	if ( count != (sizeof(stack_buf)+1) ) {
		printk( KERN_ERR "Moxa do error! paramter should be %lu \
digits, like \"%s\" \n", sizeof(stack_buf), DO_PATTERN);
		return -EINVAL;
	}

	if ( copy_from_user(stack_buf, buf, count-1) ) {
		return 0;
	}

	for (i = 0; i < sizeof(stack_buf); i++) {
		if (stack_buf[i] == '1') {
			do_set(i, DO_ON);
		} else if (stack_buf[i] == '0') {
			do_set(i, DO_OFF);
		} else {
			printk("do: error, you input is %s", stack_buf);
			break;
		}
	}

	return count;
}

ssize_t do_read (struct file *filp, char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	int ret;
	char stack_buf[DO_NUM];

	for (i = 0; (i < sizeof(stack_buf)) && (i < count); i++) {
		if ( !do_get(i, &ret) ) {
			if ( ret ) {
				stack_buf[i] = '0' + DO_ON;
			} else {
				stack_buf[i] = '0' + DO_OFF;
			}
		} else {
			return -EINVAL;
		}
	}

	ret = copy_to_user((void*)buf, (void*)stack_buf, sizeof(stack_buf));
	if( ret < 0 ) {
		printk( KERN_ERR "Moxa do error! paramter should be %lu \
digits, like \"%s\" \n", sizeof(stack_buf), DO_PATTERN);
		return -ENOMEM;
	}

	return sizeof(stack_buf);
}

long do_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {
	struct dio_set_struct   set;
	int i = 0;
	int do_state = 0;

	/* check data */
	if ( copy_from_user(&set, (struct dio_set_struct *)arg, sizeof(struct dio_set_struct)) == sizeof(struct dio_set_struct) )
		return -EFAULT;

	switch (cmd) {
		case IOCTL_SET_DOUT :
			dp("set dio:%x\n",set.io_number);
			if (set.io_number < 0 || set.io_number >= DO_NUM)
				return -EINVAL;

			if ( set.mode_data != DIO_HIGH && set.mode_data != DIO_LOW )
				return -EINVAL;

			do_set(set.io_number, set.mode_data);

			break;

		case IOCTL_GET_DOUT :
			dp("get do io_number:%x\n",set.io_number);

			if (set.io_number == ALL_PORT) {
				set.mode_data = 0;
				for( i=0; i< DO_NUM; i++) {
					do_get(set.io_number, &do_state);
					set.mode_data |= (do_state<<i);
				}
				dp(KERN_DEBUG "all port: %x", set.mode_data);
			} else { 
				if ( set.io_number < 0 || set.io_number >= DO_NUM ) 
					return -EINVAL;

				if( do_get(set.io_number, &set.mode_data) < 0 )
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
struct file_operations do_fops = {
	.owner		= THIS_MODULE,
	.write		= do_write,
	.read		= do_read,
	.unlocked_ioctl	= do_ioctl,
	.open		= do_open,
	.release	= do_release,
};

/* register as misc driver */
static struct miscdevice do_miscdev = {
	.minor = MOXA_DO_MINOR,
	.name = DO_NAME,
	.fops = &do_fops,
};

/*
 * module: Serial interface section
 */
static int sif_open (struct inode *inode, struct file *file)
{
	return 0;
}

static int sif_release (struct inode *inode, struct file *file)
{
	return 0;
}

/* Write function
 * Note: use echo 1111 > /dev/uart
 * The order is [uart1, uart2, uart3, uart4, ...\n]
 */
ssize_t sif_write (struct file *filp, const char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	char stack_buf[SIF_NUM];

	/* check input value */
	if ( count != (sizeof(stack_buf)+1) ) {
		printk( KERN_ERR "Moxa uart error! paramter should be %lu digits\
, like \"%s\" \n", sizeof(stack_buf)/sizeof(char), SIF_PATTERN);
		return -EINVAL;
	}

	if ( copy_from_user(stack_buf, buf, count-1) ) {
		return 0;
	}

	for (i = 0; i < sizeof(stack_buf); i++) {
		if (stack_buf[i] >= '0' && stack_buf[i] <= '3') {
			if ( 0 != uartif_set( i, stack_buf[i]-'0') ) {
				printk("uart: the mode is not supported, \
the input is %s", stack_buf);
				break;
			}
		} else {
			printk("uart: error, the input is %s", stack_buf);
			break;
		}
	}

	return count;
}

ssize_t sif_read (struct file *filp, char __user *buf, size_t count,
	loff_t *pos)
{
	int i;
	int ret;
	char stack_buf[SIF_NUM];

	for (i = 0; (i < sizeof(stack_buf)) && (i < count); i++) {
		if ( !uartif_get(i, &ret) ) {
			stack_buf[i] = '0' + ret;
			dbg_printf("%s", stackbuf[i] );
		} else {
			return -EINVAL;
		}
	}

	ret = copy_to_user((void*)buf, (void*)stack_buf, sizeof(stack_buf));
	if( ret < 0 ) {
		printk( KERN_ERR "Moxa uart error! paramter should be %lu digits\
, like \"%s\" \n", sizeof(stack_buf), SIF_PATTERN);
		return -ENOMEM;
	}

	return sizeof(stack_buf);
}

// ioctl - I/O control
static long sif_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
{
	unsigned char port;
	unsigned char opmode;
	int val;

	switch ( cmd ) {
	case MOXA_SET_OP_MODE:
		if (copy_from_user(&opmode, (unsigned char *) arg, sizeof(opmode))) {
			return -EFAULT;
		}
		port = opmode >> 4 ;
		opmode = opmode & 0xf;
		if (0 != uartif_set(opmode >> 4, opmode & 0xf)) {
			printk("uart: the mode is not supported, \
the input is %x\n", opmode);
			return -EFAULT;
		}

		break;

	case MOXA_GET_OP_MODE:
		if ( copy_from_user(&port, (unsigned char *)arg, sizeof(port))){
			return -EFAULT;
		}
			
		if (0 != uartif_get(port, &val)) {
			return -EINVAL;
		}
		opmode = val;
		if ( copy_to_user((unsigned char*)arg, &opmode, sizeof(opmode)) ) {
			return -EFAULT;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* define which file operations are supported */
struct file_operations sif_fops = {
	.owner		= THIS_MODULE,
	.write		= sif_write,
	.read		= sif_read,
	.unlocked_ioctl	= sif_ioctl,
	.open		= sif_open,
	.release	= sif_release,
};

/* register as misc driver */
static struct miscdevice sif_miscdev = {
	.minor = MOXA_SIF_MINOR,
	.name = SIF_NAME,
	.fops = &sif_fops,
};


#ifdef CONFIG_GPIO_SYSFS
/*
 * Support sysfs
 */

static int moxa_gpio_do_get(struct gpio_chip *gc, unsigned gpio_num)
{
	int val;

	if (0==do_get(gpio_num, &val)) {
		if (DO_ON==val)
			return 1;
	}

	return 0;
}

static void moxa_gpio_do_set(struct gpio_chip *gc, unsigned gpio_num, int val)
{
	if ( val )
		val = DO_ON;
	else
		val = DO_OFF;
	do_set(gpio_num, val);
}

const char *gpio_do_names[] = {
	"do1",
	"do2",
	"do3",
	"do4",
	"do5",
	"do6",
	"do7",
	"do8",
};

static struct gpio_chip moxa_gpio_do_chip = {
	.label		= DO_NAME,
	.owner		= THIS_MODULE,
	.get		= moxa_gpio_do_get,
	.set		= moxa_gpio_do_set,
	.base		= -1,
	.ngpio		= sizeof(do_pin_def)/2,
	.names		= gpio_do_names
};

static int moxa_gpio_di_get(struct gpio_chip *gc, unsigned gpio_num)
{
	int val;

	if (0==di_get(gpio_num, &val)) {
		if (DI_ON==val)
			return 1;
	}

	return 0;
}

static void moxa_gpio_di_set(struct gpio_chip *gc, unsigned gpio_num, int val)
{
}

const char *gpio_di_names[] = {
	"di1",
	"di2",
	"di3",
	"di4",
	"di5", 
	"di6", 
	"di7", 
	"di8", 
};

static struct gpio_chip moxa_gpio_di_chip = {
	.label		= DI_NAME,
	.owner		= THIS_MODULE,
	.get		= moxa_gpio_di_get,
	.set		= moxa_gpio_di_set,
	.base		= -1,
	.ngpio		= sizeof(di_pin_def)/2,
	.names		= gpio_di_names
};

static int moxa_gpio_uartif_get(struct gpio_chip *gc, unsigned gpio_num)
{
	int val;

	if (0==uartif_get(gpio_num/3, &val)) {
		if (val==(gpio_num%3)) {
			return 1;
		}
	}

	return 0;
}

static void moxa_gpio_uartif_set(struct gpio_chip *gc, unsigned gpio_num, int val)
{
	if (val) {
		uartif_set(gpio_num/3, gpio_num%3);
	}
}

const char *gpio_uartif_names[] = {
	"uart1_232",
	"uart1_485",
	"uart1_422",
	"uart2_232",
	"uart2_485",
	"uart2_422",
	"uart3_232",
	"uart3_485",
	"uart3_422",
	"uart4_232",
	"uart4_485",
	"uart4_422",
};

static struct gpio_chip moxa_gpio_uartif_chip = {
	.label		= SIF_NAME,
	.owner		= THIS_MODULE,
	.get		= moxa_gpio_uartif_get,
	.set		= moxa_gpio_uartif_set,
	.base		= -1,
	.ngpio		= sizeof(gpio_uartif_names)/sizeof(char*),
	.names		= gpio_uartif_names
};
#endif /* CONFIG_GPIO_SYSFS */


/* initialize module (and interrupt) */
static int __init moxa_misc_init_module (void)
{
	int retval=0;

	printk(KERN_INFO "Initializing MOXA misc. device version %s\n", DRIVER_VERSION );

	// register misc driver
	retval = misc_register(&do_miscdev);
	if ( retval != 0 ) {
		printk("Moxa do driver: Register misc fail !\n");
		goto moxa_misc_init_module_err1;
	}

	retval = misc_register(&sif_miscdev);
	if ( retval != 0 ) {
		printk("Moxa uart interface driver: Register misc fail !\n");
		goto moxa_misc_init_module_err2;
	}

	retval = misc_register(&di_miscdev);
	if ( retval != 0 ) {
		printk("Moxa di driver: Register misc fail !\n");
		goto moxa_misc_init_module_err3;
	}

	retval = gpio_init();
	if ( retval != 0 ) {
		printk("Moxa uart interface driver: gpio_init() fail !\n");
		goto moxa_misc_init_module_err4;
	}

#ifdef CONFIG_GPIO_SYSFS
	retval = gpiochip_add(&moxa_gpio_do_chip);
	if ( retval < 0 ) {
		printk("Moxa uart interface driver: gpiochip_add(&moxa_gpio_do_chip) fail !\n");
		goto moxa_misc_init_module_err5;
	}
	printk("Moxa GPIO, %s, registerd at base number: %d\n", moxa_gpio_do_chip.label, moxa_gpio_do_chip.base);

	retval = gpiochip_add(&moxa_gpio_di_chip);
	if ( retval < 0 ) {
		printk("Moxa uart interface driver: gpiochip_add(&moxa_gpio_di_chip) fail !\n");
		goto moxa_misc_init_module_err6;
	}
	printk("Moxa GPIO, %s, registerd at base number: %d\n", moxa_gpio_di_chip.label, moxa_gpio_di_chip.base);

	retval = gpiochip_add(&moxa_gpio_uartif_chip);
	if ( retval < 0 ) {
		printk("Moxa uart interface driver: gpiochip_add(&moxa_gpio_uartif_chip) fail !\n");
		goto moxa_misc_init_module_err7;
	}
	printk("Moxa GPIO, %s, registered at base number:%d\n", moxa_gpio_uartif_chip.label, moxa_gpio_uartif_chip.base);

#endif /* CONFIG_GPIO_SYSFS */

	return 0;

#ifdef CONFIG_GPIO_SYSFS
moxa_misc_init_module_err7:
	gpiochip_remove(&moxa_gpio_di_chip);
moxa_misc_init_module_err6:
	gpiochip_remove(&moxa_gpio_do_chip);
#endif
moxa_misc_init_module_err5:
	gpio_exit();
moxa_misc_init_module_err4:
	misc_deregister(&di_miscdev);
moxa_misc_init_module_err3:
	misc_deregister(&sif_miscdev);
moxa_misc_init_module_err2:
	misc_deregister(&do_miscdev);
moxa_misc_init_module_err1:
	return retval;
}

/* close and cleanup module */
static void __exit moxa_misc_cleanup_module (void)
{
	printk("cleaning up module\n");
	misc_deregister(&di_miscdev);
	misc_deregister(&do_miscdev);
	misc_deregister(&sif_miscdev);
#ifdef CONFIG_GPIO_SYSFS
	gpiochip_remove(&moxa_gpio_do_chip);
	gpiochip_remove(&moxa_gpio_uartif_chip);
	gpiochip_remove(&moxa_gpio_di_chip);
#endif /* CONFIG_GPIO_SYSFS */
	gpio_exit();
}

module_init(moxa_misc_init_module);
module_exit(moxa_misc_cleanup_module);
MODULE_AUTHOR("jared.wu@moxa.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MOXA misc. device driver");

