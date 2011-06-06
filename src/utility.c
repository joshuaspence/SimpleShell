/*
    utility.

    Author: Joshua Spence
    SID:    308216350
    
    Contains utility functions used for general string manipulation and other actions.
*/

#include    "../inc/utility.h"

/*
    ===========================================================
    remove_character
    ===========================================================
    Remove a character from a string, shifting all other 
    characters to fill the gap.
    -----------------------------------------------------------
    string:     String to work on.
    stop_at:    Stop processing when one of these characters
                            is reached.
    remove:     The character to removed
    -----------------------------------------------------------
    return value:
                A pointer to the string with the character
                removed.
    -----------------------------------------------------------
*/
char *remove_character(char *string, const char remove) {
    char *s = string;                                           // the current character being checked
    char *t;                                                    // to loop through and shift all remaining characters
    unsigned int count = 0;                                     // the number of characters removed so far
    unsigned int length = strlen(string);                       // length of the string
    
    while ((length--) > 1) {
        if (*s == remove) {
            count++;
            
            /* shift characters downwards */
            t = s;
            while (*(t + 1)) {
                *t = *(t + 1);
                t++;
            }
        }
        s++;
    }
    
    // delete characters from string
    while (count--) {
        *s-- = 0;
    }
    
    return string;
}

/*
    ===========================================================
    quoted_strtok
    ===========================================================
    Works similarly to strtok except can handle quoted 
    arguments.
    -----------------------------------------------------------
    string:     String to tokenize. If this argument is null,
                then the last processed string will be 
                continued.
    delimiters: The characters to be considered delimiters.
    -----------------------------------------------------------
    return value:
                A pointer to the next token.
    -----------------------------------------------------------
*/
char *quoted_strtok(char *string, const char *delimiters, const char *quotes) {
    static char *next;                                          // where to start searching next
    char *start;                                                // start of next token
    const char *d;                                              // current delimiter
    const char *q;                                              // current quotation mark
    boolean is_delimiter = TRUE;                                // indicates that the current character is a delimiter character
    boolean is_quoted = FALSE;                                  // indicates that an opening quotation mark has been found
    
    if (!string && !(string = next)) {
    // reached end of original string
        return NULL;
    }
    
    // skip leading whitespace before next token
    while (*string) {
        d = delimiters;
        is_delimiter = FALSE;
        
        // check if the current character is a delimiter character
        while (*d) {
            if (*string == *d) {
                is_delimiter = TRUE;
                break;
            }
            
            d++; /* move to next delimiter character */
        }
        
        if (!is_delimiter) {
                break; /* non-delimiter character found */
        } else {
            string++; /* move to next character of string */
        }
    }

    // make sure the string has some non-delimiter characters left
    if (is_delimiter) {
        return NULL;
    } else {
        start = string;
    }
            
    // find next whitespace delimiter (unless quoted) or end of string
    while (*string) {
        d = delimiters;
        q = quotes;
        is_delimiter = FALSE;
        
        while (*q) {
            if (*string == *q) {
                if (is_quoted) {
                    is_quoted = FALSE;
                } else {
                    is_quoted = TRUE;
                }
                break;
            }
            q++; /* move to next quotation character */
        }
        
        // check if the current character is a delimiter character
        if (!is_quoted) {
            while (*d) {
                if (*string == *d) {
                    is_delimiter = TRUE;
                    break;
                }
                d++; /* move to next delimiter character */
            }
        }
                
        if (!is_quoted && is_delimiter) {
            break;
        } else {
            string++; /* move to next character of string */
        }
    }

    // reached end of original string?
    if (!(*string)) {
        next = NULL;
    } else {
        *string = 0;
        next = string + 1;
    }
    
    // remove quotation marks from string
    return start;
}

/*
    ===========================================================
    get_path
    ===========================================================
    Get path to current executable. This function will allocate
    or reallocate memory if required and it is the 
    responsibility of the caller to free this memory.
    -----------------------------------------------------------
    path:       Pointer to memory available to store the path.
                Can be null.
    -----------------------------------------------------------
    return value:
                A pointer to the memory containing the path.
    -----------------------------------------------------------
*/
char *get_path(char *path) {
    const char proc[] = "/proc/%d/exe";                         // string of function to find path to executable
    char *proc_string;                                          // string of function to find path to executable (with pid inserted)
    ssize_t length = 0;                                         // current length of path
    ssize_t size = 0;                                           // current size of path
    const pid_t pid = getpid();                                 // result of getpid()
    
    // initialise
    if (!(proc_string = (char *)malloc((size_t)((strlen(proc) + digits(pid) + 1 /* for null character */) * sizeof(char))))) sys_err("malloc"); /* attempt to allocate memory for proc_string */
    sprintf(proc_string, proc, pid);
    
    do {
        size += ALLOCATION_BLOCK;
        if (!(path = (char *)realloc(path, size))) sys_err("realloc"); /* realloc(NULL,n) is the same as malloc(n) */
        length = readlink(proc_string, path, size - 1);
    } while ((length < 0) || (length >= (size - 1)));
    path[length] = '\0'; /* add null character */
    
    // free memory
    free(proc_string);
    
    return path;
}

/*
    ===========================================================
    digits
    ===========================================================
    Counts the number of digits in an integer.
    -----------------------------------------------------------
    i:          The integer to be counted.
    -----------------------------------------------------------
    return value:
                The number of digits contained in the integer.
    -----------------------------------------------------------
*/
unsigned int digits(const int i) {
    int i_copy = i;                                             // copy of i (this allows digits to be used with a non-const argument i
    unsigned int count = 1;                                     // number of digits counted so far

    if (i_copy < 0) {
        i_copy = -i_copy;
    }
    
    while ((i_copy /= 10) > 0) {
        count++;
    }
    
    return count;
}

/*
    ===========================================================
    array_size
    ===========================================================
    Counts the number of elements in an array.
    -----------------------------------------------------------
    array:      A pointer to an element of the array to be 
                counted. The array MUST be terminated by a 
                null element, to prevent an illegal read.
    -----------------------------------------------------------
    return value:
                The number of elements in the array begining
                at array and ending at the null element of the
                array.
    -----------------------------------------------------------
*/
unsigned int array_size(const char **array) {
    unsigned int count = 0;                                     // used to count size of array
    const char **ptr = array;                                   // used to traverse the array
    
    while (*ptr++) {
        count++;
    }
    
    return count;
}

/*
    ===========================================================
    array_movetoend
    ===========================================================
    Shifts the contents of an array of strings one place to the 
    left, starting with the element passed as a parameter. The
    element passed as a parameter is moved to the end of the 
    array.
    
    The caller could then delete this element and free the 
    memory associated with this element, if required.
    -----------------------------------------------------------
    element:    the element to be overwritten. The pointer 
                does not need to point to be start of the 
                array. The array, however, MUST be terminated
                by a null element to prevent an illegal read.
    -----------------------------------------------------------
*/
char **array_movetoend(char **element) {
    char **ptr = element;                                       // used to traverse the array
    
    while (*(ptr + 1)) {
        *(ptr) = *(ptr + 1); /* left shift array element */
        ptr++;
    }
    *ptr = *element;
    
    return ptr;
}

/*
    ===========================================================
    string_char_count
    ===========================================================
    Counts the number of occurences of a character within a 
    string.
    -----------------------------------------------------------
    string:     The string to be searched. MUST be null 
                terminated.
    character:  The character to count occurences of.
    -----------------------------------------------------------
    return value:
                The number of occurences of the character
                within the string.
    -----------------------------------------------------------             
*/
unsigned int string_char_count(const char *string, const char character) {
    unsigned int count = 0;                                     // used to count number of occurences of character
    const char *s = string;                                     // used to traverse the string
    
    while (*s) {
        if (*s++ == character) {
            count++;
        }
    }
    
    return count;
}

/*
    ===========================================================
    string_chars_count
    ===========================================================
    Counts the number of occurences of any single character 
    from a string within a different string.
    -----------------------------------------------------------
    string:     The string to be searched. MUST be null
                terminated.
    chars:      A string container all characters to count 
                occurences of. MUST be null terminated.
    -----------------------------------------------------------
    return value:
                The number of occurences of the characters
                within the string.
    -----------------------------------------------------------     
*/
unsigned int string_chars_count(const char *string, const char *chars) {
    unsigned int count = 0;                                     // used to count number of occurences of character
    const char *s = string;                                    // used to traverse the string
    const char *character;                                      // used to traverse the characters to count
    
    while (*s) {
        character = chars;
        while (*character) {
            if (*s == *character++) {
                count++;
            }
        }
        s++;
    }
    
    return count;
}

/*
    ===========================================================
    sys_err
    ===========================================================
    Print an error message to stderr and abort. Uses the error
    number of the last experienced error to generate an error
    message.
    -----------------------------------------------------------
    prog:       Null-terminated string containing the name of
                the program which caused the error.
    -----------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void sys_err(const char *prog) {
   fprintf(stderr, "Encountered an error!\n%s: %s\n", prog, strerror(errno)); /* print error message to stderr */
   abort(); /* abort program */
}

/*
    ===========================================================
    err
    ===========================================================
    Print an error message to stderr.
    -----------------------------------------------------------
    msg:        Null-terminated string containing the error 
                message to be printed.
    -----------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void err(const char *msg) {
   fprintf(stderr, "%s\n", msg); /* print error message to stderr */
}

#ifdef DEBUG
/*
    ===========================================================
    debug_message
    ===========================================================
    Print a debug message. The first line of the debug message
    will be prefixed with DEBUG_MESSAGE_PREFIX and each 
    successive line will be prefixed with whitespace.
    -----------------------------------------------------------
    msg:        Null-terminated string containing the message 
                to be printed.
    -----------------------------------------------------------
    no return value
    ------------------------------------------------------------
*/
void debug_message(const char *msg) {
    char *msg_copy;                                             // copy of the message (this allows debug_message to be used with a non-cost argument msg)
    char **lines;                                               // each line of output
    char **line;                                                // working pointer through lines
    char *output;                                               // current output
    char *blanks;                                               // blank spaces for debug prefixes
    unsigned int num_lines;                                     // number of lines of output
    
    // initialise
    num_lines = string_char_count(msg, '\n'); /* count the number of newline characters in msg in order to allocate memory */
    if (msg[strlen(msg) - 1] != '\n') {
        num_lines++;
    }
    if (!(lines = (char **)malloc((size_t)((num_lines + 1 /* for null element */) * sizeof(char *))))) sys_err("malloc"); /* attempt to allocate memory for lines */
    if (!(blanks = (char *)malloc((size_t)(strlen(DEBUG_MESSAGE_PREFIX) + 1 /* for null character */) * sizeof(char)))) sys_err("malloc"); /* attempt to allocate memory for blanks */
    if (!(msg_copy = (char *)malloc((size_t)(strlen(msg) + 1 /* for null character */) * sizeof(char)))) sys_err("malloc"); /* attempt to allocate memory for msg_copy */
    
    // copy the message (strtok cannot handle a const string)
    strcpy(msg_copy, msg);
    
    // replace debug prefix with spaces (one character at a time)
    *blanks = 0; /* initialise blanks */
    for (unsigned int i = 0; i < strlen(DEBUG_MESSAGE_PREFIX); ++i) {
        strcat(blanks, " ");
    }
    
    // tokenise each line of the message (using newline feed as delimiter)
    line = lines;
    *line++ = strtok(msg_copy, "\n");
    while ((*line++ = strtok(NULL, "\n")));
    
    // output first line, appending the debug prefix to the start of the line
    line = lines;
    if (*line) {
        // memory allocation
        if (!(output = (char *)malloc((size_t)(strlen(DEBUG_MESSAGE_PREFIX) + strlen(*line) + 1 /* for new-line character */ + 1 /* for null character */) * sizeof(char)))) sys_err("malloc"); /* attempt to allocate memory for output */
        
        // output this line of debug information
        sprintf(output, "%s%s\n", DEBUG_MESSAGE_PREFIX, *line++);
        printf(output);
        
        // output other lines, appending white space (equal to the length of the debug prefix) to the start of the line
        while (*line) {
            // memory allocation
            if (!(output = (char *)realloc(output, (size_t)((strlen(DEBUG_MESSAGE_PREFIX) + strlen(*line) + 1 /* for new-line character */ + 1 /* for null character */) * sizeof(char))))) sys_err("realloc"); /* attempt to reallocate memory for output */
            
            // output this line of debug information
            sprintf(output, "%s%s\n", blanks, *line++);
            printf(output);
        }
    }
    
    // clean up
    free(lines);
    free(blanks);
    free(msg_copy);
    if (output) {
        free(output);
    }
}
#endif
