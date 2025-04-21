#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80    // 最大命令行字符数
#define MAX_ARGS 10    // 最大参数数目

void execute_command(char *args[], int input_fd, int output_fd) {
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

int main() {
    char line[MAX_LINE];    // 存储输入的命令行
    char *args[MAX_ARGS];   // 命令行参数列表
    int should_run = 1;     // 控制循环结束的标志

    while (should_run) {
        printf("lhy@myShell$ ");
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

    return 0;
}
