#include <stdio.h>

void cat_command(int argc, char *argv[]) {
    FILE *file;
    int i;
    char buffer[1024];
    size_t n;

    // 如果没有提供文件参数，从标准输入读取
    if (argc == 1) {
        while ((n = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
            fwrite(buffer, 1, n, stdout);
        }
        return;
    }

    // 处理每个文件参数
    for (i = 1; i < argc; i++) {
        file = fopen(argv[i], "rb");
        if (file == NULL) {
            perror(argv[i]);
            continue;
        }

        // 读取并显示文件内容
        while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            fwrite(buffer, 1, n, stdout);
        }

        // 检查读取错误
        if (ferror(file)) {
            perror(argv[i]);
        }

        // 关闭文件
        fclose(file);
    }
}
