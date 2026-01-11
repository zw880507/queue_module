# ===== 基本配置 =====
TARGET := main
CC     ?= gcc

# 自动找出所有 .c
SRCS := \
	buffer_item_ops.c \
	group_item_ops.c \
	main.c \
	process_module.c \
	queue.c \
	source_module.c \
	sync_module.c \
	sync_policy_window.c \
        item_track_ops.c \
        item_track.c

OBJS := $(SRCS:.c=.o)

# ===== 平台判断 =====
UNAME_S := $(shell uname -s)

CFLAGS  := -Wall -Wextra -g -I.
LDFLAGS := -lpthread

ifeq ($(UNAME_S),Linux)
    CFLAGS += -D_GNU_SOURCE -DOS_LINUX
endif

ifeq ($(UNAME_S),QNX)
    CFLAGS += -DOS_QNX
endif

# ===== 规则 =====
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

