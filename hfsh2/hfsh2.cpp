//*********************************************************
//
// Zyler Niece
// Operating Systems
// Writing Your Own Shell: hfsh2
// February 20, 2023
// Instuctor: Dr. Michael Scherger
//*********************************************************


//*********************************************************
//
// Includes and Defines
//
//*********************************************************
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<ctype.h>

#define STR_EXIT "exit"
#define MAX_LINE 256
#define ERROR_MSG "An error has occurred\n"


//*********************************************************
//
// Extern Declarations
//
//*********************************************************
// Buffer state. This is used to parse string in memory...
// Leave this alone.
extern "C"{
    extern char **gettoks();
    typedef struct yy_buffer_state * YY_BUFFER_STATE;
    extern YY_BUFFER_STATE yy_scan_string(const char * str);
    extern YY_BUFFER_STATE yy_scan_buffer(char *, size_t);
    extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
    extern void yy_switch_to_buffer(YY_BUFFER_STATE buffer);
}
char path[100]; // Current path

//*********************************************************
//
// Namespace
//
// If you are new to C++, uncommment the `using namespace std` linestr.
// It will make your life a little easier.  However, all true
// C++ developers will leave this commented.  ;^)
//
//*********************************************************
// using namespace std;


//*********************************************************
//
// Function Prototypes
//
//*********************************************************

//*********************************************************
// Execute Command Function
void execute_command(char *toks[], int wait_flag);
//
// This function executes a command given as an array of strings
// (toks) and a flag (wait_flag) that indicates whether the 
// function should wait for the command to finish executing. 
// If the command is a built-in command (cd or path), it executes
// the appropriate function. Otherwise, it forks a new process for
// each command in the array and executes the command in each process.
// The function also checks for output redirection, and redirects
// the output to a file if necessary. If wait_flag is set to true,
// the function waits for each child process to finish executing 
// before returning.
//
// Return Value
// --------------
// N/A - Void method
//
// Function Parameters
// -------------------
// toks         string[]    value
// wait_flag    integer     flag 
//
// Local Variables
// ----------------
// num_commands    int      stores num commands   
// command_idx     int      keeps track of current command being executed  
// pids            int[]    stores the process ID's
// command_args    string[] stores the arguments for the current command being executed    
// pid             int      process ID from fork system call
// command         string   stores the full path of the command to be executed
// out             int      used to keep track of whether output redirection is required
// fd              int      file descriptor for output file when output redirection is required
// status          int      used to store the exit status of child process
//
//*********************************************************

//*********************************************************
// Change Directories
void change_dir(char *dir);
//
// The change_dir function changes the current working directory
// to the directory specified by the dir parameter. It does this
// using the chdir system call, which takes a string representing
// the directory path and changes the current working directory to that path.
//
// Return Value
// --------------
// N/A - either changes directories or writes an error message
//
// Function Parameters
// -------------------
// dir         *String     represents the directory path to change to
//
// Local Variables
// ----------------
// N/A
//
//*********************************************************

//*********************************************************
// Batch Mode
void batch_mode(char *filename);
//
// This function reads commands from a file named filename and executes
// them in batch mode. It checks for special characters and sets up an
// input buffer to parse the commands. It then executes each command and
// waits for it to finish before executing the next one, unless the & operator
// is used. If the file cannot be opened, an error message is displayed, and
// if the & operator is not followed by a command argument, an error message
// is also displayed. The function continues until the end of the file or
// until the exit command is executed.
//
// Return Value
// --------------
// N/A - Void method
//
// Function Parameters
// -------------------
// *filename       pointer   pointer to a string specifies name of file
//
// Local Variables
// ----------------
// *fp            FILE              A pointer to the 'File' structure  
// linestr        char[]            store a line of input from input file
// **toks         pointer           stores the tokens generated by gettoks()
// wait_flag      int flag flag     indicates whether the function should wait for command to finish executing    
// buffer         YY_BUFFER_STATE   structure that represents the input buffer used by lexer to parse command string
// 
//*********************************************************

//*********************************************************
// Is Valid Command
int is_valid_command(char *command);
//
// The function is_valid_command checks if a given command exists in
// the current path. It takes a string argument command, which represents
// the command to be checked. The function first constructs a string called
// path_command by concatenating the current path and the command name.
// It then uses the access function to check if the path_command string 
// represents a valid path to an existing file. The F_OK flag is used to
// check if the file exists. If the file exists, the function returns 1
// indicating that the command is valid. If the file does not exist,
// the function returns 0 indicating that the command is not valid.
//
// Return Value
// --------------
// Int Value of either 1(valid) or 0(invalid)
//
// Function Parameters
// -------------------
// *command         pointer     points to string representing name of command to be checked
//
// Local Variables
// ----------------
// path_command     char[]      used to store the full path of the command by concateating 'path' and 'command' strings
//
//*********************************************************

//*********************************************************
// Set Path
void set_path(char *new_path);
//
// The set_path function sets the value of the global variable path to a new path string
// passed as an argument. If the new_path argument is NULL, the function sets the path to
// an empty string by calling strcpy with an empty string as the second argument.
// If the new_path argument is not NULL, the function sets the path to the new_path string
// by calling strcpy with the new_path as the second argument.
//
// Return Value
// --------------
// N/A - modifies the global variable 'path'
//
// Function Parameters
// -------------------
// *new_path        pointer     points to string representing the new path to set
//
// Local Variables
// ----------------
// N/A
//
//*********************************************************

//*********************************************************
//
// Main Function
//
//*********************************************************
int main(int argc, char *argv[])
{
    /* local variables */
    int ii;
    char **toks;
    int retval;
    char linestr[MAX_LINE];
    YY_BUFFER_STATE buffer;
    int error_printed = 0;

    /* initialize local variables */
    ii = 0;
    toks = NULL;
    retval = 0;

    strcpy(path, "/bin"); // Set the default path to /bin

if (argc > 2) {
        write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
        exit(1);
    }
if (argc == 2) {
        batch_mode(argv[1]);
    } else {

        strcpy(path, "/bin"); // Set the default path to /bin
    
        while(1) {
            printf("hfsh2> ");
            fgets(linestr, MAX_LINE, stdin);

            // Check if the input line is empty
            if (linestr[0] == '\n') {
                continue; // Skip parsing and start the loop over again
            }

            if (strcmp(linestr, "&\n") == 0) {
            continue; // Skip parsing and execution
            } 

            // make sure linestr has a '\n' at the end of it
            if(!strstr(linestr, "\n"))
            strcat(linestr, "\n");

             int wait_flag = 1; // Default to waiting for command to finish

            // Check for & operator
            char *ampersand = strchr(linestr, '&');
            if (ampersand != NULL) {
                wait_flag = 0;
                *ampersand = '\0'; // Replace & with null terminator
                // Check if there is an argument after the &
                if (!isspace(*(ampersand + 1))) {
                    fprintf(stderr, "Error: '&' must be followed by a command argument\n");
                    continue; // Skip parsing and execution
                }
            }

            /* get arguments */
            buffer = yy_scan_string(linestr);
            yy_switch_to_buffer(buffer);
            toks = gettoks();
            yy_delete_buffer(buffer);

            if (strcmp(toks[0], STR_EXIT) == 0) {
                if (toks[1] != NULL) {
                    if (!error_printed){
                        write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                        error_printed = 1;
                }
            }   else {
                    break;
            } 
            } if (!strcmp(toks[0], STR_EXIT) == 0) {
                execute_command(toks, wait_flag);
            }
        }
    }
     /* return to calling environment */
   return( retval );
}

int is_valid_command(char *command) {
    // Check if the command is in the current path
    char path_command[MAX_LINE];
    strcpy(path_command, path);
    strcat(path_command, "/");
    strcat(path_command, command);

    if (access(path_command, F_OK) != 0) {
        return 0; // Command not found
    }

    return 1; // Command is valid
}

void set_path(char *new_path) {
    if (new_path == NULL) {
        strcpy(path, "");
        //printf("Shell path set to empty. Only built-in commands will work.\n");
    } else {
        strcpy(path, new_path);
        //printf("Shell path set to %s\n", path);
    }
}

void execute_command(char *toks[], int wait_flag) {
    if (strcmp(toks[0], "cd") == 0) {
        change_dir(toks[1]);
    } else if (strcmp(toks[0], "path") == 0) {
        set_path(toks[1]);
    } else {
        int num_commands = 1; // number of commands to execute
        int command_idx = 0; // index of the current command
        int pids[MAX_LINE]; // process IDs for each command

        // Count the number of commands separated by "&"
        for (int i = 1; toks[i] != NULL; i++) {
            if (strcmp(toks[i], "&") == 0) {
                num_commands++;
            }
        }

        while (toks[command_idx] != NULL) {
            // Build the arguments for the current command
            char *command_args[MAX_LINE];
            int i;
            for (i = command_idx; toks[i] != NULL && strcmp(toks[i], "&") != 0; i++) {
                command_args[i - command_idx] = toks[i];
            }
            command_args[i - command_idx] = NULL;

            // Check if the command is valid
            if (!is_valid_command(command_args[0])) {
                write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                break;
            }

            pid_t pid = fork();

            if (pid == 0) {
                // Child process
                char command[MAX_LINE];
                strcpy(command, path);
                strcat(command, "/");
                strcat(command, command_args[0]);

                // Check if there is an output redirection
                int out = -1;
                for (int i = 0; command_args[i] != NULL; i++) {
                    if (strcmp(command_args[i], ">") == 0) {
                        if (out != -1) {
                            // Two output redirection symbols
                            write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                            exit(1);
                        }
                        out = i + 1;

                        // Check if there is another argument after the output file
                        if (command_args[out] == NULL || command_args[out+1] != NULL) {
                            // Two output files after redirection symbol
                            write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                            exit(1);
                        }
                    }
                }

                if (out != -1) {
                    // Redirect output to file
                    int fd = open(command_args[out], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

                    if (fd == -1) {
                        write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                        exit(1);
                    }

                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                execv(command, command_args);
                write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
            } else if (pid > 0) {
                // Parent process
                pids[command_idx] = pid;

                if (!wait_flag) {
                    // Don't wait for the child process to finish
                }
            } else {
                // Fork failed
                write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
            }

            command_idx = i + 1;
        }

        // Wait for the child processes to finish
        if (wait_flag) {
            for (int i = 0; i < num_commands; i++) {
                int status;
                waitpid(pids[i], &status, 0);

                if (WIFEXITED(status)) {
                }
            }
        }
    }
}

void change_dir(char *dir) {
    if (chdir(dir) != 0) {
        write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
    }
}


void batch_mode(char *filename) {
    FILE *fp;
    char linestr[MAX_LINE];
    char **toks;
    int wait_flag;
    YY_BUFFER_STATE buffer;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
        exit(1);
    }

    while (fgets(linestr, MAX_LINE, fp) != NULL) {

        // Check if the input line is empty
        if (linestr[0] == '\n') {
            continue; // Skip parsing and start the loop over again
        }

        if (strcmp(linestr, "&\n") == 0) {
            continue; // Skip parsing and execution
        } 
        
        /* make sure linestr has a '\n' at the end of it */
        if (!strstr(linestr, "\n")) {
            strcat(linestr, "\n");
        }

        /* Check for & operator */
        wait_flag = 1;
        char *ampersand = strchr(linestr, '&');
        if (ampersand != NULL) {
            wait_flag = 0;
            *ampersand = '\0'; /* Replace & with null terminator */
            // Check if there is an argument after the &
            if (!isspace(*(ampersand + 1))) {
                fprintf(stderr, "Error: '&' must be followed by a command argument\n");
                 continue; // Skip parsing and execution
            }
        }

        /* Set up input buffer */
        buffer = yy_scan_string(linestr);
        yy_switch_to_buffer(buffer);

        /* get arguments */
        toks = gettoks();

        /* Clean up */
        yy_delete_buffer(buffer);

        /* Execute command */
        if (strcmp(toks[0], STR_EXIT) == 0) {
            if (toks[1] != NULL) {
                write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
            } else {
                break;
             } 
        }
        if (!strcmp(toks[0], STR_EXIT) == 0) {
                execute_command(toks, wait_flag);
            }
    }

    fclose(fp);
}