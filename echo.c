#include <stdio.h>

void echo_command(int argc, char *argv[]) {
    int i;

    // 输出每个参数，参数之间用空格分隔
    for (i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }

    // 输出换行符
    printf("\n");
}
