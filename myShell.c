#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LINE 80
#define MAX_ARGS 10
#define MAX_PIPE 10

// color define
#define BLUE "\e[1;34m"
#define YELLOW "\e[1;33m"
#define WHITE "\e[0;37m"
#define RED "\e[1;31m"
#define GREEN "\e[1;32m"

const char *builtins[] = {
    "cd",
    "exit",
    NULL
};

void shell_interact();

void shell_script(char *filename);

void display_hello();

char *update_prefix();

int main(int argc, char *argv[]) {
    display_hello();
    if (argc == 1) {
        // 交互模式
        shell_interact();
    } else if (argc == 2) {
        // 脚本模式
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("fopen");
            exit(1);
        }
        // 重定向标准输入到脚本文件
        dup2(fileno(file), STDIN_FILENO);
        fclose(file);
    } else {
        fprintf(stderr, "Usage: %s [script_file]\n", argv[0]);
        exit(1);
    }

    return 0;
}

// builtin command
void builtin_cd(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (chdir(args[1]) != 0) {
        perror("cd");
    }
}

void builtin_exit() {
    printf("exit lhy's shell, see you again.\n");
    exit(0);
}

int parse(char *line, char *args[]) {
    int cnt = 0;
    char *token = strtok(line, " ");
    int i = 0;
    while (token != NULL && i < MAX_ARGS - 1) {
        cnt++;
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
    return cnt;
}

void execute_out_command(char *args[]) {
    printf("%s\n", args[1]);
    execvp(args[0], args);
    perror("execvp");
    printf("last sub process end\n");
    exit(1);
}

void execute_builtin_command(char *args[]) {
    if (strcmp(args[0], "cd") == 0) {
        builtin_cd(args);
    } else if (strcmp(args[0], "exit") == 0) {
        builtin_exit();
    } else {
        fprintf(stderr, "%s: command not found\n", args[0]);
    }
}

int isBuiltin(const char *func_name) {
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(func_name, builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void execute(char *line) {
    char *args[MAX_ARGS];
    char **args_ptr = args;
    int argc = parse(line, args);

    int out_fd = -1;
    int in_fd = -1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], ">") == 0) {
            out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) {
            out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            in_fd = open(args[i + 1], O_RDONLY);
            args[i] = NULL;
        }
    }

    if (isBuiltin(*args_ptr)) {
        execute_builtin_command(args_ptr);
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (out_fd != -1) {
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        } else {
            wait(NULL);
            if (in_fd != -1) close(in_fd);
            if (out_fd != -1) close(out_fd);
        }
    }
}

void shell_interact() {
    char *line; // 存储输入的命令行

    while (1) {
        fflush(stdout);
        char *prefix = update_prefix();
        line = readline(prefix);

        if (!line) {
            break;
        }

        if (line[0] != '\0') {
            add_history(line); // 保存历史，支持上下键翻历史命令
        }

        // 解析命令行参数
        execute(line);
    }
}

void shell_script(char *filename) {
    printf("Received Script. Opening %s", filename);
    FILE *fptr;
    char line[200];
    char **args;
    fptr = fopen(filename, "r");
    if (fptr == NULL) {
        printf("\nUnable to open file.");
    } else {
        printf("\nFile Opened. Parsing. Parsed commands displayed first.");
        while (fgets(line, sizeof(line), fptr) != NULL) {
            printf("\n%s", line);
        }
    }
    free(args);
    fclose(fptr);
}

void display_hello() {
    printf("Welcome to lhy's Shell!\n");
    printf("This is the final project of UNIX at BUAA.\n");
    printf("Author: Haoyu Luo\n");
    printf("Student ID: 23373112\n");
    printf(" _      _   _ __   __        ____   _   _  _____  _      _ \n");
    printf("| |    | | | |\\ \\ / /       / ___| | | | || ____|| |    | |\n");
    printf("| |    | |_| | \\ V /  _____ \\___ \\ | |_| ||  _|  | |    | |    \n");
    printf("| |___ |  _  |  | |  |_____| ___) ||  _  || |___ | |___ | |___ \n");
    printf("|_____||_| |_|  |_|         |____/ |_| |_||_____||_____||_____|\n\n");
}

char *update_prefix() {
    char *home = getenv("HOME");
    char *current_dir = getcwd(NULL, 0);
    if (current_dir == NULL) {
        perror("getcwd");
        exit(1);
    }
    struct passwd *pwd = getpwuid(getuid());
    char *hostname = (char *) malloc(MAX_LINE);
    if (gethostname(hostname, MAX_LINE) == -1) {
        perror("gethostname");
        exit(1);
    }
    hostname[strcspn(hostname, ".")] = 0;
    hostname[strcspn(hostname, "\n")] = 0;
    char *dir = strstr(current_dir, home);
    char *prefix_dir = (char *) malloc(strlen(home) + 1);
    if (dir != NULL) {
        prefix_dir[0] = '~';
        strcpy(prefix_dir + 1, dir + strlen(home));
    } else {
        prefix_dir = current_dir;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf), RED "[lhy's Shell] " BLUE "%s@%s:" YELLOW "%s" WHITE "$ ", pwd->pw_name, hostname,
             prefix_dir);
    return strdup(buf);
}
