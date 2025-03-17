#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

/**** 
 **** FOR REMOTE SHELL USE YOUR SOLUTION FROM SHELL PART 3 HERE
 **** THE MAIN FUNCTION CALLS THIS ONE AS ITS ENTRY POINT TO
 **** EXECUTE THE SHELL LOCALLY
 ****
 */

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

/*
* All functions are taken from assignment 5, and altered when needed to adhere to our new assignment
*/

//CD Function to change directory
int change_directory(cmd_buff_t *cmd) {
    if (cmd->argc == 1) {
        return OK;
    } else if (cmd->argc == 2) {
        //If correct num of args passed, cd to specified path
        if (chdir(cmd->argv[1]) != 0) {
            perror("cd");
            return ERR_EXEC_CMD;
        }
    }
    return OK;
}

//Function to match command with built in commands located in dshlib.h
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_STOP_SVR;
    if (strcmp(input, "rc") == 0)
        return BI_CMD_RC;
    return BI_NOT_BI;
}

// Function to execute built-in commands
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);
    
    switch (cmd_type) {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
        case BI_CMD_DRAGON:
            return BI_EXECUTED;
        case BI_CMD_CD:
            return change_directory(cmd) == OK ? BI_EXECUTED : BI_NOT_BI;
        case BI_CMD_STOP_SVR:
            return BI_CMD_STOP_SVR;
        case BI_CMD_RC:
            return BI_EXECUTED;
        default:
            return BI_NOT_BI;
    }
}

//Function to parse command line input
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    //Trim leading whitespace
    while (isspace((unsigned char)*cmd_line)) 
        cmd_line++; 
    //Trim trailing whitespace
    char *end = cmd_line + strlen(cmd_line) - 1;
    while (end > cmd_line && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    //Duplicate the trimmed command line into our buffer
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) 
        return ERR_MEMORY;
    cmd_buff->argc = 0;

    //Take command line input and parse into tokens
    char *p = cmd_buff->_cmd_buffer;
    char *token_start = NULL;
    while (*p) {
        //Skip any whitespace between tokens
        while (*p && isspace((unsigned char)*p))
            p++;
        if (!*p) break;

        //Check if we are at a pipe operator
        if (*p == '|') {
            //Insert the pipe token.
            cmd_buff->argv[cmd_buff->argc++] = (char *)"|";
            p++; 
            continue;
        }
        if (*p == '"') {
            //If the token starts with "", skip the opening quote
            p++;
            token_start = p;
            //Read until we hit the closing quote
            while (*p && *p != '"') {
                p++;
            }
            //If a closing quote is found, terminate the token and move past it
            if (*p == '"') {
                *p = '\0';
                p++;
            }
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            continue;
        }

        //For unquoted tokens, record the start of the token and read until we hit a space or pipe
        token_start = p;
        while (*p && !isspace((unsigned char)*p) && *p != '|') {
            p++;
        }
        //If a pipe is found, terminate the token and move past it
        if (*p == '|') {
            *p = '\0';
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            continue;
        } else if (*p) {  //Terminate the token if not end of string
            *p = '\0';
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            p++;
        } else {  //End of string
            cmd_buff->argv[cmd_buff->argc++] = token_start;
        }
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

// Function to build command list from a command line
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    clist->num = 0;
    
    char *cmd_start = cmd_line;
    char *pipe_pos;
    
    // Find each pipe and split the command line
    while ((pipe_pos = strstr(cmd_start, "|")) != NULL) {
        // Temporarily null-terminate at the pipe
        *pipe_pos = '\0';
        
        // Build command buffer for this segment
        if (build_cmd_buff(cmd_start, &clist->commands[clist->num]) != OK) {
            return ERR_MEMORY;
        }
        
        clist->num++;
        if (clist->num >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }
        
        // Move to the next segment after the pipe
        cmd_start = pipe_pos + 1;
        
        // Restore the pipe character
        *pipe_pos = '|';
    }
    
    // Process the last or only command
    if (build_cmd_buff(cmd_start, &clist->commands[clist->num]) != OK) {
        return ERR_MEMORY;
    }
    clist->num++;
    
    return OK;
}

// Free resources in command list
int free_cmd_list(command_list_t *cmd_lst) {
    for (int i = 0; i < cmd_lst->num; i++) {
        if (cmd_lst->commands[i]._cmd_buffer) {
            free(cmd_lst->commands[i]._cmd_buffer);
            cmd_lst->commands[i]._cmd_buffer = NULL;
        }
    }
    return OK;
}

//Function for executing the pipeline with a single command buffer
int execute_pipeline(cmd_buff_t *cmd) {
    //Check if any pipe symbols are present in the command
    int pipe_positions[CMD_MAX] = {0};
    int pipe_count = 0;
    int i;

    //Find all pipe symbols and count them
    for (i = 0; i < cmd->argc; i++) {
        if (strcmp(cmd->argv[i], "|") == 0) {
            pipe_positions[pipe_count++] = i;
            if (pipe_count >= CMD_MAX - 1) {
                fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX - 1);
                return ERR_TOO_MANY_COMMANDS;
            }
        }
    }

    //Use BUILT_IN_CMDs to match command with built in commands
    if (pipe_count == 0) {
        Built_In_Cmds cmd_type = match_command(cmd->argv[0]);
        //If the command is CD run change_directory()
        if (cmd_type == BI_CMD_CD) {
            return change_directory(cmd);
        } else if (cmd_type == BI_CMD_EXIT) {
            printf("exiting...\n");
            return EXIT_SC;  // Return exit code instead of calling exit()
        } else if (cmd_type == BI_CMD_STOP_SVR) {
            printf("requesting server to stop...\n");
            return OK_EXIT;  // Signal server stop
        } else if (cmd_type == BI_CMD_DRAGON) {
            return OK;
        } else if (cmd_type == BI_NOT_BI){
            //Fork to duplicate parent process into a new child process
            pid_t pid = fork();
            if (pid == 0) {
                //Execvp() to execute the command
                if (execvp(cmd->argv[0], cmd->argv) == -1) {
                    perror("execvp");
                    exit(ERR_EXEC_CMD);
                }
            } else if (pid < 0) {
                //Handle fork failure
                perror("fork");
                return ERR_EXEC_CMD;
            } else {
                //If fork is successful, wait for child process to finish
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                }
                return ERR_EXEC_CMD;
            }
        }
    } else {
        //Setting up for piped commands
        int num_commands = pipe_count + 1;
        int pipes[CMD_MAX][2];
        pid_t pids[CMD_MAX];
        
        //Create needed pipes
        for (i = 0; i < pipe_count; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                return ERR_EXEC_CMD;
            }
        }
        
        //Execute the commands by forking. This requires multiple commands
        int cmd_start = 0;
        
        for (i = 0; i < num_commands; i++) {
            int cmd_end = (i < pipe_count) ? pipe_positions[i] : cmd->argc;
            
            pid_t pid = fork();
            
            if (pid == 0) {
                //The pipe setup handles both input and output redirection
                
                //First process reads from the terminal/stdin
                if (i > 0) {
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                }
                
                //Processes except the last write to the pipe
                if (i < num_commands - 1) {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }
                
                //Close all the pipe ends
                for (int j = 0; j < pipe_count; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                
                //Prepare the arguments
                char *args[CMD_MAX];
                int arg_index = 0;
                
                for (int j = cmd_start; j < cmd_end; j++) {
                    args[arg_index++] = cmd->argv[j];
                }
                args[arg_index] = NULL;
                
                //Exec the command
                if (execvp(args[0], args) == -1) {
                    perror("execvp");
                    exit(ERR_EXEC_CMD);
                }
            } else if (pid < 0) {
                perror("fork");
                return ERR_EXEC_CMD;
            }
            
            pids[i] = pid;
            cmd_start = cmd_end + 1;  //Skip the pipe token
        }
        
        //Close all pipes in parent process
        for (int j = 0; j < pipe_count; j++) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
        
        //Wait for child processes to finish if fork is successful
        int status;
        int last_status = 0;
        
        for (i = 0; i < num_commands; i++) {
            waitpid(pids[i], &status, 0);
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            }
        }
        
        return last_status;
    }
    
    return OK;
}

// Function to execute a pipeline using command_list_t structure
int execute_pipeline_list(command_list_t *clist) {
    // Check if there's only one command and if it's a built-in command
    if (clist->num == 1) {
        Built_In_Cmds cmd_type = exec_built_in_cmd(&clist->commands[0]);
        if (cmd_type == BI_EXECUTED) {
            return OK;
        } else if (cmd_type == BI_CMD_EXIT) {
            printf("exiting...\n");
            return EXIT_SC;
        } else if (cmd_type == BI_CMD_STOP_SVR) {
            printf("requesting server to stop...\n");
            return OK_EXIT;
        }
    }
    
    // Setup pipes
    int pipes[CMD_MAX][2];
    pid_t pids[CMD_MAX];
    int pids_st[CMD_MAX];
    int exit_code = OK;
    
    // Create pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }
    
    // Fork and execute each command
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("fork");
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            // Child process
            
            // Setup stdin (from previous pipe)
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Setup stdout (to next pipe)
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipes in child
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }
    
    // Close all pipes in parent
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }
    
    // Get exit code from last command
    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);
    
    // If any command returned EXIT_SC, return that code
    for (int i = 0; i < clist->num; i++) {
        if (WEXITSTATUS(pids_st[i]) == EXIT_SC) {
            exit_code = EXIT_SC;
        }
    }
    
    return exit_code;
}

//Function for continously prompting user for input, parsing the input and executing the command
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd;
    int rc = 0;

    //Start loop prompting the user for input
    while (1) {
        printf("%s", SH_PROMPT);
        fflush(stdout);

        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Handle empty input
        if (strlen(cmd_line) == 0) {
            continue;
        }

        rc = build_cmd_buff(cmd_line, &cmd);
        if (rc != OK) {
            fprintf(stderr, "Error parsing command\n");
            continue;
        }

        if (cmd.argc == 0) {
            printf(CMD_WARN_NO_CMD);
            free(cmd._cmd_buffer);
            continue;
        }

        rc = execute_pipeline(&cmd);
        if (rc == EXIT_SC) {
            free(cmd._cmd_buffer);
            return OK; // Exit the local command loop
        }
        
        free(cmd._cmd_buffer);
    }

    return rc;
}