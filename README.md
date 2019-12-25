# Mastermind

[Ezgi Paket](https://github.com/ezgipaket/) | [İrem Demirtaş](https://github.com/indem/) | [Ecem Konu](https://github.com/ecem.konu/)

Mastermind is a kernel module written for Linux Kernel 3.13. Inspired by [Scull](https://github.com/starpos/scull), it is a character device on which two users can play the game Mastermind.
One user initializes the module with a four digit number they picked, the other user tries to guess this number by writing to the device. To check whether their guess 
was correct, the second player reads from the device. At each read, the device returns the number of correct digits in their correct place as well as those in incorrect places. 

## Compilation & Running
Provide admin priviledges by

```sudo su```

### For module 
To create mastermind.ko

```make``` 

Insert module by calling

```insmod ./mastermind.ko```

To obtain device's major number

```grep mmind /proc/devices```

mknod will be run using major number returned from grep function. Assume that grep returned 245 as mmind major number

```mknod /dev/mmind0 c 245 0```

To write

```echo "xxxx" > /dev/mmind0``` 

where x's are digits for guess

To read

```cat /dev/mmind0```

### For test

To compile

```gcc mastermind_test.c -o mmind```

To run

```./mmind```

Program will provide you with options for testing.

