# 像素花园 —— 不想装 CMake 时的备用构建方式
# 用法：make        编译
#       make run    编译并运行
#       make clean  清理

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra
TARGET   = plant_retry
SRC      = main.c pgsm.c ui.c save.c
OBJ      = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c pgsm.h ui.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET) $(TARGET).exe

.PHONY: all run clean
