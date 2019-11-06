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

int main()
{
    int fd, retval, remaining=0;
    char number[5]= "";
    char line[16] = "";
    char option = 'a';
    
	fd = open("/dev/mmind0", 0);
	
	while (option != 'x'){
		printf("p: Play\nr: Get remaining attempts\ne: End game\nn: New game with new number\nx: Exit\n");
				 
		printf("Choose an option: ");
		scanf(" %c", &option);
		getchar();
		
		switch(option){
			case PLAY:
				printf("Guess: ");
				scanf(" %s", number);
				getchar();
				
				if ((sizeof(number) / sizeof(char)) != 5)
					printf("Invalid guess, try a 4-digit number.");
				
				retval = write(fd, number, 5);
				printf("%d", retval);
				
				if (retval != 0)
					printf("Error while playing.\n");
					
				retval = read(fd, line, 16);
				
				if (retval == 0)
					printf("%s", line);
				else
					printf("Error while reading.\n");
				break;
				
			case REMAINING:
				retval = ioctl(fd, MMIND_REMAINING, &remaining);	
				printf("Remaining: %d", remaining);
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
				scanf("%s", number);
				getchar();
				retval = ioctl(fd, MMIND_NEWGAME, number);
				break;	
				
			case EXIT:
				printf("Exitting...\n");
				break;
			default:
				printf("Invalid choice: %c", option);
				break;
		}
	}
	
    return 0; 
}
