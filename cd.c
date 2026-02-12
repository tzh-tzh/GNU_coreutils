#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>

/**
 * 执行cd命令，切换当前工作目录
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 */
void cd_command(int argc, char *argv[]) {
    char *path = NULL;
    char cwd[PATH_MAX];
    char *oldpwd = getenv("OLDPWD");
    char *home = getenv("HOME");

    // 解析命令行参数
    if (argc > 1) {
        path = argv[1];
    }

    // 处理特殊情况
    if (path == NULL) {
        // 没有指定路径，切换到用户主目录
        path = home;
    } else if (strcmp(path, "~") == 0) {
        // 切换到用户主目录
        path = home;
    } else if (path[0] == '~' && path[1] == '/') {
        // 切换到用户主目录的子目录
        static char expanded_path[PATH_MAX];
        if (home != NULL) {
            sprintf(expanded_path, "%s%s", home, path + 1);
            path = expanded_path;
        }
    } else if (path[0] == '~' && path[1] != '/') {
        // 切换到指定用户的主目录 ~username
        struct passwd *pwd;
        char username[PATH_MAX];
        static char expanded_path[PATH_MAX];
        
        // 提取用户名
        char *slash = strchr(path, '/');
        if (slash != NULL) {
            strncpy(username, path + 1, slash - path - 1);
            username[slash - path - 1] = '\0';
        } else {
            strcpy(username, path + 1);
        }
        
        // 获取指定用户的密码信息
        pwd = getpwnam(username);
        if (pwd != NULL) {
            if (slash != NULL) {
                // 切换到指定用户主目录的子目录
                sprintf(expanded_path, "%s%s", pwd->pw_dir, slash);
            } else {
                // 切换到指定用户的主目录
                strcpy(expanded_path, pwd->pw_dir);
            }
            path = expanded_path;
        } else {
            printf("cd: %s: No such user\n", username);
            return;
        }
    } else if (strcmp(path, "..") == 0) {
        // 切换到上一级目录
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            char *last_slash = strrchr(cwd, '/');
            if (last_slash != NULL) {
                if (last_slash == cwd) {
                    // 已经在根目录
                    static char root_path[] = "/";
                    path = root_path;
                } else {
                    // 切换到上一级目录
                    *last_slash = '\0';
                    path = cwd;
                }
            }
        }
    } else if (strcmp(path, ".") == 0) {
        // 切换到当前目录，无意义，直接返回
        return;
    } else if (strcmp(path, "-") == 0) {
        // 切换到上一次的工作目录
        if (oldpwd != NULL) {
            path = oldpwd;
            // 显示切换的路径
            printf("%s\n", path);
        } else {
            printf("cd: OLDPWD not set\n");
            return;
        }
    }

    // 保存当前工作目录到 OLDPWD 环境变量
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("OLDPWD", cwd, 1);
    }

    // 切换到新的工作目录
    if (chdir(path) == -1) {
        perror("cd");
        return;
    }

    // 更新 PWD 环境变量
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("PWD", cwd, 1);
    }
}
