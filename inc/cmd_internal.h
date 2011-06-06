#ifndef __CMD_INTERNAL_H_
#define __CMD_INTERNAL_H_
/*
    cmd_intenal.h

    Author: Joshua Spence
    SID:    308216350
    
    This file contains the internal shell functions.
*/

#include    <stdio.h>
#include    <unistd.h>
#include    <sys/wait.h>
#include    <termios.h>

#include    "utility.h"
#include    "strings.h"

#define     EXIT_STATUS_CONTINUE        0                       // continue execution of shell
#define     EXIT_STATUS_QUIT            1                       // quit the shell

extern process_information proc_info;                           // information about child processes
extern FILE *input_redir;                                       // file for input redirection (stdin if null)
extern FILE *output_redir;                                      // file for output redirection (stdout if null)
extern char *path;                                              // path to the executable
#ifdef DEBUG
extern boolean debug;                                           // is debug mode active?
#endif
extern char **environ;                                          // pointer to environment variables

// forward declarations (to fix compilation warnings)
int setenv(char *, char *, int);
int fileno(FILE *);
char *ctermid(char *);

// change the current working directory to the specified directory
int change_directory(const char *);

// clear the terminal screen
int clear_screen(void);

// list the contents of a directory
int list_directory(const char *);

// print the environment variables
int print_environment(void);

// echo a comment to the terminal
int echo(const char **);

// get help
int help(const char *);

// pause the shell until a specified key is pressed
int pause(void);

// quit the shell
int quit(void);

#endif
