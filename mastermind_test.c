#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "mastermind_ioctl.h"
#include <unistd.h>

int main()
{
    int fd, retval, remaining=0;
    char* number="2345";
    
    fd = open("/dev/mmind0", 0);
    if (fd == -1){
		printf("Couldn't open\n");
		return 0;
	}
	
//	retval = ioctl(fd, MMIND_REMAINING, &remaining);
//	retval = ioctl(fd, MMIND_ENDGAME);
//	retval = ioctl(fd, MMIND_NEWGAME, number);
	
	retval = write(fd, number, 5);
	printf("Retval %d\n", retval);
	number = "3489";
	retval = write(fd, number, 5);
	printf("Retval %d\n", retval);
	
	printf("Remaining %d\n", remaining);
	
    return 0; 
}
