#ifndef __MMIND_H
#define __MMIND_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

//Command Number is the number that is assigned to the ioctl. It is used to differentiate the commands from one another.

#define MMIND_IOC_MAGIC  'k' //The Magic Number is a unique number or character that will differentiate our set of ioctl calls from the other ioctl calls.
#define MMIND_IOCRESET    _IO(MMIND_IOC_MAGIC, 0) //Used for a simple ioctl that sends nothing but the type and number, and receives back nothing but an (integer) retval.
#define MMIND_IOCSQUANTUM _IOW(MMIND_IOC_MAGIC,  1, int) //Used for an ioctl that writes data to the device driver.
#define MMIND_IOCSQSET    _IOW(MMIND_IOC_MAGIC,  2, int)
#define MMIND_IOCTQUANTUM _IO(MMIND_IOC_MAGIC,   3)
#define MMIND_IOCTQSET    _IO(MMIND_IOC_MAGIC,   4)
#define MMIND_IOCGQUANTUM _IOR(MMIND_IOC_MAGIC,  5, int) //Used for an ioctl that reads data from the device driver. The driver will be allowed to return sizeof(data_type) bytes to the user.
#define MMIND_IOCGQSET    _IOR(MMIND_IOC_MAGIC,  6, int)
#define MMIND_IOCQQUANTUM _IO(MMIND_IOC_MAGIC,   7)
#define MMIND_IOCQQSET    _IO(MMIND_IOC_MAGIC,   8)
#define MMIND_IOCXQUANTUM _IOWR(MMIND_IOC_MAGIC, 9, int)
#define MMIND_IOCXQSET    _IOWR(MMIND_IOC_MAGIC,10, int)
#define MMIND_IOCHQUANTUM _IO(MMIND_IOC_MAGIC,  11)
#define MMIND_IOCHQSET    _IO(MMIND_IOC_MAGIC,  12)
#define MMIND_IOC_MAXNR 12

#endif
