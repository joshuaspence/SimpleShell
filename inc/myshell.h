/*
 * myshell.h
 *
 * Author: Joshua Spence
 * SID:    308216350
 *
 * This is the header file for the 'myshell' shell.
 */
#ifndef __MYSHELL_H_
#define __MYSHELL_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_ARGS 64 // maximum number of arguments (size of argument array)

#include "cmd_internal.h"
#include "utility.h"
#include "strings.h"

process_information proc_info; // information about child processes
FILE * input_redir; // file for input redirection (stdin if null)
FILE * output_redir; // file for output redirection (stdout if null)
char * path; // path to the executable
#ifdef DEBUG
boolean debug; // is debug mode on?
#endif // #ifdef DEBUG

// Output the shell prompt
void output_shell_prompt(const char *);

// Reallocate memory for the input buffer if required
char * get_input(char *, FILE *);

// Check arguments for dont wait character
void check_for_dont_wait(char **);

// Check arguments for input redirection
void check_for_input_redirection(char **);

// Check arguments for output redirection
void check_for_output_redirection(char **);

// Process an external command by passing it the external shell
int process_external_command(char **);

// Display an error message that a command requiring an argument has been executed without an argument
void error_no_argument(const char *);

// Display an error message that a command has been specified with an unknown argument
void error_unrecognised_argument(const char *, const char *);

// Reset the process information
void reset_process_information(void);

// Main function to run the shell
int main(int, char **);

#ifdef DEBUG
// Turns debug mode on or off
int debug_mode(const boolean);

// Display a debug message indicating that a command has been recognised
void debug_command_recognised_message(const char *, const char *);

// Display a debug message indicating that a command has been executed
void debug_command_executed_message(const char *, const char *);

// Display a debug message showing the input that has been read
void debug_read_line_message(const char *);

// Display a debug message showing the command and arguments that have been recognised and will be processed
void debug_command_args_message(const char *, const char **);

#endif // #ifdef DEBUG
#endif // #ifndef __MYSHELL_H_
