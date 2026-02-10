# 简单 GNU Coreutils 实现

这是一个简单的 GNU Coreutils 基本命令实现，包括 `ls`、`cp`、`cat`、`echo`、`mkdir` 和 `rmdir`。

## 功能

- **ls**: 列出目录内容
- **cp**: 复制文件
- **cat**: 显示文件内容
- **echo**: 输出文本
- **mkdir**: 创建目录
- **rmdir**: 删除目录
- **交互式控制台**: 支持循环执行多个命令

## 编译

### 在 Ubuntu 24.04 上

1. **使用 Makefile**:
   ```bash
   make
   ```

2. **直接使用 gcc**:
   ```bash
   gcc -Wall -Wextra -std=c99 -o coreutils main.c ls.c cp.c cat.c echo.c mkdir.c rmdir.c
   ```

## 使用方法

### 交互式控制台模式

这是默认模式，启动后会进入交互式命令行界面：

```bash
./coreutils
```

#### 控制台操作
- **命令提示符**: `> `
- **执行命令**: 直接输入命令，如 `ls`、`echo Hello`
- **退出控制台**: 输入 `exit` 或 `quit`

#### 示例会话
```
Simple GNU Coreutils Interactive Console
Type 'exit' or 'quit' to exit
Available commands: ls, cp, cat, echo, mkdir, rmdir

> ls
main.c
ls.c
cp.c
cat.c
echo.c
mkdir.c
rmdir.c
Makefile
README.md

> echo Hello World
Hello World

> mkdir testdir

> ls
main.c
ls.c
cp.c
cat.c
echo.c
mkdir.c
rmdir.c
Makefile
README.md
testdir

> rmdir testdir

> ls
main.c
ls.c
cp.c
cat.c
echo.c
mkdir.c
rmdir.c
Makefile
README.md

> exit
Exiting console...
```

### 单个命令模式

也可以直接在命令行中指定要执行的命令：

```bash
./coreutils <命令> [参数]
```

### 可用命令

#### ls - 列出目录内容
```bash
ls [路径]
```
- **路径**: 可选的目录路径（默认：当前目录）
- 示例: `ls /home`

#### cp - 复制文件
```bash
cp <源文件> <目标文件>
```
- **源文件**: 源文件路径
- **目标文件**: 目标文件路径
- 示例: `cp file1.txt file2.txt`

#### cat - 显示文件内容
```bash
cat [文件1] [文件2] ...
```
- **文件**: 可选的文件路径（如果未提供，从标准输入读取）
- 示例: `cat file.txt`

#### echo - 输出文本
```bash
echo [文本]
```
- **文本**: 要输出的文本
- 示例: `echo Hello World`

#### mkdir - 创建目录
```bash
mkdir <目录1> [目录2] ...
```
- **目录**: 要创建的目录路径
- 示例: `mkdir dir1 dir2`

#### rmdir - 删除目录
```bash
rmdir <目录1> [目录2] ...
```
- **目录**: 要删除的目录路径（目录必须为空）
- 示例: `rmdir dir1 dir2`

## 文件

- **main.c**: 主程序，实现交互式命令行界面和命令分发
- **ls.c**: `ls` 命令的实现
- **cp.c**: `cp` 命令的实现
- **cat.c**: `cat` 命令的实现
- **echo.c**: `echo` 命令的实现
- **mkdir.c**: `mkdir` 命令的实现
- **rmdir.c**: `rmdir` 命令的实现
- **Makefile**: 编译规则
- **README.md**: 使用说明

## 注意事项

- 这是一个简化的实现，可能不支持原始 GNU Coreutils 的所有功能
- **rmdir** 命令只能删除空目录
- 设计用于教育目的和基本使用
- 在 Ubuntu 24.04 上测试通过
