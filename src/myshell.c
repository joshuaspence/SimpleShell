/*
    myshell.c

    Author:	Joshua Spence
	SID:	308216350
*/

#include	"../inc/myshell.h"

process_information proc_info;									// information about child processes
FILE* input_redir;												// file for input (stdin if null)
FILE* output_redir;												// file for output (stdout if null)
char *path;														// path to the executable

/*
	===========================================================
	main
	===========================================================
	Runs the 'myshell' shell.
	-----------------------------------------------------------
	argc:		Number of arguments.
	argv:		Pointer to argument array.
	-----------------------------------------------------------
	return value:
				0 on success.
	------------------------------------------------------------
*/
int main(int argc, char **argv) {
	FILE *input;												// the source of the command inputs
	boolean display_prompt;										// should the prompt be displayed?
	
	char *input_buffer = NULL;                        			// line buffer
	char *args[MAX_ARGS];										// pointers to argument strings
	char **arg;													// working pointer through arguments
	unsigned int num_args;										// number of arguments entered into prompt
	
    int return_val;												// return value of last internal command call
	
	char *cwd ;                             					// current working directory
	
	signal(SIGINT, SIG_IGN); /* disable SIGINT to prevent shell from terminating with Ctrl+C */
	
	// check for batch file input
	if(argc > 1) {
		if(argc > 2)
		// too many arguments were entered
			err("Too many arguments were specified. Some arguments will be ignored.");

		if(argv[1]) {
		// batch file was specified
			if(!(input = fopen(argv[1], "r"))) sys_err("fopen"); /* attempt to open the input batch file */
			display_prompt = FALSE; /* don't display a prompt when reading input from a file */
		}
		else {
			input = stdin;
			display_prompt = TRUE;
		}
	}
	else {
		input = stdin;
		display_prompt = TRUE;
	}

	// get home directory
	const char const *home = getcwd(NULL, (size_t)0); /* getcwd will dynamically allocate memory for home */
	cwd = getcwd(NULL, (size_t)0); /* getcwd will dynamically allocate memory for cwd */
	
	// get path to executable and add it to the environment variables
	path = get_path(NULL); /* get path to the executable */
	if(setenv("shell", path, 1)) sys_err("setenv"); /* set the 'shell' environment variable to the path to the shell, overwriting any existing value */
	
	// keep reading input until "quit" command or EOF of stdin/redirected input
    while(!feof(input)) {
		reset_process_information();
		
		// getting current working directory
		free(cwd);
		cwd = getcwd(NULL, (size_t)0); /* getcwd will dynamically allocate memory for cwd */
		
		// output shell prompt if requred
		if(display_prompt)
			output_shell_prompt(cwd);
		
		// get input from stdin/batch file
		input_buffer = get_input(input_buffer, input);
			
		// tokenize the input into args array
		arg = args;
		*arg++ = quoted_strtok(input_buffer, SEPARATORS, QUOTATION_MARKS);
		while((*arg++ = quoted_strtok(NULL, SEPARATORS, QUOTATION_MARKS)));
		arg = args; /* point the arg variable back to the start of the arguments */

		// remove quotation marks from arguments
		while(*arg)
			remove_character(*arg++, '\"');
		arg = args; /* point the arg variable back to the start of the arguments */
		
		// count the number of arguments
		num_args = 0;
		while(*arg++)
			num_args++;			
		arg = args; /* point the arg variable back to the start of the arguments */
		
		check_for_dont_wait(args);
		check_for_input_redirection(args);
		check_for_output_redirection(args);
		
		// if anything was input, execute the commands
		if(*arg) {
		// check for internal commands
			// "clear" command
			if(!strcmp(*arg, CLEAR_SCREEN_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = clear_screen();
			}
				
			// "dir" command
			else if(!strcmp(*arg, LIST_DIRECTORY_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = list_directory(*arg++ /* increment argument pointer as an argument has been used */);
			}

			// "environ" command
			else if(!strcmp(*arg, PRINT_ENVIRONMENT_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = print_environment();
			}
				
			// "cd" command
			else if(!strcmp(*arg, CHANGE_DIRECTORY_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = change_directory(*arg++ /* increment argument pointer as an argument has been used */);
			}

			// "echo" command
			else if(!strcmp(*arg, ECHO_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = echo((const char **)arg);
				
				while(*arg++); /* increment argument pointer as all arguments have been used */
			}

			// "help" command
			else if(!strcmp(*arg, HELP_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = help(home);
			}

			// "pause" command
			else if(!strcmp(*arg, PAUSE_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = pause();
			}

			// "quit" command
			else if(!strcmp(*arg, QUIT_COMMAND)) {
				arg++; /* increment argument pointer as an argument has been used */

				return_val = quit();
			}
				
			// else pass command to external shell
			else {

				return_val = process_external_command(arg);
			}
		}
		
		// close input file
		if(input_redir) {
			if(fclose(input_redir)) sys_err("fclose"); /* attempt to close the input file */
			input_redir = NULL;
		}
			
		// close output file if necessary
		if(output_redir) {
			if(fclose(output_redir)) sys_err("fclose"); /* attempt to close the output file */
			output_redir = NULL;
		}
		
		// check if the shell should quit
		if(return_val == EXIT_STATUS_QUIT)
			break;
    }

	// clean up
	if(fclose(input)) sys_err("fclose"); /* attempt to close the input batch file */
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
	path:		A null-terminated string containing the path
				that will appear in the shell prompt.
	------------------------------------------------------------
	no return value
	------------------------------------------------------------
*/
void output_shell_prompt(const char *path) {
	char *prompt;												// the shell prompt to display
	
	// allocate space for shell prompt
	if(!(prompt = (char *)malloc((size_t)((strlen(path) + strlen(PROMPT_SUFFIX) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for prompt */
	
	// generate the shell prompt
	sprintf(prompt, "%s%s", path, PROMPT_SUFFIX);

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
	args:		A pointer to an array of character strings.
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
			// set environment variable
			if(setenv("parent", path, 1)) sys_err("setenv"); /* set the 'parent' environment variable to the path to the shell, overwriting any existing value */
		
			// redirect input if necessary
			if(input_redir)
				dup2(fileno(input_redir), STDIN_FILENO);
			
			// redirect output if necessary 
			if(output_redir)
				dup2(fileno(output_redir), STDOUT_FILENO);

			// execute the command with the appropriate arguments */
			execvp(*args, args);
			
			// if execution reaches this line, an error has occured as execvp should never return
			const char msg[] = "execvp [%s]";
			char *err_msg;
			
			// memory allocation
			if(!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(*args) + 1 /* null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */
			
			// output error message
			sprintf(err_msg, msg, *args);
			sys_err(err_msg);
			
			// clean up
			free(err_msg);
			
			break;
							
		default: /* parent */
			if(!proc_info.dont_wait) {
				// wait for the child process to return
				waitpid(proc_info.pid, &proc_info.status, WUNTRACED);
			}
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
	size_t size = 0;											// current size of input_buffer
    size_t length = 0;											// number of characters in input_buffer
	
	// keep getting input using fgets until the input_buffer is large enough
	do {
        size += ALLOCATION_BLOCK;
        if(!(input_buffer = (char *)realloc(input_buffer, size))) sys_err("realloc"); /* realloc(NULL,n) is the same as malloc(n) */            
        
		// Actually do the read. Note that fgets puts a terminal '\0' on the end of the string, so we make sure we overwrite this 
        fgets(input_buffer + length, size - length, input);
		
        length = strlen(input_buffer);
    } while(!feof(input) && !strchr(input_buffer, '\n'));

	// make sure command is valid (terminated by new-line character). if input_buffer doesn't contain a new-line character then the command is a duplicate */
	/*if(feof(input) && !strchr(input_buffer, '\n'))
		memset(input_buffer, 0, size);*/
		
	return input_buffer;
}

/*
	===========================================================
	check_for_dont_wait
	===========================================================
	This function and look at the last argument for the don't 
	wait character (defined in strings.h). Upon	finding the 
	don't wait character, the function will set a flag and set this argument to NULL.
	
	Note that the don't wait character should be the last 
	argument of the argument array. No check is made for this, 
	but all arguments after the don't wait character will be 
	ignored.
	-----------------------------------------------------------
	args:		A pointer to an array of character strings.
				MUST be terminated by a null element.
	-----------------------------------------------------------
	no return value
	-----------------------------------------------------------
*/
void check_for_dont_wait(char **args) {
	char **arg = args;      									// working pointer through args
	unsigned int count_args = 0;								// number of arguments
	
	// count arguments
	while(*arg++)
		count_args++;
	
	/* arg now points to the element AFTER the last null element */
		
	if(count_args >= 1) {
		arg = arg - 2; /* point arg to the last argument of the array */
		
		// check for dont_wait character
		if((strlen(*arg) == 1) && ((*arg)[0] == DONT_WAIT_CHARACTER)) {
			proc_info.dont_wait = TRUE; /* set the don't wait flag */
		
			// remove argument from array
			*(array_movetoend(arg)) = 0;
		}
	}
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
	args:		A pointer to an array of character strings.
				MUST be terminated by a null element.
	------------------------------------------------------------
	no return value
	------------------------------------------------------------
*/
void check_for_input_redirection(char **args) {
	char **arg = args;									// working pointer through arguments
	
	// look for input redirection character
	while(*arg) {
		if((strlen(*arg) == 1) && ((*arg)[0] == INPUT_REDIRECTION_CHAR)) {
		// input redirection has been specified
			if(*(arg + 1)) {
			// input file was specified
				// check if the file exists
				if(!access(*(arg + 1), R_OK)) {
					if(!(input_redir = fopen(*(arg + 1), "r"))) sys_err("fopen"); /* attempt to open the file for reading */
				}
				else {
					// create error message
					const char msg[] = "Unable to open the file '%s' for reading. stdin will not be redirected.";
					char *err_msg;

					// memory allocation
					if(!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(*(arg + 1)) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for dbg_msg */

					// output error message
					sprintf(err_msg, msg, *(arg + 1));
					err(err_msg);

					// clean up
					free(err_msg);
				}
				
				// remove arguments
				*(array_movetoend(arg + 1)) = 0; /* move input redirection file to end of argument array and set to null */
				*(array_movetoend(arg)) = 0; /* move input redirection character to end of argument array and set to null */
			}
			else {
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
	args:		A pointer to an array of character strings.
				MUST be terminated by a zero element.
	------------------------------------------------------------
	no return value
	------------------------------------------------------------
*/
void check_for_output_redirection(char **args) {
	char **arg = args;										// working pointer through arguments
	
	// look for output redirection character
	while(*arg) {
		if((strlen(*arg) == 1) && ((*arg)[0] == OUTPUT_REDIRECTION_CHAR)) {
		// output redirection (truncate) has been specified
			if(*(arg + 1)) {
			// output file was specified
				if(!(output_redir = fopen(*(arg + 1), "w"))) sys_err("fopen"); /* attempt to open the file for writing */
				
				// remove arguments
				*(array_movetoend(arg + 1)) = 0; /* move output redirection file to end of argument array and set to null */
				*(array_movetoend(arg)) = 0; /* move output redirection character to end of argument array and set to null */
			}
			else {
			// output file not specified
				const char output_redirection_string[2] = {OUTPUT_REDIRECTION_CHAR, '\0'};
				error_no_argument(output_redirection_string);
			}
			break;
		}
		else if((strlen(*arg) == 2) && ((*arg)[0] == OUTPUT_REDIRECTION_CHAR) && ((*arg)[1] == OUTPUT_REDIRECTION_CHAR)) {
		// output redirection (append) has been specified
			if(*(arg + 1)) {
			// output file was specified
				if(!(output_redir = fopen(*(arg + 1), "a"))) sys_err("fopen"); /* attempt to open the file for writing */
				
				// remove arguments
				*(array_movetoend(arg + 1)) = 0; /* move output redirection file to end of argument array and set to null */
				*(array_movetoend(arg)) = 0; /* move output redirection character to end of argument array and set to null */
			}
			else {
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
	command:	A null-terminated string containing the 
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
	if(!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(command) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for err_msg */
	
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
	command:	A null-terminated string containing the 
				command for which an argument was unrecognised.
	arg:		A null-terminated string containg the 
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
	if(!(err_msg = (char *)malloc((size_t)((strlen(msg) + strlen(command) + strlen(arg) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for err_msg */
	
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

