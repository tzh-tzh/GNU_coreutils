#include <stdio.h>
#include <stdlib.h>

void cp_command(int argc, char *argv[]) {
    FILE *src, *dest;
    char buffer[1024];
    size_t n;

    // 检查参数数量
    if (argc != 3) {
        printf("Usage: cp <source> <destination>\n");
        return;
    }

    // 打开源文件
    src = fopen(argv[1], "rb");
    if (src == NULL) {
        perror("fopen source");
        return;
    }

    // 打开目标文件
    dest = fopen(argv[2], "wb");
    if (dest == NULL) {
        perror("fopen destination");
        fclose(src);
        return;
    }

    // 复制文件内容
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, n, dest) != n) {
            perror("fwrite");
            fclose(src);
            fclose(dest);
            return;
        }
    }

    // 检查读取错误
    if (ferror(src)) {
        perror("fread");
    }

    // 关闭文件
    fclose(src);
    fclose(dest);
}
