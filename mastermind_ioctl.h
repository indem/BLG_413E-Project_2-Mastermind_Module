#ifndef __MMIND_H
#define __MMIND_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

//Command Number is the number that is assigned to the ioctl. It is used to differentiate the commands from one another.

#define MMIND_IOC_MAGIC  'k' //The Magic Number is a unique number or character that will differentiate our set of ioctl calls from the other ioctl calls.
#define MMIND_ENDGAME    _IO(MMIND_IOC_MAGIC, 0) //Used for a simple ioctl that sends nothing but the type and number, and receives back nothing but an (integer) retval.
#define MMIND_NEWGAME	 _IOW(MMIND_IOC_MAGIC,  1, char*) //Used for an ioctl that writes data to the device driver.
#define MMIND_REMAINING  _IOR(MMIND_IOC_MAGIC,  2, int) //Used for an ioctl that reads data from the device driver. The driver will be allowed to return sizeof(data_type) bytes to the user.
#define MMIND_MAXNR 3
#endif
