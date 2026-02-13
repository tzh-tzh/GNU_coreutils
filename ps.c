#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

// 格式化时间为HH:MM:SS格式
void format_time(unsigned long seconds, char *buffer, size_t buffer_size) {
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;
    snprintf(buffer, buffer_size, "%02lu:%02lu:%02lu", hours, minutes, secs);
}

// 格式化启动时间
void format_start_time(time_t start_time, char *buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm *tm_info;
    
    // 如果启动时间在今天，只显示时间
    // 否则显示日期
    tm_info = localtime(&now);
    int today_year = tm_info->tm_year;
    int today_month = tm_info->tm_mon;
    int today_day = tm_info->tm_mday;
    
    tm_info = localtime(&start_time);
    if (tm_info->tm_year == today_year && tm_info->tm_mon == today_month && tm_info->tm_mday == today_day) {
        // 今天启动的，显示时间
        strftime(buffer, buffer_size, "%H:%M", tm_info);
    } else {
        // 不是今天启动的，显示日期
        strftime(buffer, buffer_size, "%m-%d", tm_info);
    }
}

// 选项结构体
typedef struct {
    int a;  // 显示所有终端进程
    int u;  // 以用户友好格式显示
    int x;  // 显示所有进程，包括无终端的
    int e;  // 显示所有进程
    int f;  // 以全格式显示
} ps_options;

// 进程信息结构体
typedef struct {
    pid_t pid;         // 进程 ID
    pid_t ppid;        // 父进程 ID
    uid_t uid;         // 用户 ID
    gid_t gid;         // 组 ID
    char *user;        // 用户名
    char *group;       // 组名
    char *command;     // 命令名
    char *state;       // 进程状态
    unsigned long vsize; // 虚拟内存大小
    long rss;          // 常驻集大小
    int cpu_percent;   // CPU 使用率
    int mem_percent;   // 内存使用率
    time_t start_time; // 启动时间
    unsigned long utime; // 用户 CPU 时间
    unsigned long stime; // 系统 CPU 时间
    char *tty;         // 终端设备
} process_info;

// 读取进程信息
process_info *read_process_info(pid_t pid) {
    char path[PATH_MAX];
    char buffer[4096];
    FILE *file;
    process_info *info = NULL;

    // 分配内存
    info = (process_info *)malloc(sizeof(process_info));
    if (info == NULL) {
        return NULL;
    }

    // 初始化
    info->pid = pid;
    info->ppid = 0;
    info->uid = 0;
    info->gid = 0;
    info->user = NULL;
    info->group = NULL;
    info->command = NULL;
    info->state = NULL;
    info->vsize = 0;
    info->rss = 0;
    info->cpu_percent = 0;
    info->mem_percent = 0;
    info->start_time = 0;
    info->utime = 0;
    info->stime = 0;
    info->tty = NULL;

    // 读取状态文件
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (file != NULL) {
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            if (strncmp(buffer, "PPid:", 5) == 0) {
                sscanf(buffer + 5, "%d", &info->ppid);
            } else if (strncmp(buffer, "Uid:", 4) == 0) {
                sscanf(buffer + 4, "%d", &info->uid);
            } else if (strncmp(buffer, "Gid:", 4) == 0) {
                sscanf(buffer + 4, "%d", &info->gid);
            } else if (strncmp(buffer, "Name:", 5) == 0) {
                char name[256];
                sscanf(buffer + 5, "%s", name);
                info->command = strdup(name);
            } else if (strncmp(buffer, "State:", 6) == 0) {
                char state[16];
                sscanf(buffer + 6, "%s", state);
                info->state = strdup(state);
            } else if (strncmp(buffer, "VmSize:", 7) == 0) {
                sscanf(buffer + 7, "%lu", &info->vsize);
            } else if (strncmp(buffer, "VmRSS:", 6) == 0) {
                sscanf(buffer + 6, "%ld", &info->rss);
            }
        }
        fclose(file);
    }

    // 读取命令行文件
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    file = fopen(path, "r");
    if (file != NULL) {
        if (fgets(buffer, sizeof(buffer), file) != NULL) {
            // 替换空字符为空格
            for (int i = 0; buffer[i] != '\0'; i++) {
                if (buffer[i] == '\0') {
                    buffer[i] = ' ';
                }
            }
            // 移除末尾的空格
            int len = strlen(buffer);
            while (len > 0 && buffer[len - 1] == ' ') {
                buffer[len - 1] = '\0';
                len--;
            }
            if (len > 0) {
                if (info->command != NULL) {
                    free(info->command);
                }
                info->command = strdup(buffer);
            }
        }
        fclose(file);
    }

    // 读取启动时间、CPU时间和TTY
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (file != NULL) {
        if (fgets(buffer, sizeof(buffer), file) != NULL) {
            int dummy;
            char dummy_str[256];
            unsigned long utime, stime, start_time;
            int tty_nr;
            // 读取需要的字段
            // 格式: pid comm state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime cutime cstime priority nice num_threads itrealvalue starttime
            sscanf(buffer, "%d %s %s %d %*d %*d %d %*d %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %lu", 
                   &dummy, dummy_str, dummy_str, &info->ppid, &tty_nr, &utime, &stime, &start_time);
            info->utime = utime;
            info->stime = stime;
            // 转换启动时间为秒
            info->start_time = start_time / sysconf(_SC_CLK_TCK);
            
            // 处理TTY信息
            if (tty_nr == 0) {
                info->tty = strdup("?");
            } else {
                // 转换tty_nr为终端设备名
                char tty_buffer[32];
                int major = tty_nr >> 8;
                int minor = tty_nr & 0xff;
                
                if (major == 4) {
                    // tty设备
                    snprintf(tty_buffer, sizeof(tty_buffer), "tty%d", minor);
                } else if (major == 136) {
                    // pts设备
                    snprintf(tty_buffer, sizeof(tty_buffer), "pts/%d", minor);
                } else {
                    snprintf(tty_buffer, sizeof(tty_buffer), "?");
                }
                info->tty = strdup(tty_buffer);
            }
        }
        fclose(file);
    }

    // 获取用户名和组名
    struct passwd *pwd = getpwuid(info->uid);
    if (pwd != NULL) {
        info->user = strdup(pwd->pw_name);
    } else {
        char uid_str[16];
        sprintf(uid_str, "%d", info->uid);
        info->user = strdup(uid_str);
    }

    struct group *grp = getgrgid(info->gid);
    if (grp != NULL) {
        info->group = strdup(grp->gr_name);
    } else {
        char gid_str[16];
        sprintf(gid_str, "%d", info->gid);
        info->group = strdup(gid_str);
    }

    return info;
}

// 释放进程信息
void free_process_info(process_info *info) {
    if (info == NULL) {
        return;
    }

    if (info->user != NULL) {
        free(info->user);
    }
    if (info->group != NULL) {
        free(info->group);
    }
    if (info->command != NULL) {
        free(info->command);
    }
    if (info->state != NULL) {
        free(info->state);
    }
    if (info->tty != NULL) {
        free(info->tty);
    }

    free(info);
}

// 显示进程信息（BSD 风格：ps aux）
void display_processes_bsd_style(ps_options *opts) {
    DIR *dir;
    struct dirent *entry;
    process_info *info;

    // 处理未使用的参数
    (void)opts;

    // 打印表头
    printf("USER       PID  CPU MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");

    // 打开 /proc 目录
    dir = opendir("/proc");
    if (dir == NULL) {
        perror("ps");
        return;
    }

    // 遍历 /proc 目录
    while ((entry = readdir(dir)) != NULL) {
        // 检查是否是数字目录（进程 ID）
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
            pid_t pid = atoi(entry->d_name);

            // 读取进程信息
            info = read_process_info(pid);
            if (info != NULL) {
                // 格式化时间
                char start_time_str[16];
                char cpu_time_str[16];
                format_start_time(info->start_time, start_time_str, sizeof(start_time_str));
                format_time((info->utime + info->stime) / sysconf(_SC_CLK_TCK), cpu_time_str, sizeof(cpu_time_str));
                
                // 显示进程信息
                printf("%-8s %5d %4d %4d %6lu %6ld %-8s %-4s %s %s %s\n",
                       info->user, info->pid, info->cpu_percent, info->mem_percent,
                       info->vsize, info->rss, info->tty, info->state, start_time_str, cpu_time_str, info->command);

                // 释放进程信息
                free_process_info(info);
            }
        }
    }

    // 关闭目录
    closedir(dir);
}

// 显示进程信息（POSIX/Unix 风格：ps -ef）
void display_processes_posix_style(ps_options *opts) {
    DIR *dir;
    struct dirent *entry;
    process_info *info;

    // 处理未使用的参数
    (void)opts;

    // 打印表头
    printf("UID        PID  PPID  C STIME TTY          TIME CMD\n");

    // 打开 /proc 目录
    dir = opendir("/proc");
    if (dir == NULL) {
        perror("ps");
        return;
    }

    // 遍历 /proc 目录
    while ((entry = readdir(dir)) != NULL) {
        // 检查是否是数字目录（进程 ID）
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
            pid_t pid = atoi(entry->d_name);

            // 读取进程信息
            info = read_process_info(pid);
            if (info != NULL) {
                // 格式化时间
                char start_time_str[16];
                char cpu_time_str[16];
                format_start_time(info->start_time, start_time_str, sizeof(start_time_str));
                format_time((info->utime + info->stime) / sysconf(_SC_CLK_TCK), cpu_time_str, sizeof(cpu_time_str));
                
                // 显示进程信息
                printf("%-8s %5d %5d %2d %s %-8s %s %s\n",
                       info->user, info->pid, info->ppid, info->cpu_percent, start_time_str, info->tty, cpu_time_str, info->command);

                // 释放进程信息
                free_process_info(info);
            }
        }
    }

    // 关闭目录
    closedir(dir);
}

/**
 * 执行ps命令，显示当前系统中运行的进程快照
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void ps_command(int argc, char *argv[]) {
    int i, j;
    ps_options opts = {0}; // 初始化选项结构体
    int is_bsd_style = 0; // 是否为 BSD 风格

    // 解析命令行选项
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // POSIX/Unix 风格选项
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'e':
                        opts.e = 1;
                        break;
                    case 'f':
                        opts.f = 1;
                        break;
                    default:
                        printf("ps: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: ps [OPTION]...\n");
                        return;
                }
            }
        } else {
            // BSD 风格选项
            is_bsd_style = 1;
            for (j = 0; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'a':
                        opts.a = 1;
                        break;
                    case 'u':
                        opts.u = 1;
                        break;
                    case 'x':
                        opts.x = 1;
                        break;
                    default:
                        printf("ps: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: ps [OPTION]...\n");
                        return;
                }
            }
        }
    }

    // 根据选项风格显示进程信息
    if (is_bsd_style || opts.a || opts.u || opts.x) {
        // BSD 风格
        display_processes_bsd_style(&opts);
    } else {
        // POSIX/Unix 风格
        display_processes_posix_style(&opts);
    }
}
