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
void shell_script();

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

void shell_interact() {
    char *line;    // 存储输入的命令行

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

void shell_script(char *filename) {
    printf("Received Script. Opening %s", filename);
	FILE *fptr;
	char line[200];
	char **args;
	fptr = fopen(filename, "r");
	if (fptr == NULL)
	{
		printf("\nUnable to open file.");
	}
	else
	{
		printf("\nFile Opened. Parsing. Parsed commands displayed first.");
		while(fgets(line, sizeof(line), fptr)!= NULL)
		{
			printf("\n%s", line);
		}
	}
	free(args);
	fclose(fptr);
}

int isBuiltin(char *func_name) {
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(func_name, builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
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

void execute_command(char *args[], int input_fd, int output_fd) {
    pid_t pid = fork();
    if (pid == 0) {
        if (input_fd != 0) { 
            dup2(input_fd, 0);
            close(input_fd);
        }
        if (output_fd != 1) {
            dup2(output_fd, 1);
            close(output_fd);
        }
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else {
        wait(NULL)
    }
}

void execute(char *line) {
    char *args[MAX_ARGS];
    char *args_copy = args;
    int argc = parse(line, args);

    int pipefd[2 * (argc - 1)];
    int pipe_cnt = 0;

    // cmd1 | cmd2 | cmd3 | cmd4
    //     0\1    2\3    4\5
    for (int i = 0; i < argc; ++i) {
        if (strcmp(args[i], "|") == 0) {
            args_copy[i] = NULL;
            pipe_cnt++;
            if (pipe(pipefd + (pipe_cnt - 1) * 2) == -1) {
                perror("pipe");
                exit(1);
            }
            if (pipe_cnt == 1) execute_command(args, 0, pipefd[(pipe_cnt - 1) * 2 + 1]);
            else execute_command(args, pipefd[(pipe_cnt - 2) * 2], pipefd[(pipe_cnt - 1) * 2 + 1]);
            args = args_copy + i;
        }
    }
    // 执行最后一个命令，输入重定向自前一个管道，输出重定向到标准输出
    execute_command(args, pipefd[(pipe_cnt - 1) * 2], 1);

    // 等待所有子进程完成
    for (int i = 0; i < 2 * pipe_cnt; i++) {
        close(pipefd[i]);
    }
}

void execute_out_command(char *args[], int input_fd, int output_fd) {
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
}

void execute_builtin_command(char *args[], int input_fd, int output_fd) {
    if (strcmp(args[0], "cd") == 0) {
        cd(args);
    } else if (strcmp(args[0], "exit") == 0) {
        exit_lsh(args);
    } else {
        fprintf(stderr, "%s: command not found\n", args[0]);
    }
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
    char buf[1024];
    snprintf(buf, sizeof(buf), RED "[lhy's Shell] " BLUE "%s@%s:" YELLOW "%s" WHITE "$ ", pwd->pw_name, hostname, prefix_dir);
    return strdup(buf);
}