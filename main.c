#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_HISTORY 10
#define MAX_LINE 1024

char history[MAX_HISTORY][MAX_LINE];
int history_count = 0;

void add_history(const char *cmd) {
    if (history_count < MAX_HISTORY) {
        strncpy(history[history_count], cmd, MAX_LINE - 1);
        history[history_count][MAX_LINE - 1] = '\0';
        history_count++;
    } else {
        for (int i = 1; i < MAX_HISTORY; i++)
            strcpy(history[i - 1], history[i]);
        strncpy(history[MAX_HISTORY - 1], cmd, MAX_LINE - 1);
        history[MAX_HISTORY - 1][MAX_LINE - 1] = '\0';
    }
}

void show_history() {
    for (int i = 0; i < history_count; i++)
        printf("%d %s\n", i + 1, history[i]);
}

int handle_builtin(char **args) {
    if (!args[0]) return 0;

    if (strcmp(args[0], "exit") == 0)
        exit(0);

    if (strcmp(args[0], "cd") == 0) {
        if (!args[1] || chdir(args[1]) != 0)
            perror("cd");
        return 1;
    }

    if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("%s\n", cwd);
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("Built-ins: exit cd pwd help history !! !n\n");
        return 1;
    }

    if (strcmp(args[0], "history") == 0) {
        show_history();
        return 1;
    }

    return 0;
}

int parse_command(char *line, char **args, int *background) {
    int i = 0;
    *background = 0;

    char *token = strtok(line, " \t\n");
    while (token) {
        if (strcmp(token, "&") == 0)
            *background = 1;
        else
            args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return i;
}

int split_pipeline(char *line, char **commands) {
    int i = 0;
    char *cmd = strtok(line, "|");
    while (cmd) {
        while (*cmd == ' ') cmd++;
        commands[i++] = cmd;
        cmd = strtok(NULL, "|");
    }
    commands[i] = NULL;
    return i;
}

void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    int background = 0;

    parse_command(cmd, args, &background);
    if (!args[0]) return;

    if (handle_builtin(args)) return;

    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("exec");
        exit(1);
    } else if (pid > 0) {
        if (!background)
            waitpid(pid, NULL, 0);
        else
            printf("[Background PID %d]\n", pid);
    } else {
        perror("fork");
    }
}

void execute_pipeline(char **commands, int n, int background) {
    int in_fd = 0, fd[2];

    for (int i = 0; i < n; i++) {
        if (i < n - 1) {
            if (pipe(fd) < 0) {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < n - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }

            char *args[MAX_ARGS];
            int bg;
            parse_command(commands[i], args, &bg);
            execvp(args[0], args);
            perror("exec");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
            return;
        }

        if (in_fd != 0)
            close(in_fd);

        if (i < n - 1) {
            close(fd[1]);
            in_fd = fd[0];
        }
    }

    if (!background)
        for (int i = 0; i < n; i++)
            wait(NULL);
    else
        printf("[Pipeline running in background]\n");
}

int main() {
    char line[MAX_LINE];

    while (1) {
        printf("uinxsh> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        if (strcmp(line, "!!") == 0) {
            if (history_count == 0) {
                printf("No commands in history\n");
                continue;
            }
            strcpy(line, history[history_count - 1]);
            printf("%s\n", line);
        } else if (line[0] == '!' && line[1]) {
            int n = atoi(&line[1]);
            if (n <= 0 || n > history_count) {
                printf("No such command in history\n");
                continue;
            }
            strcpy(line, history[n - 1]);
            printf("%s\n", line);
        }

        add_history(line);

        int background = strchr(line, '&') != NULL;

        char *commands[MAX_ARGS];
        int n = split_pipeline(line, commands);

        if (n > 1) {
            for (int i = 0; i < n; i++) {
                char *tmp[MAX_ARGS];
                int bg;
                parse_command(commands[i], tmp, &bg);
                if (handle_builtin(tmp)) {
                    fprintf(stderr, "Built-in commands cannot be used in pipelines\n");
                    goto cleanup;
                }
            }
            execute_pipeline(commands, n, background);
        } else {
            execute_command(commands[0]);
        }

cleanup:
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    return 0;
}

