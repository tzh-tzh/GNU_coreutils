#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 选项结构体
typedef struct {
    int n;  // 为所有行（包括空行）编号
    int b;  // 仅对非空行编号
    int s;  // 压缩连续空行（多个空行合并为一个）
    int E;  // 每行末尾显示 $
    int T;  // Tab 显示为 ^I
    int A;  // 显示所有隐藏字符（等价于 -vET）
} cat_options;

// 处理文件内容
void process_file(FILE *file, cat_options *opts) {
    int c;
    int line_num = 0;
    int empty_line_count = 0;
    int is_newline = 1;

    while ((c = fgetc(file)) != EOF) {
        // 处理新行开始
        if (is_newline) {
            // 处理空行
            if (c == '\n') {
                // 空行压缩处理：如果启用-s选项且当前为空行
                if (opts->s) {
                    empty_line_count++;
                    // 跳过多余的空行，只保留一个
                    if (empty_line_count > 1) {
                        is_newline = 1;
                        continue;
                    }
                }
                
                // 行号处理：如果启用-n选项，即使是空行也需要编号
                if (opts->n) {
                    printf("%6d  ", ++line_num);
                }
                
                // 行尾标记处理：如果启用-E或-A选项
                if (opts->E || opts->A) {
                    putchar('$');
                }
                
                putchar('\n');
                is_newline = 1;
                continue;
            }
            
            // 非空行处理
            // 行号处理：如果启用-n选项或启用-b选项
            if (opts->n || opts->b) {
                printf("%6d  ", ++line_num);
            }
            
            is_newline = 0;
            empty_line_count = 0;
        }
        
        // 处理非新行开始的情况
        if (c != '\n') {
            // Tab字符处理
            if (c == '\t') {
                // 可视化显示：如果启用-T或-A选项
                if (opts->T || opts->A) {
                    printf("^I");
                } else {
                    putchar('\t');
                }
                continue;
            }
            
            // 不可见字符处理（仅当启用-A选项时）
            if (opts->A) {
                // 处理控制字符（ASCII 0-31，除了换行符和制表符）
                if (c < 32 && c != '\n' && c != '\t') {
                    printf("^%c", c + 64);
                // 处理删除字符（ASCII 127）
                } else if (c == 127) {
                    printf("^?");
                } else {
                    putchar(c);
                }
                continue;
            }
            
            // 普通字符直接输出
            putchar(c);
        } else {
            // 处理换行符情况
            // 重置空行计数
            empty_line_count = 0;
            
            // 行尾标记处理：如果启用-E或-A选项
            if (opts->E || opts->A) {
                putchar('$');
            }
            
            putchar('\n');
            is_newline = 1;
        }
    }
}

void cat_command(int argc, char *argv[]) {
    int i, j;
    cat_options opts = {0}; // 初始化选项结构体
    FILE *file;

    // 解析命令行选项
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // 处理选项参数
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'n':
                        opts.n = 1;
                        break;
                    case 'b':
                        opts.b = 1;
                        break;
                    case 's':
                        opts.s = 1;
                        break;
                    case 'E':
                        opts.E = 1;
                        break;
                    case 'T':
                        opts.T = 1;
                        break;
                    case 'A':
                        opts.A = 1;
                        opts.E = 1;
                        opts.T = 1;
                        break;
                    default:
                        printf("cat: invalid option -- '%c'\n", argv[i][j]);
                        printf("Usage: cat [OPTION]... [FILE]...\n");
                        return;
                }
            }
        } else {
            // 处理文件参数
            file = fopen(argv[i], "rb");
            if (file == NULL) {
                perror(argv[i]);
                continue;
            }

            // 处理文件内容
            process_file(file, &opts);

            // 检查读取错误
            if (ferror(file)) {
                perror(argv[i]);
            }

            // 关闭文件
            fclose(file);
        }
    }

    // 如果没有提供文件参数，从标准输入读取
    if (argc == 1) {
        process_file(stdin, &opts);
    }
}
