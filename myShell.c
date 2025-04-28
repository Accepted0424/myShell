#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>

#define MAX_LINE 80    // 最大命令行字符数
#define MAX_ARGS 10    // 最大参数数目

// color define
#define BLUE "\e[1;34m"
#define YELLOW "\e[1;33m"
#define WHITE "\e[0;37m"
#define RED "\e[1;31m"
#define GREEN "\e[1;32m"

#define EXECUTE_BUILTIN_COMMAND(func, args) \
    do {                                    \
        func(args);                         \
    } while (0);

void shell_interact();
void shell_script();
void execute_command(char *args[], int input_fd, int output_fd);
void execute_out_command(char *args[], int input_fd, int output_fd);
void execute_builtin_command(char *args[], int input_fd, int output_fd);
void display_hello();
void display_prefix();

const char *builtins[] = {
    "cd",
    "exit",
    NULL 
};

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
        shell_script();
    } else {
        fprintf(stderr, "Usage: %s [script_file]\n", argv[0]);
        exit(1);
    }

    return 0;
}

void shell_interact() {
    char line[MAX_LINE];    // 存储输入的命令行
    char *args[MAX_ARGS];   // 命令行参数列表

    while (1) {
        display_prefix();
        fflush(stdout);
        // 读取输入命令行
        if (!fgets(line, MAX_LINE, stdin)) {
            break;
        }

        // 移除末尾的换行符
        line[strcspn(line, "\n")] = 0;

        // 初始化参数列表
        int i;
        for (i = 0; i < MAX_ARGS; i++) {
            args[i] = NULL;
        }

        // 解析命令行参数
        char *token = strtok(line, " ");
        i = 0;
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // 检查是否存在管道操作符 "|"
        char *pipe_token = strchr(line, '|');
        if (pipe_token != NULL) {
            // 建立管道
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(1);
            }

            // 解析第一个命令和第二个命令
            *pipe_token = '\0';
            char *first_command = args[0];
            char *second_command = pipe_token + 1;

            // 执行第一个命令，输出重定向到管道写入端
            execute_command(args, 0, pipefd[1]);

            // 执行第二个命令，输入从管道读取端获取
            args[0] = second_command;
            execute_command(args, pipefd[0], 1);

            // 关闭管道
            close(pipefd[0]);
            close(pipefd[1]);
        } else {
            // 普通命令，直接执行
            execute_command(args, 0, 1);
        }
    }
}

int isBuiltin(char *func_name) {
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(func_name, builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void execute_command(char *args[], int input_fd, int output_fd) {
    // 检查是否是内置命令
    if (isBuiltin(args[0])) {
        execute_builtin_command(args, input_fd, output_fd);
        return;
    } else {
        execute_out_command(args, input_fd, output_fd);
    }
}

void execute_out_command(char *args[], int input_fd, int output_fd) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process

        // 如果存在输入重定向，则将输入重定向到指定文件描述符
        if (input_fd != 0) {
            dup2(input_fd, 0);
            close(input_fd);
        }

        // 如果存在输出重定向，则将输出重定向到指定文件描述符
        if (output_fd != 1) {
            dup2(output_fd, 1);
            close(output_fd);
        }

        // 执行外部命令
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
    } else {
        // Fork failed
        perror("fork");
        exit(1);
    }
}

void execute_builtin_command(char *args[], int input_fd, int output_fd) {
    EXECUTE_BUILTIN_COMMAND(args[0], args);
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

void display_prefix() {
    char *home = getenv("HOME");
    char *current_dir = getcwd(NULL, 0);
    if (current_dir == NULL) {
        perror("getcwd");
        exit(1);
    }
    struct passwd *pwd = getpwuid(getuid());
    char *hostname = (char *)malloc(MAX_LINE);
    if (gethostname(hostname, MAX_LINE) == -1) {
        perror("gethostname");
        exit(1);
    }
    hostname[strcspn(hostname, ".")] = 0;
    hostname[strcspn(hostname, "\n")] = 0;
    char *dir = strstr(current_dir, home);
    char *prefix_dir = (char *)malloc(strlen(home) + 1);
    if (dir != NULL) {
        prefix_dir[0] = '~';
        strcpy(prefix_dir + 1, dir + strlen(home));
    } else {
        prefix_dir = current_dir;
    }
    printf(RED "[lhy's Shell] " BLUE "%s@%s:" YELLOW "%s" WHITE "$ ", pwd->pw_name, hostname, prefix_dir);
    free(current_dir);
}

// builtin command
void cd(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (chdir(args[1]) != 0) {
        perror("cd");
    }
}

void exit_lsh(char *args[]) {
    exit(0);
}

void echo(char *args[]) {
    int i = 1;
    while (args[i] != NULL) {
        printf("%s ", args[i]);
        i++;
    }
    printf("\n");
}