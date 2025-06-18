#include <stdio.h>
#include <errno.h>
#include "node.h"
#define BASE 10

int strtoint(char *input) {
    int num = -1;

    // Conversion to uint w/ error checking
	errno = 0;
	char *endptr = NULL;
	num = (int) strtoumax(input, &endptr, BASE);
	if (errno) {
        printf("\nError converting input: %s", strerror(errno));
		exit(1);
	} else if (num == -1) {
        printf("Failed to convert string to number: %s", input);
    }
    
    return num;
}