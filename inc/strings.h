#ifndef __STRINGS_H_
#define __STRINGS_H_
/*
    strings.h

    Author: Joshua Spence
    SID:    308216350
        
    This file defines strings used in the 'myshell' shell. The shell can be customised by changing
    strings in this file.
*/

#define     PROMPT_SUFFIX               " ==> "                 // appears at the end of the shell prompt
#define     PAUSE_MESSAGE               "Press Enter to continue..." // prompt to display when in pause command

#define     README_PATH                 "./readme"              // path to readme file relative to startup path

// internal commands
#define     CHANGE_DIRECTORY_COMMAND    "cd"
#define     CLEAR_SCREEN_COMMAND        "clr"
#define     LIST_DIRECTORY_COMMAND      "dir"
#define     PRINT_ENVIRONMENT_COMMAND   "environ"
#define     ECHO_COMMAND                "echo"
#define     HELP_COMMAND                "help"
#define     PAUSE_COMMAND               "pause"
#define     QUIT_COMMAND                "quit"

#define     CHANGE_DIRECTORY_CMD_NAME   "Change directory"
#define     CLEAR_SCREEN_CMD_NAME       "Clear screen"
#define     LIST_DIRECTORY_CMD_NAME     "List directory"
#define     PRINT_ENVIRONMENT_CMD_NAME  "Print environment"
#define     ECHO_CMD_NAME               "Echo"
#define     HELP_CMD_NAME               "Help"
#define     PAUSE_CMD_NAME              "Pause"
#define     QUIT_CMD_NAME               "Quit"

// special characters
#define     DONT_WAIT_CHARACTER         '&'                     // character used to set dont_wait variable to run commands in the background
#define     INPUT_REDIRECTION_CHAR      '<'                     // character used to redirect input from a file
#define     OUTPUT_REDIRECTION_CHAR     '>'                     // character used to redict output to a file
#define     SEPARATORS                  " \t\n"                 // token sparators
#define     QUOTATION_MARKS             "\""                    // quotation marks
#define     EXIT_PAUSE_CHARACTER        '\n'                    // character used to exit pause mode

#ifdef DEBUG

#define     DEBUG_COMMAND               "debug"                 // command to access debug mode
#define     DEBUG_ON                    "on"                    // command line argument to turn debug mode on
#define     DEBUG_OFF                   "off"                   // command line argument to turn debug mode off
	
#define     DEBUG_PROMPT				"[DEBUG MODE] "         // text to prefix shell prompt if debug mode active
#define     DEBUG_MESSAGE_PREFIX		"[DEBUG]: "             // text to appear before any debug messages

#endif

#endif
