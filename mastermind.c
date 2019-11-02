#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/switch_to.h>		/* cli(), *_flags */
#include <linux/uaccess.h>	/* copy_*_user */

/* 
    TODO: 
    * Check initialization grep does not give major number
    * Check static init, dynamic init
*/

//  check if linux/uaccess.h is required for copy_*_user
//instead of asm/uaccess
//required after linux kernel 4.1+ ?
#ifndef __ASM_ASM_UACCESS_H
    #include <linux/uaccess.h>
#endif

// BOOK: CHAR DEVICES CHAPTER

#include "mastermind_ioctl.h"

#define MMIND_MAJOR 0
#define MMIND_NR_DEVS 1
#define MMIND_NUMBER "1234"           // secret number
#define MMIND_LINE_SIZE 16          // FIXED 
#define MMIND_MAX_GUESSES 10        // DEFAULT: 10 
#define MMIND_MAX_LINES 256         // FIXED 

int mmind_major = MMIND_MAJOR;
int mmind_minor = 0;
char* mmind_number = MMIND_NUMBER;
int mmind_max_guesses = MMIND_MAX_GUESSES;
int mmind_line_size = 16;
int mmind_nrof_lines = MMIND_MAX_LINES;
int mmind_nr_devs = MMIND_NR_DEVS;

module_param(mmind_major, int, S_IRUGO);
module_param(mmind_minor, int, S_IRUGO);
module_param(mmind_number, charp, S_IRUGO);
module_param(mmind_max_guesses, int, S_IRUGO);

MODULE_AUTHOR("iee");
MODULE_LICENSE("Dual BSD/GPL");

struct mmind_dev {
    char **data;			 // Lines
    unsigned int line_size;	 // Size of lines (quantum)
    unsigned int nrof_lines; // (qset)
    unsigned long size;		 // size ((capacity)
    struct semaphore sem;	 // 
    struct cdev cdev;		 // char device 
};

struct mmind_dev *mmind_devices;

int mmind_trim(struct mmind_dev *dev)
{
    int i;

    if (dev->data) {
        for (i = 0; i < dev->nrof_lines; i++) {
            if (dev->data[i])
                kfree(dev->data[i]);
        }
        kfree(dev->data);
    }
    dev->data = NULL;
    dev->line_size = mmind_line_size;
    dev->nrof_lines = mmind_nrof_lines;
    dev->size = 0;
    return 0;
}


int mmind_open(struct inode *inode, struct file *filp)
{
    struct mmind_dev *dev;

    dev = container_of(inode->i_cdev, struct mmind_dev, cdev);
    filp->private_data = dev;

    /* trim the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        mmind_trim(dev);
        up(&dev->sem);
    }
    return 0;
}


int mmind_release(struct inode *inode, struct file *filp) // TODO: End game
{
    return 0;
}


ssize_t mmind_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct mmind_dev *dev = filp->private_data;
    int line_size = dev->line_size;
    int s_pos; // not required, q_pos;
    ssize_t retval = 0;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    if (*f_pos >= dev->size)
        goto out;
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    s_pos = (long) *f_pos / line_size;
    // q_pos = (long) *f_pos % quantum;

    if (dev->data == NULL || ! dev->data[s_pos])
        goto out;

    // Not required, quantum is atomic
    /* read only up to the end of this quantum */
    // if (count > quantum - q_pos)
    //     count = quantum - q_pos;

    if (copy_to_user(buf, dev->data[s_pos], count)) {  // q_pos 0
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

  out:
    up(&dev->sem);
    return retval;
}


ssize_t mmind_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct mmind_dev *dev = filp->private_data;
    int line_size = dev->line_size /* FIXED 16 */, nrof_lines = dev->nrof_lines /* FIXED 256*/;
    int s_pos; // No qpos, fixed quantum size 
    ssize_t retval = -ENOMEM;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if (*f_pos >= line_size * nrof_lines) {
        retval = 0;
        goto out;
    }

    s_pos = (long) *f_pos / line_size;  // which quantum
    ///q_pos = (long) *f_pos % line_size;  // where in quantum

    if (!dev->data) {
        dev->data = kmalloc(nrof_lines * sizeof(char *), GFP_KERNEL);
        if (!dev->data)
            goto out;
        memset(dev->data, 0, nrof_lines * sizeof(char *));
    }
    if (!dev->data[s_pos]) {
        dev->data[s_pos] = kmalloc(line_size, GFP_KERNEL);
        if (!dev->data[s_pos])
            goto out;
    }
    
    /* write only up to the end of this quantum */
    // not 4-digit number EDGE CASE
    //if (count > line_size - q_pos)
    //    count = line_size - q_pos;

    /* TODO: PROCESS BUFF */
    int match[4] = {0};

    int i, j, same_pos = 0, diff_pos = 0;
    for (i = 0; i < 4; i++){
        if (buf[i] == mmind_number[i]){
            match[i] = 1;
            same_pos++;
        }
    }

    for (i = 0; i < 4; i++){
        if (match[i] != 1){      
            for (j = 0; j < 4; j++){
                if (buf[i] == mmind_number[j])
                    diff_pos++;
            }
        }
    }

    char *result_line = "xxxx s+ d- aaaa"; // x: guess s: same d: diff a: attempt
    result_line[0] = buf[0];
    result_line[1] = buf[1];
    result_line[2] = buf[2];
    result_line[3] = buf[3];

    result_line[5] = same_pos; // check if needs to be casted
    result_line[8] = diff_pos;

    int attempt = s_pos;
    int powers[4] = {1000, 100, 10, 1};
    for (i = 0; i < 4; i++){
        result_line[10 + i] = attempt / powers[i]; 
        attempt -= (result_line[10 + i] * powers[i]);
    }

    if (copy_from_user(dev->data[s_pos] , result_line, count)) {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;
    retval = count;

    /* update the size */
    if (dev->size < *f_pos)
        dev->size = *f_pos;

  out:
    up(&dev->sem);
    return retval;
}

long mmind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0, tmp;
	int retval = 0;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != MMIND_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > MMIND_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
	  case MMIND_IOCRESET:
		mmind_line_size = MMIND_LINE_SIZE;
		mmind_nrof_lines = MMIND_MAX_LINES;
		break;

	  case MMIND_IOCSQUANTUM: /* Set: arg points to the value */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(mmind_line_size, (int __user *)arg);
		break;

	  case MMIND_IOCTQUANTUM: /* Tell: arg is the value */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		mmind_line_size = arg;
		break;

	  case MMIND_IOCGQUANTUM: /* Get: arg is pointer to result */
		retval = __put_user(mmind_line_size, (int __user *)arg);
		break;

	  case MMIND_IOCQQUANTUM: /* Query: return it (it's positive) */
		return MMIND_LINE_SIZE;

	  case MMIND_IOCXQUANTUM: /* eXchange: use arg as pointer */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mmind_line_size;
		retval = __get_user(mmind_line_size, (int __user *)arg);
		if (retval == 0)
			retval = __put_user(tmp, (int __user *)arg);
		break;

	  case MMIND_IOCHQUANTUM: /* sHift: like Tell + Query */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mmind_line_size;
		mmind_line_size = arg;
		return tmp;

	  case MMIND_IOCSQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(mmind_nrof_lines, (int __user *)arg);
		break;

	  case MMIND_IOCTQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		mmind_nrof_lines = arg;
		break;

	  case MMIND_IOCGQSET:
		retval = __put_user(mmind_nrof_lines, (int __user *)arg);
		break;

	  case MMIND_IOCQQSET:
		return mmind_nrof_lines;

	  case MMIND_IOCXQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mmind_line_size;
		retval = __get_user(mmind_line_size, (int __user *)arg);
		if (retval == 0)
			retval = put_user(tmp, (int __user *)arg);
		break;

	  case MMIND_IOCHQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mmind_nrof_lines;
		mmind_nrof_lines = arg;
		return tmp;

	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}


loff_t mmind_llseek(struct file *filp, loff_t off, int whence)
{
    struct mmind_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}


struct file_operations mmind_fops = {
    .owner =    THIS_MODULE,		// Owner of the device
    .llseek =   mmind_llseek,		// loff (*lseek) long offset, lseek is a function, seeks in file
    .read =     mmind_read,
    .write =    mmind_write,
    .unlocked_ioctl =  mmind_ioctl,
    .open =     mmind_open,
    .release =  mmind_release,
};


void mmind_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(mmind_major, mmind_minor);

    if (mmind_devices) {
        for (i = 0; i < mmind_nr_devs; i++) {
            mmind_trim(mmind_devices + i);
            cdev_del(&mmind_devices[i].cdev);
        }
    kfree(mmind_devices);
    }

    unregister_chrdev_region(devno, mmind_nr_devs);
}


int mmind_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct mmind_dev *dev;


	// Register mmind, assigns numbers
    // STATIC NOT REQUIRED
    // if (mmind_major) {
    //     devno = MKDEV(mmind_major, mmind_minor);
    //     result = register_chrdev_region(devno, mmind_nr_devs, "mmind");
    // } else {
    result = alloc_chrdev_region(&devno, mmind_minor, mmind_nr_devs,
                                    "mmind");
    mmind_major = MAJOR(devno);
    // }
    if (result < 0) {
        printk(KERN_WARNING "mmind: can't get major %d\n", mmind_major);
        return result;
    }

    mmind_devices = kmalloc(mmind_nr_devs * sizeof(struct mmind_dev),
                            GFP_KERNEL);
    if (!mmind_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(mmind_devices, 0, mmind_nr_devs * sizeof(struct mmind_dev));

    /* Initialize each device. */
    for (i = 0; i < mmind_nr_devs; i++) {
        dev = &mmind_devices[i];
        dev->line_size = mmind_line_size;
        dev->nrof_lines = mmind_nrof_lines;
        sema_init(&dev->sem,1);
        devno = MKDEV(mmind_major, mmind_minor + i);
        cdev_init(&dev->cdev, &mmind_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &mmind_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding mmind%d", err, i);
    }

    return 0; /* succeed */

  fail:
    mmind_cleanup_module();
    return result;
}

module_init(mmind_init_module);
module_exit(mmind_cleanup_module);
