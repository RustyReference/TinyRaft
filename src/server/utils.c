#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "utils.h"

/**
 * Converts a string into an integer, with error checking.
 * 
 * @param input the input string
 * @return the string converted to a number
 */
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

/**
 * @param str the string to duplicate
 * @param limit the size of the BUFFER the string is being copied to
 *      to prevent buffer overflow.
 *      DO NOT JUST SET 'limit' EQUAL TO THE SIZE OF 'str'!
 * 
 * Postcondition:
 *      - RETURNS DYNAMIC MEMORY
 * 
 * @return a duplicate of the given string, safely
 */
char *strndup_checked(char *str, size_t limit) {
    errno = 0;
    char *new_str = strndup(str, limit);
    if (new_str == NULL || errno != 0) {
        fprintf(stderr, "strndup_safe(): Error duplicating string: %s",
            strerror(errno));
        exit(EXIT_FAILURE);
    }

    return new_str;
}

/**
 * Duplicates a string with indefinite size, with error handling
 * 
 * @param str the string to duplicate
 * 
 * Postcondition:
 *      - RETURNS DYNAMIC MEMORY
 * 
 * @return a duplicate of the given string
 */
char *strdup_checked(char *str) {
    errno = 0;
    char *new_str = strdup(str);
    if (new_str == NULL || errno != 0) {
        fprintf(stderr, "strdup_checked(): Error duplicating string: %s",
            strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    return new_str;
}