

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h> /* error */
#include <linux/types.h> /* dev_t, atomic_t */
#include <linux/kdev_t.h> /* MAJOR(12bits),MINOR(20bits) */
#include <linux/fs.h> /* alloc_ch##() */
#include <linux/printk.h>
#include <linux/cdev.h> /* cdev_add */
#include <linux/device.h> /* sysfs class_create() */
#include <linux/err.h>
#include <linux/slab.h> /* kmalloc */
#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/uaccess.h> /* copy_ * _user */

static DEFINE_SPINLOCK(ssd_state_lock);
static int major_number;
struct class *ssd_class;
static int open_cnt;
static int write_mode;
static int open_mode;
static int reader_cnt;
#define DRIVER_NAME "test"
#define MAX_DEV 1
#define SSD_WRITE 2
#define SSD_EXCL 3


/* Per device structure */
struct ssd_dev {
	struct cdev cdev; /* link sysfs to file operations */
} *ssd_devp;
static char *ptr="Hello tiny people to read";

ssize_t ssd_write(struct file *filp, const char __user *buf,
					size_t count, loff_t *offset)
{
	char *ptr;
	ptr = kmalloc(sizeof(count),GFP_KERNEL|__GFP_NORETRY);
	if(copy_from_user(ptr,buf,count)) {
		kfree(ptr);
		return -EFAULT;
	}
	ptr[count]='\0';
	pr_debug("User msg %s\n",ptr);
	kfree(ptr);
	return count;
}

ssize_t ssd_read(struct file *filp, char __user *buf,
					size_t count, loff_t *offset)
{
	int rc = strlen(ptr);
	if(copy_to_user(buf,ptr,rc))
		rc = -EFAULT;

	return rc;
}
int ssd_open(struct inode *inode, struct file *file)
{
	spin_lock(&ssd_state_lock);
//	struct ssd_dev *ssd_devp;
	if ((open_cnt && (file->f_mode & O_EXCL)) || 
		((file->f_mode & FMODE_WRITE) && (open_mode & SSD_WRITE))) {
		spin_unlock(&ssd_state_lock);
		return -EBUSY;
	}
//	ssd_devp = container_of(inode->i_cdev, ssd_devp, cdev);
//	file->private_data = ssd_devp;
	if (file->f_mode & FMODE_READ) {
		reader_cnt++;
		pr_debug("Reader count %d\n", reader_cnt);
	}
	if (file->f_mode & FMODE_WRITE)
		open_mode |= SSD_WRITE;
	open_cnt++;
	spin_unlock(&ssd_state_lock);
return 0;
}

int ssd_release(struct inode *inode, struct file *file)
{
	spin_lock(&ssd_state_lock);
	open_cnt--;

	if (file->f_mode & FMODE_READ)
		reader_cnt--;
	if (file->f_mode & FMODE_WRITE)
		open_mode &= ~SSD_WRITE;
	spin_unlock(&ssd_state_lock);
return 0;
}

/* File operation structure */
static const struct file_operations ssd_fops = {
	.owner = THIS_MODULE,
	.open = ssd_open,	/* open method */
	.read = ssd_read,	/* read  method */
	.write = ssd_write,	/* write method */
	.release = ssd_release  /* release method */
};

static int __init ssd_init(void)
{
	dev_t devt;
	int ret = -ENODEV;
	struct device *dev;

	pr_debug("char skeletan\n");
	/* Request dynamic registration of device major number */
	ret = alloc_chrdev_region(&devt, 0, MAX_DEV, DRIVER_NAME);
	if (ret < 0) {
		pr_warn("Failed to create region : ssd. Aborting\n");
		goto out_err;
	}
	/* Populate sysfs entery in  /sys/class as DRIVER_NAME */
	ssd_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(ssd_class)) {
		ret = PTR_ERR(ssd_class);
		pr_warn("Failed to create class : ssd. Aborting\n");
		goto unroll_chrdev_region;
	}
	major_number = MAJOR(devt);
	/* Allocate memory for the per-device structure */
	ssd_devp = kmalloc(sizeof(struct ssd_dev), GFP_KERNEL);
	if (!ssd_devp) {
		ret = -ENOMEM;
		goto unroll_class;
	}
	/* making its ready with add to the system with cdev_add() */
	cdev_init(&ssd_devp->cdev, &ssd_fops);
	ssd_devp->cdev.owner = THIS_MODULE;
	ssd_devp->cdev.ops = &ssd_fops;
	/* add device to system and live immediatly */
	ret = cdev_add(&ssd_devp->cdev, devt, MAX_DEV);
	if (ret < 0) {
		pr_warn("Failed to add cdev : ssd. Aborting\n");
		goto unroll_per_device;
	}
	/* register device with sysfs class */
	dev = device_create(ssd_class, NULL, devt, NULL, DRIVER_NAME"%d", 0);
	if (IS_ERR(dev)) {
		pr_warn("Failed to create %s device. Aborting\n", DRIVER_NAME);
		ret = -ENODEV;
		goto unroll_chrdev;
	}
	pr_debug("ssd_init() success with major number %d\n", major_number);
	return 0;
unroll_chrdev:
	cdev_del(&ssd_devp->cdev);
unroll_per_device:
	kfree(ssd_devp);
unroll_class:
	class_destroy(ssd_class);
unroll_chrdev_region:
	unregister_chrdev_region(MKDEV(major_number, 0), MAX_DEV);
out_err:
	return ret;
}

static void __exit ssd_exit(void)
{
	if (ssd_class) {
		device_destroy(ssd_class, MKDEV(major_number, 0));
		class_destroy(ssd_class);
		
		pr_debug("ssd : device and class is cleanup!!\n");
	}
	if (ssd_devp) {
		cdev_del(&ssd_devp->cdev);
		kfree(ssd_devp);
		pr_debug("ssd : cleanup per device structure!!\n");
	}
	/* making dev_t from major and minor */
	if (major_number)
		unregister_chrdev_region(MKDEV(major_number, 0), MAX_DEV);
	pr_debug("char skeletan cleaned up!!\n");
}
module_init(ssd_init);
module_exit(ssd_exit);

MODULE_AUTHOR("Pintu Sakariya <sakariyapin2@gmail.com");
MODULE_DESCRIPTION("general char driver skeletal");
MODULE_LICENSE("GPL");
