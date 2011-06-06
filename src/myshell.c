/*
    myshell.c

    Author: Joshua Spence
    SID:    308216350
*/

#include    "../inc/myshell.h"

process_information proc_info;                                  // information about child processes
FILE* input_redir;                                              // file for input (stdin if null)
FILE* output_redir;                                             // file for output (stdout if null)
char *path;                                                     // path to the executable
#ifdef DEBUG

boolean debug;                                                  // is debug mode on?
#endif

/*
    ===========================================================
    main
    ===========================================================
    Runs the 'myshell' shell.
    -----------------------------------------------------------
    argc:       Number of arguments.
    argv:       Pointer to argument array.
    -----------------------------------------------------------
    return value:
                0 on success.
    ------------------------------------------------------------
*/
int main(int argc, char **argv) {
    FILE *input;                                                // the source of the command inputs
    boolean display_prompt;                                     // should the prompt be displayed?
    
    char *input_buffer = NULL;                                  // line buffer
    char *args[MAX_ARGS];                                       // pointers to argument strings
    char **arg;                                                 // working pointer through arguments
    unsigned int num_args;                                      // number of arguments entered into prompt
    
    int return_val;                                             // return value of last internal command call
    
    char *cwd ;                                                 // current working directory
    
    signal(SIGINT, SIG_IGN); /* disable SIGINT to prevent shell from terminating with Ctrl+C */
    signal(SIGCHLD, SIG_IGN); /* prevent zombie children */
    
    // check for batch file input
    if (argc > 1) {
        if (argc > 2) {
        // too many arguments were entered
            err("Too many arguments were specified. Some arguments will be ignored.");
        }
        
        if (argv[1]) {
        // batch file was specified
#ifdef DEBUG
            if (debug) {
                const char msg[] = "Input batch file '%s' was specified. Input commands will be parsed from this file.";
                char *dbg_msg;

                // memory allocation
                if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + strlen(argv[1]) + 1 /* null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                // output debug message
                sprintf(dbg_msg, msg, argv[1]);
                debug_message(dbg_msg);

                // clean up
                free(dbg_msg);
            }

#endif
            if (!(input = fopen(argv[1], "r"))) sys_err("fopen"); /* attempt to open the input batch file */
            display_prompt = FALSE; /* don't display a prompt when reading input from a file */
        } else {
#ifdef DEBUG
            if (debug) {
                debug_message("No input batch file was specified. Input commands will be parsed from stdin.");
            }
                        
#endif
            input = stdin;
            display_prompt = TRUE;
        }
    } else {
        input = stdin;
        display_prompt = TRUE;
    }

    // get home directory
    const char const *home = getcwd(NULL, (size_t)0); /* getcwd will dynamically allocate memory for home */
    cwd = getcwd(NULL, (size_t)0); /* getcwd will dynamically allocate memory for cwd */
    
    // get path to executable and add it to the environment variables
    path = get_path(NULL); /* get path to the executable */
    if (setenv("shell", path, 1)) sys_err("setenv"); /* set the 'shell' environment variable to the path to the shell, overwriting any existing value */
    
    // keep reading input until "quit" command or EOF of stdin/redirected input
    while (!feof(input)) {
        reset_process_information();
        
        // getting current working directory
        free(cwd);
        cwd = getcwd(NULL, (size_t)0); /* getcwd will dynamically allocate memory for cwd */
        
        // output shell prompt if requred
        if (display_prompt) {
                output_shell_prompt(cwd);
        }
        
        // get input from stdin/batch file
        input_buffer = get_input(input_buffer, input);
                
#ifdef DEBUG
        if (debug) {
            debug_read_line_message(input_buffer);
        }
                                        
#endif
                // tokenize the input into args array
#ifdef DEBUG
        if (debug) {
            debug_message("Tokenizing input into array of arguments.");
        }
                
#endif
        arg = args;
        *arg++ = quoted_strtok(input_buffer, SEPARATORS, QUOTATION_MARKS);
        while ((*arg++ = quoted_strtok(NULL, SEPARATORS, QUOTATION_MARKS)));
        arg = args; /* point the arg variable back to the start of the arguments */

        // remove quotation marks from arguments
#ifdef DEBUG
        if (debug) {
            debug_message("Removing quotation marks from arguments.");
        }
                
#endif
        while (*arg) {
            remove_character(*arg++, '\"');
        }
        arg = args; /* point the arg variable back to the start of the arguments */
        
        // count the number of arguments
        num_args = 0;
        while (*arg++) {
            num_args++;                     
        }
        arg = args; /* point the arg variable back to the start of the arguments */
        
#ifdef DEBUG
        if(debug) {
            const char msg[] = "Tokenized input into %d arguments.";
            char *dbg_msg;

            // memory allocation
            if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + digits(num_args) + 1 /* null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

            // output debug message
            sprintf(dbg_msg, msg, num_args);
            debug_message(dbg_msg);

            // clean up
            free(dbg_msg);
        }
                        
#endif  
        check_for_dont_wait(args);
        check_for_input_redirection(args);
        check_for_output_redirection(args);
        
        // if anything was input, execute the commands
        if (*arg) {
#ifdef DEBUG
            if (debug) {
                debug_command_args_message(input_buffer, (const char **)args);
            }
            
            // check for debug mode change
            if (!strcmp(*arg, DEBUG_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
            
                // check for arguments
                if (*arg) {
                    // check for valid arguments
                    if (!strcmp(*arg, DEBUG_ON)) {
                        arg++; /* increment argument pointer as an argument has been used */

                        // turn debug mode on
                        return_val = debug_mode(TRUE);
                    } else if(!strcmp(*arg, DEBUG_OFF)) {
                        arg++; /* increment argument pointer as an argument has been used */

                        // turn debug mode off
                        return_val = debug_mode(FALSE);
                    } else {
                        error_unrecognised_argument(args[0], args[1]);
                    }
                } else {
                    error_no_argument(args[0]);
                }
            }
            else
#endif
        // check for internal commands
            // "clear" command
            if (!strcmp(*arg, CLEAR_SCREEN_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], CLEAR_SCREEN_CMD_NAME);
                }
#endif

                return_val = clear_screen();
#ifdef DEBUG

                if (debug) {
                    debug_command_executed_message(args[0], CLEAR_SCREEN_CMD_NAME);
                }
#endif
            }
                    
            // "dir" command
            else if (!strcmp(*arg, LIST_DIRECTORY_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], LIST_DIRECTORY_CMD_NAME);
                }
#endif

                return_val = list_directory(*arg++ /* increment argument pointer as an argument has been used */);
#ifdef DEBUG

                if (debug) {
                    debug_command_executed_message(args[0], LIST_DIRECTORY_CMD_NAME);
                }
#endif
            }

            // "environ" command
            else if (!strcmp(*arg, PRINT_ENVIRONMENT_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], PRINT_ENVIRONMENT_CMD_NAME);
                }
#endif

                return_val = print_environment();
#ifdef DEBUG

                if (debug) {
                    debug_command_executed_message(args[0], PRINT_ENVIRONMENT_CMD_NAME);
                }
#endif
            }
                    
            // "cd" command
            else if (!strcmp(*arg, CHANGE_DIRECTORY_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], CHANGE_DIRECTORY_CMD_NAME);
                }
#endif

                return_val = change_directory(*arg++ /* increment argument pointer as an argument has been used */);
#ifdef DEBUG

                if (debug) {
                        debug_command_executed_message(args[0], CHANGE_DIRECTORY_CMD_NAME);
                }
#endif
            }

            // "echo" command
            else if (!strcmp(*arg, ECHO_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], ECHO_CMD_NAME);
                }
#endif

                return_val = echo((const char **)arg);
                                
                while(*arg++); /* increment argument pointer as all arguments have been used */
#ifdef DEBUG

                if (debug) {
                    debug_command_executed_message(args[0], ECHO_CMD_NAME);
                }
#endif
            }

            // "help" command
            else if (!strcmp(*arg, HELP_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], HELP_CMD_NAME);
                }
#endif

                return_val = help(home);
#ifdef DEBUG

                if (debug) {
                    debug_command_executed_message(args[0], HELP_CMD_NAME);
                }
#endif
            }

            // "pause" command
            else if (!strcmp(*arg, PAUSE_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], PAUSE_CMD_NAME);
                }
#endif

                return_val = pause();
#ifdef DEBUG

                if (debug) {
                    debug_command_executed_message(args[0], PAUSE_CMD_NAME);
                }
#endif
            }

            // "quit" command
            else if (!strcmp(*arg, QUIT_COMMAND)) {
                arg++; /* increment argument pointer as an argument has been used */
#ifdef DEBUG
                if (debug) {
                    debug_command_recognised_message(args[0], QUIT_CMD_NAME);
                }
#endif

                return_val = quit();
#ifdef DEBUG

                if (debug) {
                    debug_command_executed_message(args[0], QUIT_CMD_NAME);
                }
#endif
            }
                    
            // else pass command to external shell
            else {
#ifdef DEBUG
                if (debug) {
                    // create debug message
                    const char msg[] = "Command '%s' not recognised internally, passing to system shell.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + strlen(*arg) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, *arg);
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }
                                
#endif

                                return_val = process_external_command(arg);
            }
        }
        
        // close input file
        if (input_redir) {
#ifdef DEBUG
            if (debug) {
                debug_message("Closing input file.");
            }
                        
#endif
            if (fclose(input_redir)) sys_err("fclose"); /* attempt to close the input file */
            input_redir = NULL;
#ifdef DEBUG

            if (debug) {
                debug_message("Closed input file.");
            }

#endif
                }
                        
                // close output file if necessary
        if (output_redir) {
#ifdef DEBUG
            if (debug) {
                debug_message("Closing output file.");
            }
            
#endif
            if (fclose(output_redir)) sys_err("fclose"); /* attempt to close the output file */
            output_redir = NULL;
#ifdef DEBUG

            if (debug) {
                debug_message("Closed ouput file.");
            }
                        
#endif
                }
                
                // check if the shell should quit
        if (return_val == EXIT_STATUS_QUIT) {
                break;
        }       
    }

    // clean up
    if (fclose(input)) sys_err("fclose"); /* attempt to close the input batch file */
    input = NULL;
    free((void *)home); /* free the memory dynamically allocated by getcwd */
    free(cwd); /* free the memory dynamically allocated by getcwd */
    free(path); /* free the memory dynamically allocated by get_path */
    free(input_buffer); /* free the memory dynamically allocated by get_input */
    
    return 0;
}

/*
    ===========================================================
    output_shell_prompt
    ===========================================================
    This function outputs to the terminal a prompt indicating
    that the shell is waiting for user input.
    -----------------------------------------------------------
    path:       A null-terminated string containing the path
                that will appear in the shell prompt.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void output_shell_prompt(const char *path) {
    char *prompt;                                               // the shell prompt to display
    
    // allocate space for shell prompt
#ifdef DEBUG
    if (!(prompt = (char *)malloc((size_t)((strlen(DEBUG_PROMPT) + strlen(path) + strlen(PROMPT_SUFFIX) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for prompt */
#else
    if (!(prompt = (char *)malloc((size_t)((strlen(path) + strlen(PROMPT_SUFFIX) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for prompt */
#endif
        
    // generate the shell prompt
#ifdef DEBUG
    if (debug) {
        sprintf(prompt, "%s%s%s", DEBUG_PROMPT, path, PROMPT_SUFFIX);
    } else {
        sprintf(prompt, "%s%s", path, PROMPT_SUFFIX);
    }
#else
    sprintf(prompt, "%s%s", path, PROMPT_SUFFIX);
#endif

    // write prompt
    printf("%s", prompt);
    
    // clean up
    free(prompt);
}

/*
    ===========================================================
    process_external_command
    ===========================================================
    This function passes an unrecognised command to the system
    for processing.
    -----------------------------------------------------------
    args:       A pointer to an array of character strings.
                MUST be terminated by a null entry.
    ------------------------------------------------------------
    return value:
                An exit status indicating to the shell what
                action should be taken.
    ------------------------------------------------------------
*/
int process_external_command(char **args) {     
    // fork the current process
    switch(proc_info.pid = fork()) {
        case -1: /* fork failed */
            sys_err("fork");
            break;
                
        case 0: /* child */
#ifdef DEBUG
            if (debug) {
                debug_message("Setting 'parent' environment variable in child process.");
            }
                        
#endif
            // set environment variable
            if (setenv("parent", path, 1)) sys_err("setenv"); /* set the 'parent' environment variable to the path to the shell, overwriting any existing value */
    
#ifdef DEBUG
            if (debug) {
                // create debug message
                const char msg[] = "Attempting to execute '%s' in child process.";
                char *dbg_msg;

                // memory allocation
                if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + strlen(*args) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                // output debug message
                sprintf(dbg_msg, msg, *args);
                debug_message(dbg_msg);

                // clean up
                free(dbg_msg);
            }
                        
#endif
            // redirect input if necessary
            if (input_redir) {
                    dup2(fileno(input_redir), STDIN_FILENO);
            }
            
            // redirect output if necessary 
            if (output_redir) {
                    dup2(fileno(output_redir), STDOUT_FILENO);
            }
            
            // execute the command with the appropriate arguments */
            execvp(*args, args);
            
            // if execution reaches this line, an error has occured as execvp should never return
            const char msg[] = "execvp [%s]";
            char *err_msg;
            
            // memory allocation
            if (!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(*args) + 1 /* null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */
            
            // output error message
            sprintf(err_msg, msg, *args);
            sys_err(err_msg);
            
            // clean up
            free(err_msg);
            
            break;
                                                
        default: /* parent */
            if (!proc_info.dont_wait) {
#ifdef DEBUG
                if (debug) {
                    // create debug message
                    const char msg[] = "Parent process waiting for child process [PID: %d] to return.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + digits(proc_info.pid) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, proc_info.pid);
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }
                                
#endif
                // wait for the child process to return
                waitpid(proc_info.pid, &proc_info.status, WUNTRACED);
#ifdef DEBUG

                if (debug) {
                    // create debug message
                    const char msg[] = "Child process %d has returned with status: %d.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + digits(proc_info.pid) + digits(proc_info.status) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, proc_info.pid, proc_info.status);
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }
#endif
            }
#ifdef DEBUG
            else {
                if (debug) {
                    // create debug message
                    const char msg[] = "Parent process continuing without waiting for child process [PID: %d] to return.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + digits(proc_info.pid) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, proc_info.pid);
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }
            }
#endif
    }
    
    // return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
    ===========================================================
    get_input
    ===========================================================
    This function gets input from the user. It allocates and
    reallocates memory for the input_buffer if the current
    input_buffer is not large enough.
    
    The memory allocated by this function must be freed by the
    caller.
    -----------------------------------------------------------
    input_buffer:
                A buffer in which the input will be stored.
                this can be null.
    ------------------------------------------------------------
    return value:
                A pointer to a null-terminated string containing
                the input.
    ------------------------------------------------------------
*/
char *get_input(char *input_buffer, FILE *input) {
    size_t size = 0;                                            // current size of input_buffer
    size_t length = 0;                                          // number of characters in input_buffer
    
    // keep getting input using fgets until the input_buffer is large enough
    do {
        size += ALLOCATION_BLOCK;
        if (!(input_buffer = (char *)realloc(input_buffer, size))) sys_err("realloc"); /* realloc(NULL,n) is the same as malloc(n) */            
    
        // Actually do the read. Note that fgets puts a terminal '\0' on the end of the string, so we make sure we overwrite this 
        fgets(input_buffer + length, size - length, input);
            
        length = strlen(input_buffer);
    } while (!feof(input) && !strchr(input_buffer, '\n'));
     
    return input_buffer;
}


/*
    ===========================================================
    check_for_dont_wait
    ===========================================================
    This function and look at the last argument for the don't 
    wait character (defined in strings.h). Upon     finding the 
    don't wait character, the function will set a flag and set this argument to NULL.
    
    Note that the don't wait character should be the last 
    argument of the argument array. No check is made for this, 
    but all arguments after the don't wait character will be 
    ignored.
    -----------------------------------------------------------
    args:       A pointer to an array of character strings.
                MUST be terminated by a null element.
    -----------------------------------------------------------
    no return value
    -----------------------------------------------------------
*/
void check_for_dont_wait(char **args) {
    char **arg = args;                                          // working pointer through args
    unsigned int count_args = 0;                                // number of arguments
    
    // count arguments
    while(*arg++) {
        count_args++;
    }
    
    /* arg now points to the element AFTER the last null element */
            
    if (count_args >= 1) {
        arg = arg - 2; /* point arg to the last argument of the array */
            
#ifdef DEBUG
        if (debug) {
            // create debug message
            const char msg[] = "Checking for don't wait character '%c'.";
            char *dbg_msg;

            // memory allocation
            if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for dont wait character */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

            // output debug message
            sprintf(dbg_msg, msg, DONT_WAIT_CHARACTER);
            debug_message(dbg_msg);

            // clean up
            free(dbg_msg);
        }
        
#endif
        // check for dont_wait character
        if ((strlen(*arg) == 1) && ((*arg)[0] == DONT_WAIT_CHARACTER)) {
#ifdef DEBUG
            if (debug) {
                // create debug message
                const char msg[] = "Found the character '%c'. This command will be executed in the background.";
                char *dbg_msg;

                // memory allocation
                if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for dont wait character */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                // output debug message
                sprintf(dbg_msg, msg, DONT_WAIT_CHARACTER);
                debug_message(dbg_msg);

                // clean up
                free(dbg_msg);
            }
                
#endif
            proc_info.dont_wait = TRUE; /* set the don't wait flag */
                
#ifdef DEBUG
            if (debug) {
                // create debug message
                const char msg[] = "Removing '%c' from argument list.";
                char *dbg_msg;

                // memory allocation
                if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for dont wait character */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                // output debug message
                sprintf(dbg_msg, msg, DONT_WAIT_CHARACTER);
                debug_message(dbg_msg);

                // clean up
                free(dbg_msg);
            }
                
#endif
            // remove argument from array
            *(array_movetoend(arg)) = 0;
        }
    }
#ifdef DEBUG
    else {
    // not enough arguments
        if (debug) {
            debug_message("Not enough arguments for there to be a don't wait character. Skipping this check.");
        }
    }
#endif
}

/*
    ===========================================================
    check_for_input_redirection
    ===========================================================
    This function loops through the argument array and looks
    for the input redirection character (defined in strings.h)
    and input redirection file. Upon finding these arguments, 
    the function will set a flag and set these argument to
    NULL.
    
    Note that the don't wait character should be the last 
    argument of the argument array. No check is made for this, 
    but all arguments after the don't wait character will be 
    ignored.
    -----------------------------------------------------------
    args:       A pointer to an array of character strings.
                MUST be terminated by a null element.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void check_for_input_redirection(char **args) {
    char **arg = args;                                          // working pointer through arguments
    
#ifdef DEBUG
    if(debug) {
        // create debug message
        const char msg[] = "Checking for input redirection character '%c'.";
        char *dbg_msg;

        // memory allocation
        if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for input redirection character */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */
        
        // output debug message
        sprintf(dbg_msg, msg, INPUT_REDIRECTION_CHAR);
        debug_message(dbg_msg);
        
        // clean up
        free(dbg_msg);
    }
        
#endif
    // look for input redirection character
    while (*arg) {
        if ((strlen(*arg) == 1) && ((*arg)[0] == INPUT_REDIRECTION_CHAR)) {
        // input redirection has been specified
#ifdef DEBUG
            if (debug) {
                // create debug message
                const char msg[] = "Found the character '%c'. stdin will be directed from a file.";
                char *dbg_msg;

                // memory allocation
                if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for input redirection character */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                // output debug message
                sprintf(dbg_msg, msg, INPUT_REDIRECTION_CHAR);
                debug_message(dbg_msg);

                // clean up
                free(dbg_msg);
            }
    
#endif
            if (*(arg + 1)) {
            // input file was specified
#ifdef DEBUG
                if (debug) {
                    // create debug message
                    const char msg[] = "Found an argument after the character '%c'. stdin will be directed from this file '%s'.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for input redirection character */ + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, INPUT_REDIRECTION_CHAR, *(arg + 1));
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }
                
#endif
                // check if the file exists
                if (!access(*(arg + 1), R_OK)) {
                    if (!(input_redir = fopen(*(arg + 1), "r"))) sys_err("fopen"); /* attempt to open the file for reading */
                } else {
                    // create error message
                    const char msg[] = "Unable to open the file '%s' for reading. stdin will not be redirected.";
                    char *err_msg;

                    // memory allocation
                    if (!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output error message
                    sprintf(err_msg, msg, *(arg + 1));
                    err(err_msg);

                    // clean up
                    free(err_msg);
                }
                                
                // remove arguments
#ifdef DEBUG
                if(debug) {
                    // create debug message
                    const char msg[] = "Removing '%c %s' from argument list.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for input redirection character */ + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, INPUT_REDIRECTION_CHAR, *(arg + 1));
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }
                
#endif
                *(array_movetoend(arg + 1)) = 0; /* move input redirection file to end of argument array and set to null */
                *(array_movetoend(arg)) = 0; /* move input redirection character to end of argument array and set to null */
            } else {
            // input file not specified
                const char input_redirection_string[2]= {INPUT_REDIRECTION_CHAR, '\0'};
                error_no_argument(input_redirection_string);
            }
            break;
        }
        arg++;
    }
}

/*
    ===========================================================
    check_for_output_redirection
    ===========================================================
    This function loops through the argument array and looks
    for the input redirection character (defined in strings.h)
    and input redirection file. Upon finding these arguments, 
    the function will set a flag and set these argument to
    NULL.
    
    Note that the don't wait character should be the last 
    argument of the argument array. No check is made for this, 
    but all arguments after the don't wait character will be 
    ignored.
    ------------------------------------------------------------
    args:       A pointer to an array of character strings.
                MUST be terminated by a zero element.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void check_for_output_redirection(char **args) {
    char **arg = args;                                          // working pointer through arguments
        
#ifdef DEBUG
    if (debug) {
        // create debug message
        const char msg[] = "Checking for output redirection character '%c'.";
        char *dbg_msg;

        // memory allocation
        if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for output redirection character */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */
        
        // output debug message
        sprintf(dbg_msg, msg, OUTPUT_REDIRECTION_CHAR);
        debug_message(dbg_msg);
        
        // clean up
        free(dbg_msg);
    }
        
#endif
    // look for output redirection character
    while (*arg) {
        if ((strlen(*arg) == 1) && ((*arg)[0] == OUTPUT_REDIRECTION_CHAR)) {
        // output redirection (truncate) has been specified
#ifdef DEBUG
            if (debug) {
                // create debug message
                const char msg[] = "Found the character '%c'. stdout will be directed to a file.";
                char *dbg_msg;

                // memory allocation
                if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for output redirection character */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                // output debug message
                sprintf(dbg_msg, msg, OUTPUT_REDIRECTION_CHAR);
                debug_message(dbg_msg);

                // clean up
                free(dbg_msg);
            }
    
#endif
            if (*(arg + 1)) {
            // output file was specified
#ifdef DEBUG
                if (debug) {
                        // create debug message
                        const char msg[] = "Found an argument after the character '%c'. stdout will be directed to this file '%s'.";
                        char *dbg_msg;

                        // memory allocation
                        if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for output redirection character */ + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                        // output debug message
                        sprintf(dbg_msg, msg, OUTPUT_REDIRECTION_CHAR, *(arg + 1));
                        debug_message(dbg_msg);

                        // clean up
                        free(dbg_msg);
                }

#endif
                if (!(output_redir = fopen(*(arg + 1), "w"))) sys_err("fopen"); /* attempt to open the file for writing */
                
                // remove arguments
#ifdef DEBUG
                if (debug) {
                    // create debug message
                    const char msg[] = "Removing '%c %s' from argument list.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 1 /* for output redirection character */ + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, OUTPUT_REDIRECTION_CHAR, *(arg + 1));
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }

#endif
                *(array_movetoend(arg + 1)) = 0; /* move output redirection file to end of argument array and set to null */
                *(array_movetoend(arg)) = 0; /* move output redirection character to end of argument array and set to null */
            } else {
            // output file not specified
                const char output_redirection_string[2] = {OUTPUT_REDIRECTION_CHAR, '\0'};
                error_no_argument(output_redirection_string);
            }
            break;
        } else if((strlen(*arg) == 2) && ((*arg)[0] == OUTPUT_REDIRECTION_CHAR) && ((*arg)[1] == OUTPUT_REDIRECTION_CHAR)) {
        // output redirection (append) has been specified
#ifdef DEBUG
            if (debug) {
                // create debug message
                const char msg[] = "Found the characters '%c%c'. stdout will be appended to a file.";
                char *dbg_msg;

                // memory allocation
                if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 2 /* for output redirection characters */ + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                // output debug message
                sprintf(dbg_msg, msg, OUTPUT_REDIRECTION_CHAR, OUTPUT_REDIRECTION_CHAR);
                debug_message(dbg_msg);

                // clean up
                free(dbg_msg);
            }
        
#endif
            if (*(arg + 1)) {
            // output file was specified
#ifdef DEBUG
                if (debug) {
                    // create debug message
                    const char msg[] = "Found an argument after the characters '%c%c'. stdout will be appended to this file '%s'.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 2 /* for output redirection characters */ + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, INPUT_REDIRECTION_CHAR, INPUT_REDIRECTION_CHAR, *(arg + 1));
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }

#endif
                if (!(output_redir = fopen(*(arg + 1), "a"))) sys_err("fopen"); /* attempt to open the file for writing */
                
                // remove arguments
#ifdef DEBUG
                if(debug) {
                    // create debug message
                    const char msg[] = "Removing '%c%c %s' from argument list.";
                    char *dbg_msg;

                    // memory allocation
                    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + 2 /* for output redirection characters */ + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

                    // output debug message
                    sprintf(dbg_msg, msg, OUTPUT_REDIRECTION_CHAR, OUTPUT_REDIRECTION_CHAR, *(arg + 1));
                    debug_message(dbg_msg);

                    // clean up
                    free(dbg_msg);
                }

#endif
                *(array_movetoend(arg + 1)) = 0; /* move output redirection file to end of argument array and set to null */
                *(array_movetoend(arg)) = 0; /* move output redirection character to end of argument array and set to null */
            } else {
            // output file not specified
                    const char output_redirection_string[2] = {OUTPUT_REDIRECTION_CHAR, '\0'};
                    error_no_argument(output_redirection_string);
            }
            break;
        }
        arg++;
    }
}

/*
    ===========================================================
    error_no_argument
    ===========================================================
    Display an error message that a command requiring an 
    argument has been executed without an argument.
    ------------------------------------------------------------
    command:    A null-terminated string containing the 
                command for which arguments were required but
                not specified.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void error_no_argument(const char *command) {
    // create error message
    const char msg[] = "No argument found for command '%s'.";
    char *err_msg;
    
    // memory allocation
    if (!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(command) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for err_msg */
    
    // output error message
    sprintf(err_msg, msg, command);
    err(err_msg);
    
    // clean up
    free(err_msg);
}

/*
    ===========================================================
    error_unrecognised_argument
    ===========================================================
    Display an error message that a command has been specified
    with an unknown argument.
    ------------------------------------------------------------
    command:    A null-terminated string containing the 
                command for which an argument was unrecognised.
    arg:        A null-terminated string containg the 
                unrecognised argument.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void error_unrecognised_argument(const char *command, const char *arg) {
    // create error message
    const char msg[] = "Unrecognised argument to command '%s': '%s'.";
    char *err_msg;
    
    // memory allocation
    if (!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(command) + strlen(arg) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for err_msg */
    
    // output error message
    sprintf(err_msg, msg, command, arg);
    err(err_msg);
    
    // clean up
    free(err_msg);
}

/*
    ===========================================================
    reset_process_information
    ===========================================================
    Reset the process information.
    ------------------------------------------------------------
    no input arguments
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void reset_process_information(void) {
    // reset process information
    proc_info.pid = 0;
    proc_info.dont_wait = FALSE; /* by default, run commands in the foreground */
    proc_info.status = 0;
}

#ifdef DEBUG
/*
    ===========================================================
    debug_mode
    ===========================================================
    Turns debug mode on or off.
    ------------------------------------------------------------
    state:      The boolean state to set debug mode to.
    ------------------------------------------------------------
    return value:
                An exit status indicating to the shell what
                action should be taken.
    ------------------------------------------------------------
*/
int debug_mode(const boolean state) {
    if (proc_info.dont_wait) {
        err("Background execution is not supported for this command. Ignoring this parameter.");
    }
    if (input_redir) {
        err("Input redirection is not supported for this command. Ignoring this parameter.");
    }
    if (output_redir) {
        err("Output redirection is not supported for this command. Ignoring this parameter.");
    }
    
    if (state) {
        debug = TRUE;
        debug_message("Debug mode on.");
    }
    else {
        debug = FALSE;
        debug_message("Debug mode off.");
    }
    
    // return an exit status indicating to the shell that it should continue executing
    return EXIT_STATUS_CONTINUE;
}

/*
    ===========================================================
    debug_command_recognised_message
    ===========================================================
    Display a debug message indicating that a command has been
    recognised.
    ------------------------------------------------------------
    command:    The command that was recognised.
    command_name:
                The full name of the command that was 
                recognised.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void debug_command_recognised_message(const char *command, const char *command_name) {
    // create debug message
    const char msg[] = "%s command '%s' recognised.";
    char *dbg_msg;

    // memory allocation
    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + strlen(command) + strlen(command_name) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */
    
    // output debug message
    sprintf(dbg_msg, msg, command_name, command);
    debug_message(dbg_msg);
    
    // clean up
    free(dbg_msg);
}

/*
    ===========================================================
    debug_command_executed_message
    ===========================================================
    Display a debug message indicating that a command has been
    executed.
    ------------------------------------------------------------
    command:    The command that was executed.
    command_name:
                The full name of the command that was 
                executed.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void debug_command_executed_message(const char *command, const char *command_name) {
    // create debug message
    const char msg[] = "%s command '%s' executed.";
    char *dbg_msg;

    // memory allocation
    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + strlen(command) + strlen(command_name) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */
    
    // output debug message
    sprintf(dbg_msg, msg, command_name, command);
    debug_message(dbg_msg);
    
    // clean up
    free(dbg_msg);
}

/*
    ===========================================================
    debug_read_line_message
    ===========================================================
    Display a debug message showing the input that has been 
    read.
    ------------------------------------------------------------
    buffer:     A null-terminated string containing the text
                that has been read.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void debug_read_line_message(const char *buffer) {
    // create debug message
    const char msg[] = "Read line: '%.*s'.";
    char *dbg_msg;

    // memory allocation
    if (!(dbg_msg = (char *)malloc((size_t)((strlen(msg) + (strlen(buffer) - 1 /* for new line character which will not be output */) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

    // output debug message
    sprintf(dbg_msg, msg, (int)(strlen(buffer) - 1 /* don't print new-line character */), buffer);
    debug_message(dbg_msg);

    // clean up
    free(dbg_msg);
}

/*
    ===========================================================
    debug_command_args_message
    ===========================================================
    Display a debug message showing the command and arguments
    that have been recognised and will be processed.
    ------------------------------------------------------------
    command:    The command that will be processed.
    args:       A pointer to the first argument that will be
                processed.
    ------------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void debug_command_args_message(const char *command, const char **args) {
    const char **arg = args;                                    // working pointer through arguments
    size_t size = 0;                                            // current size of dbg_msg
    
    // create debug message
    const char msg[] = "Command '%s' was input.\n";
    const char args_msg[] = "%sArgument %d: '%s'\n";
    char *dbg_msg;
    
    // memory allocation
    if (!(dbg_msg = (char *)malloc((size_t)(size += ((strlen(msg) + strlen(command) + 1 /* for null character */) * sizeof(char)))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

    // output debug message
    sprintf(dbg_msg, msg, command);
    unsigned int i = 1;
    arg = args;
    while (*(++arg)) {
        if (!(dbg_msg = (char *)realloc(dbg_msg, (size_t)(size += ((strlen(args_msg) + digits(i) + strlen(*arg)) * sizeof(char)))))) sys_err("realloc"); /* attempt to reallocate memory for dbg_msg */
        sprintf(dbg_msg, args_msg, dbg_msg, i++, *arg);
    }
    debug_message(dbg_msg);

    // clean up
    free(dbg_msg);
}
#endif
