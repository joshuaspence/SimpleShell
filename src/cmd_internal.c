/*
 * cmd_intenal.c
 *
 * Author:     Joshua Spence
 * SID:        308216350
 *
 * This file contains the internal shell functions.
 */

#include "../inc/cmd_internal.h"

/*
 * Change the current working directory to the specified directory.
 *
 * PARAMETERS
 *     directory: The directory to change to. If null, then the current working
 *         directory is output.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int change_directory(const char * directory) {
    char * cwd; // current working directory
    int stdout_save; // to save and restore stdout

    if (proc_info.dont_wait) {
        err("Background execution is not supported for this command. Ignoring this parameter.");
    }

    if (input_redir) {
        err("Input redirection is not supported for this command. Ignoring this parameter.");
    }

    if (directory == NULL) {
        // Directory not specified - report current directory
#ifdef DEBUG
        if (debug) {
            debug_message("Directory not specified. Reporting current directory.");
        }

#endif // #ifdef DEBUG
        // Get the current working directory
        cwd = getcwd(NULL, (size_t) 0); // getcwd will dynamically allocate memory for cwd

        // Redirect output if necessary
        if (output_redir != NULL) {
            stdout_save = dup(STDOUT_FILENO); // save stdout
            dup2(fileno(output_redir), STDOUT_FILENO); // redirect output
        }

        // Output the current working directory
        printf("%s\n", cwd);

        if (output_redir != NULL) {
            dup2(stdout_save, STDOUT_FILENO); // restore stdout
        }

        // Clean up
        free(cwd); // free the memory dynamically allocated by getcwd
    } else {
        // Directory specified - change to this directory
#ifdef DEBUG
        if (debug) {
            // Create debug message
            const char msg[] = "Directory '%s' specified. Attempt to change to this directory.";
            char * dbg_msg;

            // Memory allocation
            if (!(dbg_msg = (char *) malloc((size_t) ((strlen(msg) + strlen(directory) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

            // Output debug message
            sprintf(dbg_msg, msg, directory);
            debug_message(dbg_msg);

            // Clean up
            free(dbg_msg);
        }

#endif // #ifdef DEBUG
        // Attempt to change the directory
        if (!chdir(directory)) {
#ifdef DEBUG
            if (debug) {
                // Create debug message
                const char msg[] = "Changed current working directory to '%s'.";
                char * dbg_msg;

                // Memory allocation
                if (!(dbg_msg = (char *) malloc((size_t) ((strlen(msg) + strlen(directory) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                // Output debug message
                sprintf(dbg_msg, msg, directory);
                debug_message(dbg_msg);

                // Clean up
                free(dbg_msg);
            }

#endif // #ifdef DEBUG
            // Get the full path to the new directory
            cwd = getcwd(NULL, (size_t) 0); // getcwd will dynamically allocate memory for cwd

#ifdef DEBUG
            if (debug) {
                debug_message("Attempting to change 'PWD' environment variable.");
            }
#endif // #ifdef DEBUG
            // Set the environment variable to the new directory
            if (setenv("PWD", cwd, 1)) sys_err("setenv"); // set the 'pwd' environment variable to the new directory, overwriting any existing value

#ifdef DEBUG
            if (debug) {
                // Create debug message
                const char dbg[] = "Changed 'PWD' environment variable '%s'.";
                char * dbg_msg;

                // Memory allocation
                if (!(dbg_msg = (char *) malloc((size_t) ((strlen(dbg) + strlen(cwd) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                // Output debug message
                sprintf(dbg_msg, dbg, cwd);
                debug_message(dbg_msg);

                // Clean up
                free(dbg_msg);
            }

#endif // #ifdef DEBUG
            // Clean up
            free(cwd); // free the memory dynamically allocated by getcwd
        } else {
            // Unable to change directory - output error message
            // Create error message
            const char msg[] = "Unable to change to directory '%s'.";
            char * err_msg;

            // Memory allocation
            if (!(err_msg = (char *) malloc((size_t) ((strlen(msg) + strlen(directory) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for err_msg */

            // Output error message
            sprintf(err_msg, msg, directory);
            err(err_msg);

            // Clean up
            free(err_msg);
        }
    }

    // Return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
 * Clear the terminal screen.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int clear_screen(void) {
    if (proc_info.dont_wait) {
        err("Background execution is not supported for this command. Ignoring this parameter.");
    }
    if (input_redir) {
        err("Input redirection is not supported for this command. Ignoring this parameter.");
    }
    if (output_redir) {
        err("Output redirection is not supported for this command. Ignoring this parameter.");
    }

    // Clear the screen
    system("clear");

    // Return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
 * List the contents of a directory.
 *
 * PARAMETERS
 *     directory: The path of the directory to list the contents of.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int list_directory(const char * directory) {
    // Fork the current process
    switch (proc_info.pid = fork()) {
        case -1: // fork failed
            sys_err("fork");
            break;

        case 0: // child
#ifdef DEBUG
            if (debug) {
                debug_message("Setting 'parent' environment variable in child process.");
            }

#endif // #ifdef DEBUG
            // Set environment variable
            if (setenv("parent", path, 1)) sys_err("setenv"); // set the 'parent' environment variable to the path to the shell, overwriting any existing value

#ifdef DEBUG
            if (debug) {
                debug_message("Executing '/bin/ls' in child process.");
            }

#endif
            // Redirect input if necessary
            if (input_redir) {
                    dup2(fileno(input_redir), STDIN_FILENO);
            }

            // Redirect output if necessary
            if (output_redir) {
                    dup2(fileno(output_redir), STDOUT_FILENO);
            }

            // Execute the command with the appropriate arguments
            execlp("/bin/ls", "ls", "-al", directory, NULL);
            sys_err("execlp"); // if execution reaches this line, an error has occured as execlp should never return
            break;

        default: // parent
            if (!proc_info.dont_wait) {
#ifdef DEBUG
                if (debug) {
                    // Create debug message
                    const char dbg[] = "Parent process waiting for child process [PID: %d] to return.";
                    char * dbg_msg;

                    // Memory allocation
                    if (!(dbg_msg = (char *) malloc((strlen(dbg) + digits(proc_info.pid) + 1) * sizeof(char)))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                    // Output debug message
                    sprintf(dbg_msg, dbg, proc_info.pid);
                    debug_message(dbg_msg);

                    // Clean up
                    free(dbg_msg);
                }

#endif // #ifdef DEBUG
                // Wait for the child process to return
                waitpid(proc_info.pid, &proc_info.status, WUNTRACED);
#ifdef DEBUG

                if (debug) {
                    // Create debug message
                    const char dbg[] = "Child process %d has returned with status: %d.";
                    char * dbg_msg;

                    // Memory allocation
                    if (!(dbg_msg = (char *) malloc((strlen(dbg) + digits(proc_info.pid) + digits(proc_info.status) + 1) * sizeof(char)))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                    // Output debug message
                    sprintf(dbg_msg, dbg, proc_info.pid, proc_info.status);
                    debug_message(dbg_msg);

                    // Clean up
                    free(dbg_msg);
                }
#endif // #ifdef DEBUG
            }
#ifdef DEBUG
            else {
                if (debug) {
                    // Create debug message
                    const char dbg[] = "Parent process continuing without waiting for child process [PID: %d] to return.";
                    char * dbg_msg;

                    // Memory allocation
                    if (!(dbg_msg = (char *) malloc((strlen(dbg) + digits(proc_info.pid) + 1) * sizeof(char)))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                    // Output debug message
                    sprintf(dbg_msg, dbg, proc_info.pid);
                    debug_message(dbg_msg);

                    // Clean up
                    free(dbg_msg);
                }
            }

#endif // #ifdef DEBUG
    }

    // Return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
 * Print the environment variables.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int print_environment(void) {
    const char ** env = (const char **) environ; // pointer to step through environment variables
    int stdout_save; // to save and restore stdout

    if (proc_info.dont_wait) {
        err("Background execution is not supported for this command. Ignoring this parameter.");
    }
    if (input_redir) {
        err("Input redirection is not supported for this command. Ignoring this parameter.");
    }

    // Redirect output if necessary
    if (output_redir) {
        stdout_save = dup(STDOUT_FILENO); // save stdout
        dup2(fileno(output_redir), STDOUT_FILENO); // redirect output
    }

    // Print all environment variables
    while(*env) {
        printf("%s\n", *env++);
    }

    if (output_redir) {
        dup2(stdout_save, STDOUT_FILENO); // restore stdout
    }

    // Return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
 * Echo a comment to the terminal.
 *
 * PARAMETERS
 *     args: The first argument that forms the comment to be echoed.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int echo(const char ** args) {
    int stdin_save; // to save and restore stdin
    int stdout_save; // to save and restore stdout

    if (proc_info.dont_wait) {
        err("Background execution is not supported for this command. Ignoring this parameter.");
    }

    // Redirect input if necessary
    if (input_redir) {
        stdin_save = dup(STDIN_FILENO); // save stdin
        dup2(fileno(input_redir), STDIN_FILENO); // redirect input
    }

    // Redirect output if necessary
    if (output_redir) {
        stdout_save = dup(STDOUT_FILENO); // save stdout
        dup2(fileno(output_redir), STDOUT_FILENO); // redirect output
    }

    // Print the comments
    while(*args) {
        printf("%s ", *args++);
    }
    printf("\n");

    if (input_redir) {
        dup2(stdin_save, STDIN_FILENO); // restore stdin
    }
    if (output_redir) {
        dup2(stdout_save, STDOUT_FILENO); // restore stdout
    }

    // Return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
 * Get help.
 *
 * PARAMETERS
 *     home: The path to the home directory, which should contain the readme
 *         file.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int help(const char * home) {
    // Fork the current process
    switch(proc_info.pid = fork()) {
        case -1: // fork failed
            sys_err("fork");
            break;

        case 0: // child
#ifdef DEBUG
            if (debug) {
                debug_message("Setting 'parent' environment variable in child process.");
            }

#endif // #ifdef DEBUG
            // Set environment variable
            if (setenv("parent", path, 1)) sys_err("setenv"); // set the 'parent' environment variable to the path to the shell, overwriting any existing value

#ifdef DEBUG
            if (debug) {
                debug_message("Executing '/bin/more' in child process.");
            }

#endif // #ifdef DEBUG
            // Redirect input if necessary
            if (input_redir) {
                dup2(fileno(input_redir), STDIN_FILENO);
            }

            // Redirect output if necessary
            if (output_redir) {
                dup2(fileno(output_redir), STDOUT_FILENO);
            }

            // Change to the directory in which the readme file should exist
            if (chdir(home)) sys_err("chdir"); // attempt to change to home directory

            // Ensure readme file exists and can be read
            if (!access(README_PATH, R_OK)) {
                // Create error message
                const char msg[] = "Unable to open the readme file '%s'.";
                char * err_msg;

                // Memory allocation
                if (!(err_msg = (char *) malloc((size_t) ((strlen(msg) + strlen(README_PATH) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                // Output error message
                sprintf(err_msg, msg, README_PATH);
                err(err_msg);

                // Clean up
                free(err_msg);
            }

            // Execute the command with the appropriate arguments
            execlp("/bin/more", "more", README_PATH, NULL);
            sys_err("execlp"); // if execution reaches this line, an error has occured as execlp should never return
            break;

        default: // parent
            if (!proc_info.dont_wait) {
#ifdef DEBUG
                if (debug) {
                    // Create debug message
                    const char dbg[] = "Parent process waiting for child process [PID: %d] to return.";
                    char * dbg_msg;

                    // Memory allocation
                    if (!(dbg_msg = (char *) malloc((strlen(dbg) + digits(proc_info.pid) + 1) * sizeof(char)))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                    // Output debug message
                    sprintf(dbg_msg, dbg, proc_info.pid);
                    debug_message(dbg_msg);

                    // Clean up
                    free(dbg_msg);
                }

#endif // #ifdef DEBUG
                // Wait for the child process to return
                waitpid(proc_info.pid, &proc_info.status, WUNTRACED);
#ifdef DEBUG

                if (debug) {
                    // Create debug message
                    const char dbg[] = "Child process %d has returned with status: %d.";
                    char * dbg_msg;

                    // Memory allocation
                    if(!(dbg_msg = (char *) malloc((strlen(dbg) + digits(proc_info.pid) + digits(proc_info.status) + 1) * sizeof(char)))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                    // Output debug message
                    sprintf(dbg_msg, dbg, proc_info.pid, proc_info.status);
                    debug_message(dbg_msg);

                    // Clean up
                    free(dbg_msg);
                }
#endif // #ifdef DEBUG
            }
#ifdef DEBUG
            else {
                if (debug) {
                    // Create debug message
                    const char dbg[] = "Parent process continuing without waiting for child process [PID: %d] to return.";
                    char * dbg_msg;

                    // Memory allocation
                    if (!(dbg_msg = (char *) malloc((strlen(dbg) + digits(proc_info.pid) + 1) * sizeof(char)))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

                    // Output debug message
                    sprintf(dbg_msg, dbg, proc_info.pid);
                    debug_message(dbg_msg);

                    // Clean up
                    free(dbg_msg);
                }
            }
#endif // #ifdef DEBUG
    }

    // Return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
 * Pause the shell until a specified key is pressed. Terminal echo will be
 * turned off and a specified prompt will be displayed.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int pause(void) {
    struct termios old; // structure containing old terminal information
    struct termios new; // structure containing new terminal information
    FILE * tty; // pointer to the tty input device
    char current_character; // the last key pressed

    if (proc_info.dont_wait) {
        err("Background execution is not supported for this command. Ignoring this parameter.");
    }
    if (input_redir) {
        err("Input redirection is not supported for this command. Ignoring this parameter.");
    }
    if (output_redir) {
        err("Output redirection is not supported for this command. Ignoring this parameter.");
    }

    // Output pause message
    printf("%s", PAUSE_MESSAGE);

    if (!(tty = fopen(ctermid(NULL), "r"))) sys_err("fopen"); // attempt to open tty device
    setbuf(tty, NULL); // set the standard input stream unbuffered

    if (tcgetattr(fileno(tty), &old)) sys_err("tcgetattr"); // attempt to save tty state
    new = old; // copy the structure

    /*
     * Disables (note the tilde) the following:
     *   - ECHO     Echo input characters
     *   - ECHOE    Echo erase characters as backspace-space-backspace
     *   - ECHOK    Echo a newline after a kill character
     *   - ECHONL   Echo newline even if not echoing other characters
     *   - ICANON   Enable erase, kill, werase, and rprnt special characters
     */
    new.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new)) sys_err("tcsetattr"); // change to the new terminal state

    // Pause until the specified key is pressed
    while ((current_character = getc(tty)) != EXIT_PAUSE_CHARACTER);

    if (tcsetattr(fileno(tty), TCSAFLUSH, &old)) sys_err("tcsetattr"); // attempt to restore TTY state
    printf("\n");

#ifdef DEBUG
    if (debug) {
        // Create debug message
        const char dbg[] = "Escape character '%c' detected. Resuming shell.";
        char * dbg_msg;

        // Memory allocation
        if (!(dbg_msg = (char *) malloc((strlen(dbg) + 1 + 1) * sizeof(char)))) sys_err("malloc"); // attempt to allocate memory for dbg_msg

        // Output debug message
        sprintf(dbg_msg, dbg, EXIT_PAUSE_CHARACTER);
        debug_message(dbg_msg);

        // Clean up
        free(dbg_msg);
    }

#endif // #ifdef DEBUG
    if (fclose(tty)) sys_err("fclose"); // attempt to close /dev/tty

    // Return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
 * Quit the shell.
 *
 * RETURN VALUE
 * An exit status indicating to the shell what action should be taken.
 */
int quit(void) {
    if (proc_info.dont_wait) {
        err("Background execution is not supported for this command. Ignoring this parameter.");
    }
    if (input_redir) {
        err("Input redirection is not supported for this command. Ignoring this parameter.");
    }
    if (output_redir) {
        err("Output redirection is not supported for this command. Ignoring this parameter.");
    }

    // Return an exit status indicating to the shell that it should quit
    return EXIT_STATUS_QUIT;
}
