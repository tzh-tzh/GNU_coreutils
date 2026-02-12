#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

// 选项结构体
typedef struct {
    int a;  // 仅更新访问时间（atime）
    int m;  // 仅更新修改时间（mtime）
    int c;  // 如果文件不存在，不创建新文件
    char *r; // 使用参考文件的时间戳
    char *t; // 指定具体时间
    char *d; // 使用自然语言指定时间
} touch_options;

// 函数声明
int file_exists(const char *path);

// 使用参考文件的时间戳
int use_reference_file(const char *ref_path, struct utimbuf *times) {
    struct stat st;
    if (stat(ref_path, &st) == -1) {
        perror(ref_path);
        return -1;
    }
    times->actime = st.st_atime;
    times->modtime = st.st_mtime;
    return 0;
}

// 使用指定的时间字符串
int use_time_string(const char *time_str, struct utimbuf *times) {
    struct tm tm;
    time_t t;
    int year, month, day, hour, minute, second = 0;
    int count;
    
    // 尝试解析不同格式的时间字符串
    count = sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    if (count >= 3) {
        // 设置时间结构体
        time(&t);
        struct tm *tm_ptr = localtime(&t);
        if (tm_ptr != NULL) {
            tm = *tm_ptr;
        }
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        if (count >= 5) {
            tm.tm_hour = hour;
            tm.tm_min = minute;
            if (count >= 6) {
                tm.tm_sec = second;
            } else {
                tm.tm_sec = 0;
            }
        } else {
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
        }
        
        t = mktime(&tm);
        if (t != -1) {
            times->actime = t;
            times->modtime = t;
            return 0;
        }
    }
    
    printf("touch: invalid date format '%s'\n", time_str);
    return -1;
}

// 使用指定的时间格式 [[CC]YY]MMDDhhmm[.ss]
int use_time_format(const char *time_format, struct utimbuf *times) {
    struct tm tm;
    time_t t;
    int year, month, day, hour, minute, second = 0;
    int len = strlen(time_format);
    
    // 解析时间格式
    if (len == 12) {
        // YYMMDDhhmm
        sscanf(time_format, "%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute);
        year += 2000;
    } else if (len == 14) {
        // CCYYMMDDhhmm
        sscanf(time_format, "%4d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute);
    } else if (len == 13) {
        // YYMMDDhhmm.ss
        sscanf(time_format, "%2d%2d%2d%2d%2d.%2d", &year, &month, &day, &hour, &minute, &second);
        year += 2000;
    } else if (len == 15) {
        // CCYYMMDDhhmm.ss
        sscanf(time_format, "%4d%2d%2d%2d%2d.%2d", &year, &month, &day, &hour, &minute, &second);
    } else {
        printf("touch: invalid time format '%s'\n", time_format);
        return -1;
    }
    
    // 设置时间结构体
    time(&t);
    struct tm *tm_ptr = localtime(&t);
    if (tm_ptr != NULL) {
        tm = *tm_ptr;
    }
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    
    t = mktime(&tm);
    if (t == -1) {
        printf("touch: invalid time value\n");
        return -1;
    }
    
    times->actime = t;
    times->modtime = t;
    return 0;
}

/**
 * 执行touch命令，更新文件的访问时间和修改时间
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void touch_command(int argc, char *argv[]) {
    int i, j;
    touch_options opts = {0}; // 初始化选项结构体
    struct utimbuf times;
    time_t current_time;
    
    // 获取当前时间
    time(&current_time);
    times.actime = current_time;
    times.modtime = current_time;
    
    // 解析命令行选项和参数
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // 处理选项参数
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'a':
                        opts.a = 1;
                        break;
                    case 'm':
                        opts.m = 1;
                        break;
                    case 'c':
                        opts.c = 1;
                        break;
                    case 'r':
                        if (i + 1 < argc) {
                            opts.r = argv[++i];
                        } else {
                            printf("touch: option requires an argument -- 'r'\n");
                            return;
                        }
                        break;
                    case 't':
                        if (i + 1 < argc) {
                            opts.t = argv[++i];
                        } else {
                            printf("touch: option requires an argument -- 't'\n");
                            return;
                        }
                        break;
                    case 'd':
                        if (i + 1 < argc) {
                            opts.d = argv[++i];
                        } else {
                            printf("touch: option requires an argument -- 'd'\n");
                            return;
                        }
                        break;
                    default:
                        printf("touch: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: touch [OPTION]... FILE...\n");
                        return;
                }
            }
        } else {
            // 处理文件参数
            const char *file = argv[i];
            
            // 检查文件是否存在
            if (!file_exists(file)) {
                if (opts.c) {
                    // 如果文件不存在且指定了 -c 选项，跳过
                    continue;
                }
                
                // 创建新文件
                FILE *fp = fopen(file, "w");
                if (fp == NULL) {
                    perror(file);
                    continue;
                }
                fclose(fp);
            }
            
            // 处理时间选项
            struct utimbuf file_times = times;
            
            if (opts.r) {
                // 使用参考文件的时间戳
                if (use_reference_file(opts.r, &file_times) == -1) {
                    continue;
                }
            } else if (opts.d) {
                // 使用自然语言指定时间
                if (use_time_string(opts.d, &file_times) == -1) {
                    continue;
                }
            } else if (opts.t) {
                // 使用指定的时间格式
                if (use_time_format(opts.t, &file_times) == -1) {
                    continue;
                }
            }
            
            // 更新文件时间戳
            if (utime(file, &file_times) == -1) {
                perror(file);
            }
        }
    }
}
