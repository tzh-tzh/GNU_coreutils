#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

// 选项结构体
typedef struct {
    int L;  // 显示逻辑路径（保留符号链接名称）
    int P;  // 显示物理路径（解析并展开所有符号链接）
} pwd_options;

/**
 * 执行pwd命令，显示当前工作目录
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void pwd_command(int argc, char *argv[]) {
    int i, j;
    pwd_options opts = {0}; // 初始化选项结构体
    char cwd[PATH_MAX];
    char *result;
    char *path = NULL;

    // 解析命令行选项和参数
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // 处理选项参数
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'L':
                        opts.L = 1;
                        break;
                    case 'P':
                        opts.P = 1;
                        break;
                    default:
                        printf("pwd: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: pwd [OPTION]... [PATH]\n");
                        return;
                }
            }
        } else {
            // 处理路径参数
            path = argv[i];
        }
    }

    // 如果指定了路径参数
    if (path != NULL) {
        // 处理 ~ 符号
        if (path[0] == '~') {
            char *home = getenv("HOME");
            if (home != NULL) {
                char expanded_path[PATH_MAX];
                if (strlen(path) == 1) {
                    // 只有 ~ 符号
                    strcpy(expanded_path, home);
                } else if (path[1] == '/') {
                    // ~/path 格式
                    sprintf(expanded_path, "%s%s", home, path + 1);
                } else {
                    // 其他格式，保持不变
                    strcpy(expanded_path, path);
                }
                path = expanded_path;
            }
        }

        // 显示指定路径
        if (opts.P) {
            // 显示物理路径（解析并展开所有符号链接）
            char *resolved_path = realpath(path, NULL);
            if (resolved_path != NULL) {
                printf("%s\n", resolved_path);
                free(resolved_path);
            } else {
                perror("pwd");
            }
        } else {
            // 显示逻辑路径（保留符号链接名称）
            if (realpath(path, cwd) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd");
            }
        }
    } else {
        // 默认显示当前工作目录
        // 默认使用 -L 选项
        if (!opts.P) {
            // 显示逻辑路径（保留符号链接名称）
            result = getcwd(cwd, sizeof(cwd));
            if (result != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd");
            }
        } else {
            // 显示物理路径（解析并展开所有符号链接）
            result = getcwd(cwd, sizeof(cwd));
            if (result != NULL) {
                // 使用 realpath 解析符号链接
                char *resolved_path = realpath(cwd, NULL);
                if (resolved_path != NULL) {
                    printf("%s\n", resolved_path);
                    free(resolved_path);
                } else {
                    perror("pwd");
                }
            } else {
                perror("pwd");
            }
        }
    }
}
