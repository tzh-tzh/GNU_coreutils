#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

// 函数声明
void ls_command(int argc, char *argv[]);
void cp_command(int argc, char *argv[]);
void cat_command(int argc, char *argv[]);
void echo_command(int argc, char *argv[]);
void mkdir_command(int argc, char *argv[]);
void rmdir_command(int argc, char *argv[]);
void rm_command(int argc, char *argv[]);
void touch_command(int argc, char *argv[]);
void pwd_command(int argc, char *argv[]);
void cd_command(int argc, char *argv[]);
void mv_command(int argc, char *argv[]);
void ps_command(int argc, char *argv[]);

// 分割命令行参数
/**
 * 解析输入字符串并将其分割为命令行参数数组
 * @param input 输入的命令行字符串
 * @param argv 用于存储分割后参数的字符指针数组
 * @return 返回解析出的参数个数
 */
int parse_command(char *input, char **argv) {
    int argc = 0;
    char *token = strtok(input, " ");
    
    while (token != NULL && argc < 10) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    argv[argc] = NULL;
    return argc;
}

// 显示指令提示信息
void show_help() {
    printf("\n可用命令:\n");
    printf("  ls [路径]                  - 列出目录内容\n");
    printf("  cp <源文件> <目标文件>      - 复制文件\n");
    printf("  cat [文件1] [文件2] ...     - 显示文件内容\n");
    printf("  echo [文本]                - 输出文本\n");
    printf("  mkdir <目录1> [目录2] ...   - 创建目录\n");
    printf("  rmdir <目录1> [目录2] ...   - 删除空目录\n");
    printf("  rm [OPTION]... FILE...     - 删除文件或目录\n");
    printf("  touch [OPTION]... FILE...  - 更新文件的访问时间和修改时间\n");
    printf("  pwd [OPTION]...            - 显示当前工作目录\n");
    printf("  cd [目录路径]               - 切换当前工作目录\n");
    printf("  mv [OPTION]... SOURCE... DEST - 移动或重命名文件或目录\n");
    printf("  ps [OPTION]...             - 显示当前系统中运行的进程快照\n");
    printf("  exit/quit                  - 退出控制台\n\n");
}

int main() {
    char input[256];
    char *argv[10];
    int argc;
    
    printf("Simple GNU Coreutils Interactive Console\n");
    printf("Type 'exit' or 'quit' to exit\n");
    show_help(); // 第一次进入时显示提示
    
    while (1) {
        // 显示当前工作目录和提示符
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s> ", cwd);
        } else {
            printf("> ");
        }
        
        // 读取用户输入
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 去除换行符
        input[strcspn(input, "\n")] = '\0';
        
        // 分割命令行参数
        argc = parse_command(input, argv);
        
        if (argc == 0) {
            // 用户没有输入任何指令，直接按了 Enter 键
            show_help();
            continue;
        }
        
        // 检查退出命令
        if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
            break;
        }
        
        // 命令分发
        if (strcmp(argv[0], "ls") == 0) {
            ls_command(argc, argv);
        } else if (strcmp(argv[0], "cp") == 0) {
            cp_command(argc, argv);
        } else if (strcmp(argv[0], "cat") == 0) {
            cat_command(argc, argv);
        } else if (strcmp(argv[0], "echo") == 0) {
            echo_command(argc, argv);
        } else if (strcmp(argv[0], "mkdir") == 0) {
            mkdir_command(argc, argv);
        } else if (strcmp(argv[0], "rmdir") == 0) {
            rmdir_command(argc, argv);
        } else if (strcmp(argv[0], "rm") == 0) {
            rm_command(argc, argv);
        } else if (strcmp(argv[0], "touch") == 0) {
            touch_command(argc, argv);
        } else if (strcmp(argv[0], "pwd") == 0) {
            pwd_command(argc, argv);
        } else if (strcmp(argv[0], "cd") == 0) {
            cd_command(argc, argv);
        } else if (strcmp(argv[0], "mv") == 0) {
            mv_command(argc, argv);
        } else if (strcmp(argv[0], "ps") == 0) {
            ps_command(argc, argv);
        } else {
            printf("Unknown command: %s\n", argv[0]);
            show_help();
        }
        
        printf("\n");
    }
    
    printf("Exiting console...\n");
    return 0;
}
