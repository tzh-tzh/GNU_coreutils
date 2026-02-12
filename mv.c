#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

// 函数声明
int file_exists(const char *path);
int is_directory(const char *path);

// 选项结构体
typedef struct {
    int i;  // 交互模式：覆盖前提示确认
    int f;  // 强制模式：不提示，直接覆盖
    int v;  // 显示详细过程
    int u;  // 仅当源文件比目标新或目标不存在时才移动
    int n;  // 不覆盖已存在文件
} mv_options;

// 检查源文件是否比目标文件新
int is_source_newer(const char *source, const char *dest) {
    struct stat st_source, st_dest;
    if (stat(source, &st_source) == -1 || stat(dest, &st_dest) == -1) {
        return 0;
    }
    return st_source.st_mtime > st_dest.st_mtime;
}

// 交互模式：覆盖前提示确认
int confirm_overwrite(const char *dest) {
    char response[10];
    printf("mv: overwrite '%s'? ", dest);
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    return response[0] == 'y' || response[0] == 'Y';
}

/**
 * 执行mv命令，移动或重命名文件或目录
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void mv_command(int argc, char *argv[]) {
    int i, j;
    mv_options opts = {0}; // 初始化选项结构体
    int source_count = 0;
    char *sources[100];
    char *dest = NULL;

    // 解析命令行选项和参数
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // 处理选项参数
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'i':
                        opts.i = 1;
                        break;
                    case 'f':
                        opts.f = 1;
                        break;
                    case 'v':
                        opts.v = 1;
                        break;
                    case 'u':
                        opts.u = 1;
                        break;
                    case 'n':
                        opts.n = 1;
                        break;
                    default:
                        printf("mv: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: mv [OPTION]... SOURCE... DEST\n");
                        return;
                }
            }
        } else {
            // 处理源文件或目标参数
            sources[source_count++] = argv[i];
        }
    }

    // 检查参数数量
    if (source_count < 2) {
        printf("mv: missing file operand\n");
        return;
    }

    // 最后一个参数是目标
    dest = sources[--source_count];

    // 检查目标是否是目录
    int dest_is_dir = is_directory(dest);

    // 处理多个源文件的情况
    if (source_count > 1) {
        if (!dest_is_dir) {
            printf("mv: target '%s' is not a directory\n", dest);
            return;
        }
    }

    // 处理每个源文件
    for (i = 0; i < source_count; i++) {
        char *source = sources[i];
        char dest_path[PATH_MAX];

        // 构建目标路径
        if (dest_is_dir) {
            // 目标是目录，构建完整的目标路径
            char *basename = strrchr(source, '/');
            if (basename == NULL) {
                basename = source;
            } else {
                basename++;
            }
            snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, basename);
        } else {
            // 目标不是目录，直接使用目标路径
            strcpy(dest_path, dest);
        }

        // 检查源文件是否存在
        if (!file_exists(source)) {
            printf("mv: cannot stat '%s': No such file or directory\n", source);
            continue;
        }

        // 检查目标文件是否存在
        if (file_exists(dest_path)) {
            // 目标文件存在
            if (opts.n) {
                // 不覆盖已存在文件
                continue;
            }

            if (opts.u && !is_source_newer(source, dest_path)) {
                // 仅当源文件比目标新时才移动
                continue;
            }

            if (opts.i && !opts.f) {
                // 交互模式：覆盖前提示确认
                if (!confirm_overwrite(dest_path)) {
                    continue;
                }
            }
        }

        // 执行移动操作
        if (rename(source, dest_path) == -1) {
            perror("mv");
            continue;
        }

        // 显示详细过程
        if (opts.v) {
            printf("%s -> %s\n", source, dest_path);
        }
    }
}
