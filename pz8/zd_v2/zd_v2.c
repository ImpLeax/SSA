#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_CMDS 16

void execute_pipeline(char *cmds[], int num_cmds) {
    int i;
    int fd[2];
    int prev_fd = 0;
    pid_t pid;

    for (i = 0; i < num_cmds; i++) {
        char *cmd_copy = strdup(cmds[i]);
        char *args[MAX_ARGS];
        int arg_count = 0;
        char *token = strtok(cmd_copy, " \t\n");

        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[arg_count] = NULL;

        if (arg_count == 0) {
            free(cmd_copy);
            continue;
        }

        if (i < num_cmds - 1) {
            if (pipe(fd) < 0) {
                perror("pipe failed");
                exit(1);
            }
        }

        pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            if (prev_fd != 0) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else {
            if (prev_fd != 0) {
                close(prev_fd);
            }
            if (i < num_cmds - 1) {
                close(fd[1]);
                prev_fd = fd[0];
            }
            free(cmd_copy);
        }
    }

    for (i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

int main() {
    char input[MAX_INPUT];
    char *cmds[MAX_CMDS];

    while (1) {
        printf("minishell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        int num_cmds = 0;
        char *cmd_token = strtok(input, "|");

        while (cmd_token != NULL && num_cmds < MAX_CMDS) {
            cmds[num_cmds++] = cmd_token;
            cmd_token = strtok(NULL, "|");
        }

        if (num_cmds > 0) {
            execute_pipeline(cmds, num_cmds);
        }
    }

    return 0;
}
