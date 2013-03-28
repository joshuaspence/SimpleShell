/*
 * cmd_intenal.h
 *
 * Author: Joshua Spence
 * SID:    308216350
 *
 * This file contains the internal shell functions.
 */
#ifndef __CMD_INTERNAL_H_
#define __CMD_INTERNAL_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>

#include "utility.h"
#include "strings.h"

#define EXIT_STATUS_CONTINUE    0 // continue execution of shell
#define EXIT_STATUS_QUIT        1 // quit the shell

extern process_information proc_info; // information about child processes
extern FILE * input_redir; // file for input redirection (stdin if null)
extern FILE * output_redir; // file for output redirection (stdout if null)
extern char * path; // path to the executable
#ifdef DEBUG
extern boolean debug; // is debug mode active?
#endif // #ifdef DEBUG
extern char ** environ; // pointer to environment variables

// Forward declarations (to fix compilation warnings)
int setenv(char *, char *, int);
int fileno(FILE *);
char * ctermid(char *);

// Change the current working directory to the specified directory
int change_directory(const char *);

// Clear the terminal screen
int clear_screen(void);

// List the contents of a directory
int list_directory(const char *);

// Print the environment variables
int print_environment(void);

// Echo a comment to the terminal
int echo(const char **);

// Get help
int help(const char *);

// Pause the shell until a specified key is pressed
int pause(void);

// Quit the shell
int quit(void);

#endif // #ifndef __CMD_INTERNAL_H_
