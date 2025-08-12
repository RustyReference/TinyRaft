#ifndef UTILS_H
#define UTILS_H
#define BASE 10

/**
 * Converts a string into an integer, with error checking.
 * 
 * @param input the input string
 * @return the string converted to a number
 */
int strtoint(char *input);

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
char *strndup_checked(char *str, size_t limit);

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
char *strdup_checked(char *str);

#endif