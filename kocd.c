#include <linux/fs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DEVICE_NAME		"kocd"
#define DEVICE_CLASS_NAME	"kocd_class"
#define DEVICE_PROC_FS_NAME	"kocd"
#define LOG_PREFIX		"kocd: "

#define RST_BUF_CMD		0xF0
#define RST_BUF_WR_POS_CMD	0xF1
#define RST_BUF_RD_POS_CMD	0xF2

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maciej Gorzkowski");
MODULE_DESCRIPTION("Driver for " DEVICE_NAME);
MODULE_VERSION("1.0");

static int dev_open(struct inode *inode, struct file *fd);
static int dev_release(struct inode *inode, struct file *fd);
static ssize_t dev_read(struct file *fd, char *buffer,
		size_t length, loff_t *offset);
static ssize_t dev_write(struct file *fd, const char *buffer,
		size_t length, loff_t *offset);
static long dev_ioctl(struct file *fd, unsigned int ioctl_num,
		unsigned long ioctl_param);
static loff_t dev_llseek(struct file *fd, loff_t offset, int whence);
static int proc_open(struct inode *inode, struct file *fd);
static int proc_release(struct inode *inode, struct file *fd);
static ssize_t proc_read(struct file *filed, char *buffer,
		size_t length, loff_t *offset);

// change this fucntion
static void printk_dev_buf(void);
static void reset_buf_wr_pos(void);
static void reset_buf_rd_pos(void);
static void reset_buf(void);

static char *dev_buf = NULL;
static int dev_buf_size = 0;
static int dev_buf_wr_pos = 0;
static int dev_buf_rd_pos = 0;

static int dev_major = 0;
static int size = 0;
static struct class *dev_class = NULL;
static struct device *dev = NULL;
static struct proc_dir_entry *dev_proc_file = NULL;

struct file_operations dev_fops = {
	llseek: dev_llseek,
	read: dev_read,
	write: dev_write,
	open: dev_open,
	release: dev_release,
	unlocked_ioctl: dev_ioctl,
};

struct file_operations proc_fops = {
	read: proc_read,
	open: proc_open,
	release: proc_release,
};

module_param(size, int, 0);
MODULE_PARM_DESC(size, "Buffer size of " DEVICE_NAME);

static int __init dev_init(void)
{
	dev_major = register_chrdev(0, DEVICE_NAME, &dev_fops);
	if (dev_major < 0) {
		printk(KERN_INFO LOG_PREFIX "Cannot register device\n");
		return -1;
	}
	dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(dev_class)) {
		printk(KERN_INFO LOG_PREFIX "Cannot register device class\n");
		unregister_chrdev(dev_major, DEVICE_NAME);
		return -2;
	}
	dev = device_create(dev_class, NULL, MKDEV(dev_major, 0),
			NULL, DEVICE_NAME);
	if (IS_ERR(dev)) {
		printk(KERN_INFO LOG_PREFIX "Cannot create device /dev/%s\n",
			DEVICE_NAME);
		class_destroy(dev_class);
		unregister_chrdev(dev_major, DEVICE_NAME);
		return -3;
	}
	dev_proc_file = proc_create(DEVICE_PROC_FS_NAME,
				0666, NULL, &proc_fops);
	if (NULL == dev_proc_file) {
		remove_proc_entry(DEVICE_PROC_FS_NAME, NULL);
		printk(KERN_INFO LOG_PREFIX "Cannot create device /dev/%s\n",
			DEVICE_PROC_FS_NAME);
		class_destroy(dev_class);
		unregister_chrdev(dev_major, DEVICE_NAME);
		return -4;
	}

	dev_buf_wr_pos = 0;
	dev_buf_rd_pos = 0;
	dev_buf_size = size;
	dev_buf = kmalloc(dev_buf_size+1, 0);
	memset(dev_buf, 0, dev_buf_size+1);
	printk(KERN_INFO LOG_PREFIX "Initialization of /dev/%s device\n",
		DEVICE_NAME);
	printk(KERN_INFO LOG_PREFIX "dev_buf_size = %d\n", dev_buf_size);
	printk(KERN_INFO LOG_PREFIX "major = %d\n", dev_major);
	printk_dev_buf();
	return 0;
}

static void __exit dev_exit(void)
{
	device_destroy(dev_class, MKDEV(dev_major, 0));
	class_unregister(dev_class);
	class_destroy(dev_class);
	unregister_chrdev(dev_major, DEVICE_NAME);
	remove_proc_entry(DEVICE_PROC_FS_NAME, NULL);
	kfree(dev_buf);
	printk(KERN_INFO LOG_PREFIX "exit\n");
}

module_init(dev_init);
module_exit(dev_exit);

/*
 * /dev file system functions
 */

static int dev_open(struct inode *inode, struct file *fd)
{
	printk(KERN_INFO LOG_PREFIX "open\n");
	return 0;
}

static int dev_release(struct inode *inode, struct file *fd)
{
	printk(KERN_INFO LOG_PREFIX "close\n");
	return 0;
}

static ssize_t dev_read(struct file *filed, char *buffer,
		size_t length, loff_t *offset)
{
	int to_read = (dev_buf_wr_pos-dev_buf_rd_pos>=length)
			? length : dev_buf_wr_pos-dev_buf_rd_pos;
	printk(KERN_INFO LOG_PREFIX "reading %d bytes\n", to_read);
	if (copy_to_user(buffer, dev_buf+dev_buf_rd_pos, to_read)) {
		return -EFAULT;
	}
	dev_buf_rd_pos += to_read;
	printk_dev_buf();
	return to_read;
}

static ssize_t dev_write(struct file *filed, const char *buffer,
		size_t length, loff_t *offset)
{
	int free_space_length = dev_buf_size - dev_buf_wr_pos;
	int to_write = (free_space_length>length) ? length : free_space_length;
	printk(KERN_INFO LOG_PREFIX "writing %d bytes\n", to_write);
	memcpy(dev_buf+dev_buf_wr_pos, buffer, to_write);
	dev_buf_wr_pos += to_write;
	printk_dev_buf();
	return to_write;
}

static long dev_ioctl(struct file *file, unsigned int ioctl_num,
		unsigned long ioctl_param)
{
	switch(ioctl_num) {
	case RST_BUF_CMD:
		reset_buf();
		break;
	case RST_BUF_WR_POS_CMD:
		reset_buf_wr_pos();
		break;
	case RST_BUF_RD_POS_CMD:
		reset_buf_rd_pos();
		break;
	default:
		return -1;
	}
	return 0;
}

static loff_t dev_llseek(struct file *fd, loff_t offset, int whence)
{
	int new_val = 0;
	switch(whence) {
	case 0:
		new_val = offset;
		break;
	case 1:
		new_val = dev_buf_rd_pos + offset;
		break;
	case 2:
		new_val = dev_buf_size - offset;
		break;
	default:
		return -1;
	}
	if (new_val<0 || new_val>dev_buf_size) {
		return -1;
	}
	dev_buf_rd_pos = new_val;
	return 0;
}

/*
 * /proc file system functions
 */

static int proc_msg_pos = 0;

static int proc_open(struct inode *inode, struct file *fd)
{
	printk(KERN_INFO LOG_PREFIX "procfs open\n");
	proc_msg_pos = 0;
	return 0;
}

static int proc_release(struct inode *inode, struct file *fd)
{
	printk(KERN_INFO LOG_PREFIX "procfs release\n");
	return 0;
}

static ssize_t proc_read(struct file *filed, char *buffer,
		size_t length, loff_t *offset)
{
	static char proc_msg[1024];
	int proc_msg_length;
	int to_read;
	sprintf(proc_msg, "buffer = [%s]\nwr_pos = %d\nrd_pos = %d\n",
		dev_buf, dev_buf_wr_pos, dev_buf_rd_pos);
	proc_msg_length = strlen(proc_msg);
	to_read = (proc_msg_length-proc_msg_pos>=length)
		? length : (proc_msg_length-proc_msg_pos);
	printk(KERN_INFO LOG_PREFIX "procfs readed %d from %d\n",
		to_read, proc_msg_length);
	if (proc_msg_pos == proc_msg_length) {
		return 0;
	} else {
		if (copy_to_user(buffer, proc_msg, to_read)) {
			printk(KERN_INFO LOG_PREFIX "procfs read error\n");
			return -EFAULT;
		}
		proc_msg_pos += to_read;
	}
	return to_read;
}

/*
 * Other functions
 */

static void printk_dev_buf(void)
{
#define BYTES_IN_LINE	16
#ifdef TAINT
	int i = 0, j = 0;
	int lines = dev_buf_size / BYTES_IN_LINE;
	char line[3*BYTES_IN_LINE] = {};
	char bytestr[4] = {};
	for (i=0; i<lines; i++) {
		for (j=0; j<BYTES_IN_LINE; j++) {
			sprintf(bytestr, "%x ", (char)dev_buf[i*BYTES_IN_LINE+j]);
			strncat(line, bytestr, 3);
		}
		line[3*BYTES_IN_LINE-1] = 0;
		printk(KERN_INFO LOG_PREFIX "%s\n", line);
	}
#else
	printk(KERN_INFO LOG_PREFIX "[%s]\n", dev_buf);
#endif


}

static void reset_buf_wr_pos(void)
{
	dev_buf_wr_pos = 0;
}

static void reset_buf_rd_pos(void)
{
	dev_buf_rd_pos = 0;
}

static void reset_buf(void)
{
	memset(dev_buf, 0, dev_buf_size+1);
	reset_buf_wr_pos();
	reset_buf_rd_pos();
}