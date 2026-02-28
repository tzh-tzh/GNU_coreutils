#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

// 选项结构体
typedef struct {
    int l;  // 长格式显示
    int a;  // 显示所有文件（包括隐藏文件）
    int h;  // 以易读单位显示文件大小
    int t;  // 按修改时间排序
    int r;  // 反向排序
    int S;  // 按文件大小排序
    int d;  // 仅显示目录本身
    int R;  // 递归列出子目录
} ls_options;

// 全局指针，用于在比较函数中访问选项
ls_options *current_options;

// 文件信息结构体，用于排序
typedef struct {
    char name[256];
    struct stat st;
} file_info;

// 比较函数：按名称排序（与标准ls一致，隐藏文件排在前面）
int compare_name(const void *a, const void *b) {
    const file_info *fa = (const file_info *)a;
    const file_info *fb = (const file_info *)b;
    
    // 检查是否是隐藏文件
    int fa_hidden = (fa->name[0] == '.');
    int fb_hidden = (fb->name[0] == '.');
    
    // 隐藏文件排在前面
    if (fa_hidden && !fb_hidden) {
        return -1;
    } else if (!fa_hidden && fb_hidden) {
        return 1;
    }
    
    // 都是隐藏文件或都是非隐藏文件，按名称排序
    int result = strcmp(fa->name, fb->name);
    return current_options->r ? -result : result;
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
    return current_options->r ? -result : result;
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
    return current_options->r ? -result : result;
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
void print_file_info(const char *path, const char *name, ls_options *opts) {
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

    // 打印所有者和组
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    if (pw != NULL) {
        printf("%-8s ", pw->pw_name);
    } else {
        printf("%5ld ", (long)st.st_uid);
    }
    if (gr != NULL) {
        printf("%-8s ", gr->gr_name);
    } else {
        printf("%5ld ", (long)st.st_gid);
    }

    // 打印文件大小
    if (opts->h) {
        format_size(st.st_size, size_str);
        printf("%8s ", size_str);
    } else {
        printf("%8ld ", (long)st.st_size);
    }

    // 打印修改时间
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
    printf("%s ", time_str);

    // 打印文件名
    printf("%s\n", name);
}

/*
 * 列出指定目录的内容
 * @param path: 要列出的目录路径
 * @param level: 递归层级（用于控制缩进和避免无限递归）
 * @param opts: 选项结构体指针
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
void list_directory(const char *path, int level, ls_options *opts) {
    DIR *dir;
    struct dirent *entry;
    char full_path[512];
    file_info *files = NULL;
    int file_count = 0;
    int i;

    // 设置全局指针，用于比较函数
    current_options = opts;

    // 打开目录
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    // 读取所有目录项
    while ((entry = readdir(dir)) != NULL) {
        // 跳过隐藏文件，除非指定了 -a 选项
        if (!opts->a && entry->d_name[0] == '.') {
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

    // 计算并显示磁盘块总数（与标准ls一致，以1024字节为单位）
    // 只有在使用 -l 选项时才显示"总计"行
    if (files != NULL && file_count > 0 && opts->l && !opts->d) {
        long total_blocks = 0;
        for (int j = 0; j < file_count; j++) {
            total_blocks += files[j].st.st_blocks;
        }
        // 标准ls命令显示的是1024字节为单位的块数，所以除以2
        printf("总计 %ld\n", total_blocks / 2);
    }

    // 处理和显示文件列表
    if (files != NULL && file_count > 0) {
        // 根据指定选项对文件进行排序
        if (opts->t) {
            // 按修改时间排序
            qsort(files, file_count, sizeof(file_info), compare_time);
        } else if (opts->S) {
            // 按文件大小排序
            qsort(files, file_count, sizeof(file_info), compare_size);
        } else {
            // 按名称排序
            qsort(files, file_count, sizeof(file_info), compare_name);
        }

        // 打印文件信息
        if (opts->d) {
            // 仅显示目录本身
            if (opts->l) {
                print_file_info(".", path, opts);
            } else {
                printf("%s\n", path);
            }
        } else {
            // 列出目录内容
            for (i = 0; i < file_count; i++) {
                if (opts->l) {
                    // 长格式显示
                    print_file_info(path, files[i].name, opts);
                } else {
                    // 简单格式显示
                    printf("%s\n", files[i].name);
                }
            }

            // 递归列出子目录内容（如果指定了 -R 选项）
            for (i = 0; i < file_count; i++) {
                if (opts->R && S_ISDIR(files[i].st.st_mode) && 
                    strcmp(files[i].name, ".") != 0 && 
                    strcmp(files[i].name, "..") != 0) {
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);
                    printf("\n%s/:\n", full_path);
                    list_directory(full_path, level + 1, opts);
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
    ls_options opts = {0}; // 初始化选项结构体

    /* 解析命令行选项和参数 */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            /* 处理选项参数 */
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'l':
                        opts.l = 1;
                        break;
                    case 'a':
                        opts.a = 1;
                        break;
                    case 'h':
                        opts.h = 1;
                        break;
                    case 't':
                        opts.t = 1;
                        break;
                    case 'r':
                        opts.r = 1;
                        break;
                    case 'S':
                        opts.S = 1;
                        break;
                    case 'd':
                        opts.d = 1;
                        break;
                    case 'R':
                        opts.R = 1;
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
    list_directory(path, 0, &opts);
}
