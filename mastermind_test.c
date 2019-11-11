#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "mastermind_ioctl.h"
#include <unistd.h>

#define PLAY		'p'
#define REMAINING	'r'
#define END_GAME	'e'
#define NEW_GAME	'n'
#define EXIT		'x'
#define RESULT		's'

int main()
{
    int fd, retval, remaining=0, num;
    char number[5]= "";
    char line[1000] = "";
    char option = 'a';
    
	fd = open("/dev/mmind0", O_RDWR);
	
	while (option != 'x'){
		printf("p: Play\nr: Get remaining attempts\ne: End game\nn: New game with new number\ns: Get result\nx: Exit\n");
				 
		printf("Choose an option: ");
		scanf(" %c", &option);
		getchar();
		
		switch(option){
			case PLAY:
				printf("Guess: ");
				scanf(" %s", number);
				getchar();
				
				if ((sizeof(number) / sizeof(char)) != 5)
					printf("Invalid guess, try a 4-digit number.\n");
				
				retval = write(fd, number, 5);
				
				if (retval != 5)
					printf("Error while playing.\n");
				
				break;
				
			case REMAINING:
				retval = ioctl(fd, MMIND_REMAINING, &remaining);	
				printf("Remaining: %d\n", remaining);
				break;
				
			case END_GAME:
				retval = ioctl(fd, MMIND_ENDGAME);
				if (retval == 0)
					printf("Game successfully ended.\n");
				else
					printf("Error while ending game.\n");
				break;
				
			case NEW_GAME:
				printf("New number: ");
				scanf("%d", &num);
				getchar();
				printf("Setting to: %d", num);
				retval = ioctl(fd, MMIND_NEWGAME, &num);
				
				break;	
				
			case EXIT:
				printf("Exitting...\n");
				break;
				
			default:
				printf("Invalid choice: %c\n", option);
				break;
		}
	}
	
    return 0; 
}
