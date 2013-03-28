/*
 * utility.h
 *
 * Author: Joshua Spence
 * SID:    308216350
 *
 * Contains utility functions used for general string manipulation and other actions.
 */
#ifndef __UTILITY_H_
#define __UTILITY_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "strings.h"

#define ALLOCATION_BLOCK 64 // amount of memory to be allocated each time when calling malloc for a string of unknown size

typedef int boolean; // boolean type
#define FALSE 0 // used for boolean false
#define TRUE  1 // used for boolean true

typedef struct {
    pid_t pid; // process id when forking
    boolean dont_wait; // wait for forked process?
    int status; // status information about the forked process
} process_information;

extern int errno; // system error number

// Forward declarations (to fix compilation warnings)
ssize_t readlink(char * restrict, char * restrict, size_t);

// Remove a character from a string, shifting all other characters to fill the gap
char * remove_character(char *, const char);

// Works similarly to strtok except can handle quoted arguments
char * quoted_strtok(char *, const char *, const char *);

// Get path to current executable
char * get_path(char *);

// Counts the number of digits in an integer
unsigned int digits(const int);

// Counts the number of elements in an array of strings
unsigned int array_size(const char **);

// Shifts the contents of an array of strings one place to the left, overwriting the current element
char ** array_movetoend(char **);

// Counts the number of occurrences of a character within a string
unsigned int string_char_count(const char *, const char);

// Counts the number of occurrences of any single character from a string within a different string
unsigned int string_chars_count(const char *, const char *);

// Print an error message to stderr and abort
void sys_err(const char *);

// Print an error message to stderr
void err(const char *);

#ifdef DEBUG
// Print a debug message
void debug_message(const char *);
#endif // #ifdef DEBUG
#endif // #ifndef __UTILITY_H_
