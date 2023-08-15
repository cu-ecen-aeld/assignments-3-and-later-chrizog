#include "systemcalls.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int result = system(cmd);
    if (0 == result) {
        return true;
    }
    else {
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    bool success = false;
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    pid_t pid_child = fork();
    if (0 == pid_child) {
        // The return value 0 means that it's called in the child process
        int result = execv(command[0], &command[0]);
        if (result < 0) {
            exit(-1);
        }
    }
    else {
        // Parent process
        int status;
        int ret_wait = waitpid(pid_child, &status, 0);
        if (ret_wait == -1) {
            printf("waitpid failed.\n");
        }
        else {
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0) {
                    success = true;
                }
            }
        }
    }

    va_end(args);

    return success;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    bool success = false;
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT);
    int kidpid;
    int status;
    int result_wait;
    switch (kidpid = fork()) {
        case -1: 
            abort();
        case 0:
            if (dup2(fd, 1) < 0) {
                printf("Failed to copy fd!");
                exit(-1);
            }
            int ret_exec = execv(command[0], &command[0]);
            
            if (ret_exec < 0) {
                printf("execv returned an error");
                exit(-1);
            }
        default:
            // Parent process
            result_wait = waitpid(kidpid, &status, 0);
            if (result_wait == -1) {
                success = false;
            }
            else {
                if (WIFEXITED(status)) {
                    int exit_status = WEXITSTATUS(status);
                    if (exit_status == 0) {
                        success = true;
                    }
                }
            }
            close(fd);
    }

    va_end(args);

    return success;
}
