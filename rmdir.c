#include <stdio.h>
#include <unistd.h>

void rmdir_command(int argc, char *argv[]) {
    int i;

    // 检查参数数量
    if (argc < 2) {
        printf("Usage: rmdir <directory> [directories]\n");
        return;
    }

    // 删除每个指定的目录
    for (i = 1; i < argc; i++) {
        if (rmdir(argv[i]) == -1) {
            perror(argv[i]);
        }
    }
}
