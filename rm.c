#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// 选项结构体
typedef struct {
    int f;  // 强制删除，忽略不存在的文件，不提示确认
    int i;  // 每次删除前交互式确认
    int r;  // 递归删除目录及其内容
    int v;  // 显示详细过程
} rm_options;

// 函数声明
int file_exists(const char *path);
int is_directory(const char *path);

// 递归删除目录
int remove_directory(const char *path, rm_options *opts) {
    DIR *dir;
    struct dirent *entry;
    char full_path[1024];

    // 打开目录
    dir = opendir(path);
    if (dir == NULL) {
        if (!opts->f) {
            perror(path);
        }
        return -1;
    }

    // 读取目录内容
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 构建完整路径
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        // 检查是否是目录
        if (is_directory(full_path)) {
            // 递归删除子目录
            if (remove_directory(full_path, opts) == -1) {
                closedir(dir);
                return -1;
            }
        } else {
            // 删除文件
            if (opts->i) {
                // 交互式确认
                char response;
                printf("rm: remove regular file '%s'? ", full_path);
                if (scanf("%c", &response) != 1 || response != 'y') {
                    continue;
                }
            }

            if (unlink(full_path) == -1) {
                if (!opts->f) {
                    perror(full_path);
                }
            } else if (opts->v) {
                printf("removed '%s'\n", full_path);
            }
        }
    }

    // 关闭目录
    closedir(dir);

    // 删除空目录
    if (opts->i) {
        // 交互式确认
        char response;
        printf("rm: remove directory '%s'? ", path);
        if (scanf("%c", &response) != 1 || response != 'y') {
            return 0;
        }
    }

    if (rmdir(path) == -1) {
        if (!opts->f) {
            perror(path);
        }
        return -1;
    } else if (opts->v) {
        printf("removed directory '%s'\n", path);
    }

    return 0;
}

// 删除文件
int remove_file(const char *path, rm_options *opts) {
    if (opts->i) {
        // 交互式确认
        char response;
        printf("rm: remove regular file '%s'? ", path);
        if (scanf("%c", &response) != 1 || response != 'y') {
            return 0;
        }
    }

    if (unlink(path) == -1) {
        if (!opts->f) {
            perror(path);
        }
        return -1;
    } else if (opts->v) {
        printf("removed '%s'\n", path);
    }

    return 0;
}

/**
 * 执行rm命令，删除文件或目录
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void rm_command(int argc, char *argv[]) {
    int i, j;
    rm_options opts = {0}; // 初始化选项结构体

    // 解析命令行选项和参数
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // 处理选项参数
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'f':
                        opts.f = 1;
                        break;
                    case 'i':
                        opts.i = 1;
                        break;
                    case 'r':
                    case 'R':
                        opts.r = 1;
                        break;
                    case 'v':
                        opts.v = 1;
                        break;
                    default:
                        printf("rm: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: rm [OPTION]... FILE...\n");
                        return;
                }
            }
        } else {
            // 处理文件/目录参数
            if (!file_exists(argv[i])) {
                if (!opts.f) {
                    printf("rm: cannot remove '%s': No such file or directory\n", argv[i]);
                }
                continue;
            }

            if (is_directory(argv[i])) {
                if (!opts.r) {
                    printf("rm: cannot remove '%s': Is a directory\n", argv[i]);
                    continue;
                }
                // 递归删除目录
                remove_directory(argv[i], &opts);
            } else {
                // 删除文件
                remove_file(argv[i], &opts);
            }
        }
    }
}
