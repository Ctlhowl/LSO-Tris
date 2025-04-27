CC = gcc
CFLAGS = -Wall -Wextra -std=c11 $(shell pkg-config --cflags jansson)
LDFLAGS = $(shell pkg-config --libs jansson) -lpthread
DEBUG_FLAGS = -g -O0
RELEASE_FLAGS = -O2

SRC_DIR = .
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

TARGET = tic_tac_toe_server

all: debug

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

rebuild: clean all

.PHONY: all debug release clean rebuild