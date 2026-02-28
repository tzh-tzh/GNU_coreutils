#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void mkdir_command(int argc, char *argv[]) {
    int i, j;
    int p_flag = 0;  // 递归创建
    int v_flag = 0;  // 显示过程
    char *m_mode = NULL;  // 权限模式

    // 解析选项
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'p':
                        p_flag = 1;
                        break;
                    case 'v':
                        v_flag = 1;
                        break;
                    case 'm':
                        if (argv[i][j + 1] != '\0') {
                            m_mode = &argv[i][j + 1];
                            j = strlen(argv[i]) - 1;
                        } else if (i + 1 < argc) {
                            m_mode = argv[++i];
                        } else {
                            printf("mkdir: option requires an argument -- 'm'\n");
                            return;
                        }
                        break;
                    default:
                        printf("mkdir: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: mkdir [-pv] [-m mode] directory...\n");
                        return;
                }
            }
        } else {
            break;
        }
    }

    // 检查是否指定了目录名
    if (i >= argc) {
        printf("Usage: mkdir [-pv] [-m mode] directory...\n");
        return;
    }

    // 计算权限模式
    mode_t mode = 0755;
    if (m_mode != NULL) {
        mode = strtol(m_mode, NULL, 8);
    }

    // 创建每个指定的目录
    for (; i < argc; i++) {
        if (p_flag) {
            // 递归创建目录
            char *path = strdup(argv[i]);
            char *p = path;
            
            // 跳过根目录
            while (*p == '/') p++;
            
            while ((p = strchr(p, '/')) != NULL) {
                *p = '\0';
                if (mkdir(path, mode) == -1 && errno != EEXIST) {
                    perror(path);
                }
                *p = '/';
                p++;
            }
            
            // 创建最后一个目录
            if (mkdir(path, mode) == -1) {
                if (errno != EEXIST) {
                    perror(path);
                }
            } else {
                if (v_flag) {
                    printf("mkdir: created directory '%s'\n", argv[i]);
                }
            }
            free(path);
        } else {
            // 普通创建
            if (mkdir(argv[i], mode) == -1) {
                perror(argv[i]);
            } else {
                if (v_flag) {
                    printf("mkdir: created directory '%s'\n", argv[i]);
                }
            }
        }
    }
}
