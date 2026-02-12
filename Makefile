CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# 条件编译：根据操作系统设置不同的链接选项
ifeq ($(OS),Windows_NT)
LDFLAGS = -lmsvcrt  # Windows 环境
else
LDFLAGS =  # Linux/Ubuntu 环境
endif

TARGET = coreutils

SRCS = main.c ls.c cp.c cat.c echo.c mkdir.c rmdir.c rm.c touch.c pwd.c cd.c mv.c
OBJS = $(SRCS:.c=.o)

all: clean $(TARGET)
	# 编译完成后删除临时目标文件
	rm -f $(OBJS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
