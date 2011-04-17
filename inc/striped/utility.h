#ifndef __UTILITY_H_
#define __UTILITY_H_
/*
    utility.h

    Author:	Joshua Spence
	SID:	308216350
	
	Contains utility functions used for general string manipulation and other actions.
*/

#include	<errno.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h>

#include	"strings.h"

#define		ALLOCATION_BLOCK			64						// amount of memory to be allocated each time when calling malloc for a string of unknown size

typedef int boolean;                                            // boolean type
#define		FALSE						0                       // used for boolean false
#define		TRUE						1                       // used for boolean true

typedef struct process_information {
	pid_t pid;													// process id when forking
	boolean dont_wait;											// wait for forked process?
	int status;													// status information about the forked process
} process_information;

extern int errno;                                               // system error number

// forward declarations (to fix compilation warnings)
ssize_t readlink(char *restrict, char *restrict, size_t);

// remove a character from a string, shifting all other characters to fill the gap
char *remove_character(char *, const char);

// works similarly to strtok except can handle quoted arguments
char *quoted_strtok(char *, const char *, const char *);

// get path to current executable
char *get_path(char *);

// counts the number of digits in an integer
unsigned int digits(const int);

// counts the number of elements in an array of strings
unsigned int array_size(const char **);

// shifts the conents of an array of strings one place to the left, overwriting the current element
char **array_movetoend(char **);

// counts the number of occurences of a character within a string
unsigned int string_char_count(const char *, const char);

// counts the number of occurences of any single character from a string within a different string
unsigned int string_chars_count(const char *, const char *);

// print an error message to stderr and abort
void sys_err(const char *);

// print an error message to stderr
void err(const char *);
#endif