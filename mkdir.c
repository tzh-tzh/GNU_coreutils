#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

void mkdir_command(int argc, char *argv[]) {
    int i;

    // 检查参数数量
    if (argc < 2) {
        printf("Usage: mkdir <directory> [directories]\n");
        return;
    }

    // 创建每个指定的目录
    for (i = 1; i < argc; i++) {
#ifdef _WIN32
        // 在 Windows 上，mkdir 只接受一个参数
        if (mkdir(argv[i]) == -1) {
#else
        // 在 Linux/Unix 上，mkdir 需要两个参数：路径和权限模式
        if (mkdir(argv[i], 0755) == -1) {
#endif
            perror(argv[i]);
        }
    }
}
