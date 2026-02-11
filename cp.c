#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

// 选项标志
int opt_r = 0;  // 递归复制目录
int opt_i = 0;  // 交互式覆盖确认
int opt_f = 0;  // 强制覆盖
int opt_v = 0;  // 显示复制过程
int opt_a = 0;  // 归档模式
int opt_p = 0;  // 保留文件属性
int opt_u = 0;  // 仅更新
int opt_l = 0;  // 创建硬链接
int opt_s = 0;  // 创建软链接

// 检查文件是否存在
int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

// 检查是否是目录
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

// 复制单个文件
int copy_file(const char *src_path, const char *dest_path) {
    int src_fd, dest_fd;
    char buffer[4096];
    ssize_t n;
    struct stat src_stat;

    // 获取源文件状态
    if (stat(src_path, &src_stat) == -1) {
        perror("stat source");
        return -1;
    }

    // 检查目标文件是否存在
    if (file_exists(dest_path)) {
        // 检查是否需要更新
        if (opt_u) {
            struct stat dest_stat;
            if (stat(dest_path, &dest_stat) == 0) {
                if (src_stat.st_mtime <= dest_stat.st_mtime) {
                    if (opt_v) {
                        printf("%s is newer or same age as %s, skipping\n", dest_path, src_path);
                    }
                    return 0;
                }
            }
        }

        // 交互式确认
        if (opt_i && !opt_f) {
            char response;
            printf("cp: overwrite '%s'? ", dest_path);
            if (scanf("%c", &response) != 1 || response != 'y') {
                return 0;
            }
        }
    }

    // 创建硬链接
    if (opt_l) {
        if (link(src_path, dest_path) == -1) {
            perror("link");
            return -1;
        }
        if (opt_v) {
            printf("%s -> %s (hard link)\n", src_path, dest_path);
        }
        return 0;
    }

    // 创建软链接
    if (opt_s) {
        if (symlink(src_path, dest_path) == -1) {
            perror("symlink");
            return -1;
        }
        if (opt_v) {
            printf("%s -> %s (symbolic link)\n", src_path, dest_path);
        }
        return 0;
    }

    // 打开源文件
    src_fd = open(src_path, O_RDONLY);
    if (src_fd == -1) {
        perror("open source");
        return -1;
    }

    // 打开目标文件
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    if (opt_f) {
        flags |= O_EXCL;
    }
    dest_fd = open(dest_path, flags, 0644);
    if (dest_fd == -1) {
        perror("open destination");
        close(src_fd);
        return -1;
    }

    // 复制文件内容
    while ((n = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dest_fd, buffer, n) != n) {
            perror("write");
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }

    // 检查读取错误
    if (n == -1) {
        perror("read");
        close(src_fd);
        close(dest_fd);
        return -1;
    }

    // 关闭文件
    close(src_fd);
    close(dest_fd);

    // 保留文件属性
    if (opt_p || opt_a) {
        if (chmod(dest_path, src_stat.st_mode) == -1) {
            perror("chmod");
        }
        if (chown(dest_path, src_stat.st_uid, src_stat.st_gid) == -1) {
            // 可能没有权限，忽略错误
        }
        if (utime(dest_path, NULL) == -1) {
            perror("utime");
        }
    }

    if (opt_v) {
        printf("%s -> %s\n", src_path, dest_path);
    }

    return 0;
}

// 递归复制目录
int copy_directory(const char *src_dir, const char *dest_dir) {
    DIR *dir;
    struct dirent *entry;
    char src_path[1024], dest_path[1024];
    struct stat st;

    // 打开源目录
    dir = opendir(src_dir);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    // 创建目标目录
    if (!file_exists(dest_dir)) {
        if (mkdir(dest_dir, 0755) == -1) {
            perror("mkdir");
            closedir(dir);
            return -1;
        }
    }

    // 读取目录内容
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 构建源路径和目标路径
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        // 获取文件状态
        if (stat(src_path, &st) == -1) {
            perror("stat");
            continue;
        }

        // 根据文件类型处理
        if (S_ISDIR(st.st_mode)) {
            // 递归复制子目录
            if (copy_directory(src_path, dest_path) == -1) {
                closedir(dir);
                return -1;
            }
        } else {
            // 复制文件
            if (copy_file(src_path, dest_path) == -1) {
                closedir(dir);
                return -1;
            }
        }
    }

    // 关闭目录
    closedir(dir);

    if (opt_v) {
        printf("%s/ -> %s/\n", src_dir, dest_dir);
    }

    return 0;
}

void cp_command(int argc, char *argv[]) {
    int i, j;
    char *src_path, *dest_path;
    int src_count = 0;
    char **src_paths = NULL;

    // 重置选项标志
    opt_r = 0;
    opt_i = 0;
    opt_f = 0;
    opt_v = 0;
    opt_a = 0;
    opt_p = 0;
    opt_u = 0;
    opt_l = 0;
    opt_s = 0;

    // 解析选项
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // 处理选项
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'r':
                    case 'R':
                        opt_r = 1;
                        break;
                    case 'i':
                        opt_i = 1;
                        break;
                    case 'f':
                        opt_f = 1;
                        break;
                    case 'v':
                        opt_v = 1;
                        break;
                    case 'a':
                        opt_a = 1;
                        opt_p = 1;
                        opt_r = 1;
                        break;
                    case 'p':
                        opt_p = 1;
                        break;
                    case 'u':
                        opt_u = 1;
                        break;
                    case 'l':
                        opt_l = 1;
                        break;
                    case 's':
                        opt_s = 1;
                        break;
                    default:
                        printf("cp: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: cp [OPTION]... SOURCE DEST\n");
                        return;
                }
            }
        } else {
            // 处理路径参数
            src_count++;
            src_paths = realloc(src_paths, src_count * sizeof(char *));
            src_paths[src_count - 1] = argv[i];
        }
    }

    // 检查参数数量
    if (src_count < 2) {
        printf("Usage: cp [OPTION]... SOURCE DEST\n");
        free(src_paths);
        return;
    }

    // 最后一个参数是目标
    dest_path = src_paths[src_count - 1];

    // 处理多个源文件的情况
    if (src_count > 2) {
        // 检查目标是否是目录
        if (!is_directory(dest_path)) {
            printf("cp: target '%s' is not a directory\n", dest_path);
            free(src_paths);
            return;
        }

        // 复制每个源文件到目标目录
        for (i = 0; i < src_count - 1; i++) {
            char dest_file[1024];
            snprintf(dest_file, sizeof(dest_file), "%s/%s", dest_path, basename(src_paths[i]));

            if (is_directory(src_paths[i])) {
                if (!opt_r) {
                    printf("cp: omitting directory '%s'\n", src_paths[i]);
                    continue;
                }
                if (copy_directory(src_paths[i], dest_file) == -1) {
                    free(src_paths);
                    return;
                }
            } else {
                if (copy_file(src_paths[i], dest_file) == -1) {
                    free(src_paths);
                    return;
                }
            }
        }
    } else {
        // 单个源文件的情况
        src_path = src_paths[0];

        if (is_directory(src_path)) {
            if (!opt_r) {
                printf("cp: omitting directory '%s'\n", src_path);
                free(src_paths);
                return;
            }

            // 检查目标是否存在
            if (file_exists(dest_path)) {
                // 目标存在，检查是否是目录
                if (is_directory(dest_path)) {
                    // 目标是目录，复制到 dest_dir/src_dir
                    char dest_subdir[1024];
                    snprintf(dest_subdir, sizeof(dest_subdir), "%s/%s", dest_path, basename(src_path));
                    if (copy_directory(src_path, dest_subdir) == -1) {
                        free(src_paths);
                        return;
                    }
                } else {
                    // 目标是文件，不能复制目录到文件
                    printf("cp: cannot copy directory '%s' to non-directory '%s'\n", src_path, dest_path);
                    free(src_paths);
                    return;
                }
            } else {
                // 目标不存在，创建目录并复制
                if (copy_directory(src_path, dest_path) == -1) {
                    free(src_paths);
                    return;
                }
            }
        } else {
            // 源是文件
            if (is_directory(dest_path)) {
                // 目标是目录，复制到 dest_dir/src_file
                char dest_file[1024];
                snprintf(dest_file, sizeof(dest_file), "%s/%s", dest_path, basename(src_path));
                if (copy_file(src_path, dest_file) == -1) {
                    free(src_paths);
                    return;
                }
            } else {
                // 目标是文件，直接复制
                if (copy_file(src_path, dest_path) == -1) {
                    free(src_paths);
                    return;
                }
            }
        }
    }

    free(src_paths);
}
