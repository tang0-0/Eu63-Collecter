CC = gcc 
LD = gcc

# -I指定头文件目录
INCLUDE     =-I../Src
# LIB_NAMES =
# LIB_PATH 	=
# 开启编译warning和设置优化等级
CFLAGS      = -Wall -O2

# 正则表达式表示目录下所有.c文件，相当于：SRCS = main.c a.c b.c
SRCS = $(wildcard *.c ../Src/*.c)
# OBJS表示SRCS中把列表中的.c全部替换为.o，相当于：OBJS = main.o a.o b.o
OBJS = $(patsubst %.c, %.o, $(SRCS))

# 可执行文件的名字
TARGET = eu63-collecter

# .PHONE伪目标
.PHONY:all clean

# 要生成的目标文件
all: $(TARGET)

# 第一行依赖关系：冒号后面为依赖的文件，相当于Hello: main.o a.o b.o
# 第二行规则：$@表示目标文件，$^表示所有依赖文件，$<表示第一个依赖文件
$(TARGET): $(OBJS)
	$(LD) -o $@ $^

# 上一句目标文件依赖一大堆.o文件，这句表示所有.o都由相应名字的.c文件自动生成
%.o:%.c
	$(CC) -c $< -o $@

# make clean删除所有.o和目标文件
clean:
	rm -f $(OBJS) $(TARGET)

