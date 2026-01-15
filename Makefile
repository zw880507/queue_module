TARGET := main
CC     ?= gcc

SRCS := \
	buffer_item_ops.c \
	main.c \
	queue.c \
        item_track_ops.c \
        item_track.c \
        queue_module.c \
        queue_ops.c \
        process_ops.c

OBJS := $(SRCS:.c=.o)

UNAME_S := $(shell uname -s)

CFLAGS  := -Wall -Wextra -g -I.
LDFLAGS := -lpthread

ifeq ($(UNAME_S),Linux)
    CFLAGS += -D_GNU_SOURCE -DOS_LINUX
endif

ifeq ($(UNAME_S),QNX)
    CFLAGS += -DOS_QNX
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

