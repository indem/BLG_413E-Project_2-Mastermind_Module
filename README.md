# Mastermind

**Ezgi Paket**, 070140122 | **İrem Nur Demirtaş**, 150150302 | **Ayser Ecem Konu**, 150160711 

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

