#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

// 选项标志
int opt_l = 0;  // 长格式显示
int opt_a = 0;  // 显示所有文件（包括隐藏文件）
int opt_h = 0;  // 以易读单位显示文件大小
int opt_t = 0;  // 按修改时间排序
int opt_r = 0;  // 反向排序
int opt_S = 0;  // 按文件大小排序
int opt_d = 0;  // 仅显示目录本身
int opt_R = 0;  // 递归列出子目录

// 文件信息结构体，用于排序
typedef struct {
    char name[256];
    struct stat st;
} file_info;

// 比较函数：按名称排序
int compare_name(const void *a, const void *b) {
    const file_info *fa = (const file_info *)a;
    const file_info *fb = (const file_info *)b;
    int result = strcmp(fa->name, fb->name);
    return opt_r ? -result : result;
}

// 比较函数：按修改时间排序
int compare_time(const void *a, const void *b) {
    const file_info *fa = (const file_info *)a;
    const file_info *fb = (const file_info *)b;
    int result;
    if (fa->st.st_mtime > fb->st.st_mtime) {
        result = -1;
    } else if (fa->st.st_mtime < fb->st.st_mtime) {
        result = 1;
    } else {
        result = 0;
    }
    return opt_r ? -result : result;
}

// 比较函数：按文件大小排序
int compare_size(const void *a, const void *b) {
    const file_info *fa = (const file_info *)a;
    const file_info *fb = (const file_info *)b;
    int result;
    if (fa->st.st_size > fb->st.st_size) {
        result = -1;
    } else if (fa->st.st_size < fb->st.st_size) {
        result = 1;
    } else {
        result = 0;
    }
    return opt_r ? -result : result;
}

// 格式化文件大小为易读格式
void format_size(off_t size, char *buffer) {
    if (size < 1024) {
        sprintf(buffer, "%ld", (long)size);
    } else if (size < 1024 * 1024) {
        sprintf(buffer, "%.1fK", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        sprintf(buffer, "%.1fM", size / (1024.0 * 1024.0));
    } else {
        sprintf(buffer, "%.1fG", size / (1024.0 * 1024.0 * 1024.0));
    }
}

// 打印文件权限
void print_permissions(mode_t mode) {
    printf("%c", S_ISDIR(mode) ? 'd' : '-');
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

// 打印文件信息（长格式）
void print_file_info(const char *path, const char *name) {
    char full_path[512];
    struct stat st;
    char size_str[32];
    char time_str[64];

    // 构建完整路径
    snprintf(full_path, sizeof(full_path), "%s/%s", path, name);

    // 获取文件状态
    if (stat(full_path, &st) == -1) {
        perror("stat");
        return;
    }

    // 打印权限
    print_permissions(st.st_mode);
    printf(" ");

    // 打印链接数
    printf("%3ld ", (long)st.st_nlink);

    // 打印所有者和组（简化版，只显示UID和GID）
    printf("%5ld %5ld ", (long)st.st_uid, (long)st.st_gid);

    // 打印文件大小
    if (opt_h) {
        format_size(st.st_size, size_str);
        printf("%8s ", size_str);
    } else {
        printf("%8ld ", (long)st.st_size);
    }

    // 打印修改时间
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
    printf("%s ", time_str);

    // 打印文件名
    printf("%s\n", name);
}

/*
 * 列出指定目录的内容
 * @param path: 要列出的目录路径
 * @param level: 递归层级（用于控制缩进和避免无限递归）
 * @return: 无返回值
 * 
 * 该函数实现类似ls命令的功能，支持多种选项：
 * - -a: 显示隐藏文件
 * - -l: 长格式显示
 * - -t: 按修改时间排序
 * - -S: 按文件大小排序
 * - -R: 递归显示子目录
 * - -d: 仅显示目录本身而不显示其内容
 */
void list_directory(const char *path, int level) {
    DIR *dir;
    struct dirent *entry;
    char full_path[512];
    file_info *files = NULL;
    int file_count = 0;
    int i;

    // 打开目录
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    // 读取所有目录项
    while ((entry = readdir(dir)) != NULL) {
        // 跳过隐藏文件，除非指定了 -a 选项
        if (!opt_a && entry->d_name[0] == '.') {
            continue;
        }

        // 分配内存存储文件信息
        if (file_count == 0) {
            // 第一次分配，使用 malloc
            files = malloc(sizeof(file_info));
        } else {
            // 后续分配，使用 realloc
            files = realloc(files, (file_count + 1) * sizeof(file_info));
        }
        if (files == NULL) {
            perror("malloc/realloc");
            closedir(dir);
            return;
        }

        // 存储文件名
        strcpy(files[file_count].name, entry->d_name);

        // 构建完整路径并获取文件状态
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        stat(full_path, &files[file_count].st);

        file_count++;
    }

    // 关闭目录
    closedir(dir);

    // 处理和显示文件列表
    if (files != NULL && file_count > 0) {
        // 根据指定选项对文件进行排序
        if (opt_t) {
            // 按修改时间排序
            qsort(files, file_count, sizeof(file_info), compare_time);
        } else if (opt_S) {
            // 按文件大小排序
            qsort(files, file_count, sizeof(file_info), compare_size);
        } else {
            // 按名称排序
            qsort(files, file_count, sizeof(file_info), compare_name);
        }

        // 打印文件信息
        if (opt_d) {
            // 仅显示目录本身
            if (opt_l) {
                print_file_info(".", path);
            } else {
                printf("%s\n", path);
            }
        } else {
            // 列出目录内容
            for (i = 0; i < file_count; i++) {
                if (opt_l) {
                    // 长格式显示
                    print_file_info(path, files[i].name);
                } else {
                    // 简单格式显示
                    printf("%s\n", files[i].name);
                }
            }

            // 递归列出子目录内容（如果指定了 -R 选项）
            for (i = 0; i < file_count; i++) {
                if (opt_R && S_ISDIR(files[i].st.st_mode) && 
                    strcmp(files[i].name, ".") != 0 && 
                    strcmp(files[i].name, "..") != 0) {
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);
                    printf("\n%s/:\n", full_path);
                    list_directory(full_path, level + 1);
                }
            }
        }

        // 释放内存
        free(files);
    }
}

/**
 * 执行ls命令，列出目录内容
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void ls_command(int argc, char *argv[]) {
    char *path = ".";
    char expanded_path[256];
    int i, j;

    /* 初始化选项标志 */
    opt_l = 0;
    opt_a = 0;
    opt_h = 0;
    opt_t = 0;
    opt_r = 0;
    opt_S = 0;
    opt_d = 0;
    opt_R = 0;

    /* 解析命令行选项和参数 */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            /* 处理选项参数 */
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'l':
                        opt_l = 1;
                        break;
                    case 'a':
                        opt_a = 1;
                        break;
                    case 'h':
                        opt_h = 1;
                        break;
                    case 't':
                        opt_t = 1;
                        break;
                    case 'r':
                        opt_r = 1;
                        break;
                    case 'S':
                        opt_S = 1;
                        break;
                    case 'd':
                        opt_d = 1;
                        break;
                    case 'R':
                        opt_R = 1;
                        break;
                    default:
                        printf("ls: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: ls [OPTION]... [FILE]...\n");
                        return;
                }
            }
        } else {
            /* 设置目标路径 */
            path = argv[i];
        }
    }

    /* 处理用户主目录符号(~) */
    if (path[0] == '~') {
#ifdef _WIN32
        /* 获取Windows系统用户配置文件路径 */
        char *home = getenv("USERPROFILE");
#else
        /* 获取Unix/Linux系统用户主目录路径 */
        char *home = getenv("HOME");
#endif
        if (home != NULL) {
            /* 构建完整的用户目录路径 */
            strcpy(expanded_path, home);
            if (strlen(path) > 1) {
                /* 追加~符号后的相对路径部分 */
                strcat(expanded_path, path + 1);
            }
            path = expanded_path;
        }
    }

    /* 调用核心函数显示目录内容 */
    list_directory(path, 0);
}
