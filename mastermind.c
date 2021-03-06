#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/printk.h>
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


#ifndef __ASM_ASM_UACCESS_H
    #include <linux/uaccess.h>
#endif
#include "mastermind_ioctl.h"


#define MMIND_MAJOR 0
#define MMIND_NR_DEVS 1
#define MMIND_NUMBER "1234"           // secret number
#define MMIND_LINE_SIZE 16 * sizeof(char)          // FIXED 
#define MMIND_MAX_GUESSES 13        // DEFAULT: 10 
#define MMIND_MAX_LINES 256         // FIXED 

int mmind_major = MMIND_MAJOR;
int mmind_minor = 0;
const int digits = 5 * sizeof(char);
char mmind_number[5] = MMIND_NUMBER;
int mmind_max_guesses = MMIND_MAX_GUESSES;
int mmind_line_size = MMIND_LINE_SIZE;
int mmind_nrof_lines = MMIND_MAX_LINES;
int mmind_nr_devs = MMIND_NR_DEVS;

module_param(mmind_major, int, S_IRUGO);
module_param(mmind_minor, int, S_IRUGO);
//module_param(mmind_number, charp, S_IRUGO);
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

struct mmind_dev *mmind_device;

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

    return 0;
}


int mmind_release(struct inode *inode, struct file *filp) {
    return 0;
}


ssize_t mmind_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos) {    
    struct mmind_dev *dev = filp->private_data;
    int line_size = dev->line_size;
    int s_pos, q_pos; // not required, q_pos;
    ssize_t retval = 0;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    if (*f_pos >= dev->size)
        goto out;
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;
		
    s_pos = (long) *f_pos / line_size;
	
    if (dev->data == NULL || ! dev->data[s_pos])
        goto out;

    if (count > line_size - q_pos)
        count = line_size - q_pos;

    if (copy_to_user(buf, dev->data[s_pos], mmind_line_size)) {  // q_pos 0
        retval = -EFAULT;
        goto out;
    }
	
    *f_pos += mmind_line_size;
    retval = mmind_line_size;

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
    char *result_line = kmalloc(mmind_line_size, GFP_KERNEL); // x: guess s: same d: diff a: attempt
    memset(result_line, 0, mmind_line_size);
    
    if (count != 5){ // invalid input
        printk("Invalid input, count: %d", count);
        goto out;
    }
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    if (*f_pos >= line_size * nrof_lines) {
        retval = 0;
        goto out;
    }
    
    s_pos = (long) dev->size / line_size; 
    
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
    
    if (mmind_max_guesses == 0){
        strcpy(dev->data[s_pos], "YOU LOSE.\n");
        dev->size += mmind_line_size;
        retval = count;
        goto out;
    }
    
    char *temp = kmalloc(count, GFP_KERNEL);
    memset(temp, 0, count);
    if (copy_from_user(temp, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
        
    
    int match[4] = {0};
    
    unsigned int i, j;
    unsigned int same_pos = 0, diff_pos = 0;
    for (i = 0; i < 4; i++){
        if (temp[i] == mmind_number[i]){
            match[i] = 1;
            same_pos++;
        }
    }
    
    for (i = 0; i < 4; i++){
        if (match[i] != 1){
            for (j = 0; j < 4; j++){
                if (temp[i] == mmind_number[j])
                    diff_pos++;
            }
        }
    }
    
    if (same_pos == 4){
        strcpy(dev->data[s_pos], "YOU WON!\n");
        retval = count;
        dev->size += mmind_line_size;
        goto out;
    }
    
    unsigned char c_same_pos = '0' + same_pos, c_diff_pos = '0' + diff_pos;
    
	int attempt = dev->size / dev->line_size + 1;
	char att_str[5] = "";
	sprintf(att_str, "%04d", attempt); 
    strncpy(result_line, temp, 4);
    strncat(result_line, " ", 1);
    strncat(result_line, &c_same_pos, 1);
    strncat(result_line, "+ ", 1);
    strncat(result_line, &c_diff_pos, 1);
    strncat(result_line, "- ", 1);
    strncat(result_line, " ", 1);
    strncat(result_line, att_str, 4);
    strncat(result_line, "\n", 1);
    
    strcpy(dev->data[s_pos], result_line);
    
    
    retval = count;
    mmind_max_guesses--;
    /* update the size */
    dev->size += mmind_line_size;

out:
    up(&dev->sem);
    return retval;
}

long mmind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0;
	int retval = 0;


	if (_IOC_TYPE(cmd) != MMIND_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > MMIND_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) return -EFAULT;

	switch(cmd) {
	  case MMIND_ENDGAME: /* Clear history and reset guess count*/
		if (! capable(CAP_SYS_ADMIN))
			return -EPERM;
		mmind_trim(mmind_device);
		mmind_max_guesses = MMIND_MAX_GUESSES;
		break;

	  case MMIND_NEWGAME:		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		int temp;
		mmind_trim(mmind_device);
		mmind_max_guesses = MMIND_MAX_GUESSES;
		retval = __get_user(temp, (int* __user *)arg);
		snprintf(mmind_number, 4, "%d", temp);

		break;
	  
	  case MMIND_REMAINING: 
		retval = __put_user(mmind_max_guesses, (char* __user *)arg);
		break;

	  default: 
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
    .owner =    THIS_MODULE,
    .llseek =   mmind_llseek,
    .read =     mmind_read,
    .write =    mmind_write,
    .unlocked_ioctl =  mmind_ioctl,
    .open =     mmind_open,
    .release =  mmind_release,
};


void mmind_cleanup_module(void)
{
    dev_t devno = MKDEV(mmind_major, mmind_minor);

    if (mmind_device) {
		mmind_trim(mmind_device);
		cdev_del(&mmind_device->cdev);
		kfree(mmind_device);
	}

    unregister_chrdev_region(devno, mmind_nr_devs);
}


int mmind_init_module(void)
{
    int result;
    int err;
    dev_t devno = 0;
    struct mmind_dev *dev;



    result = alloc_chrdev_region(&devno, mmind_minor, mmind_nr_devs,
                                    "mmind");
    mmind_major = MAJOR(devno);
    if (result < 0) {
        printk(KERN_WARNING "mmind: can't get major %d\n", mmind_major);
        return result;
    }

    mmind_device = kmalloc(sizeof(struct mmind_dev),
                            GFP_KERNEL);
    if (!mmind_device) {
        result = -ENOMEM;
        goto fail;
    }
    memset(mmind_device, 0, sizeof(struct mmind_dev));

    dev = mmind_device;
	dev->line_size = mmind_line_size;
	dev->nrof_lines = mmind_nrof_lines;
	sema_init(&dev->sem,1);
	devno = MKDEV(mmind_major, mmind_minor);
	cdev_init(&dev->cdev, &mmind_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &mmind_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding mmind", err);


    return 0;

  fail:
    mmind_cleanup_module();
    return result;
}

module_init(mmind_init_module);
module_exit(mmind_cleanup_module);
