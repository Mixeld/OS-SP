CC = gcc
CFLAGS = -std=c11 -g2 -ggdb -pedantic -W -Wall -Wextra
.SUFFIXES:
.SUFFIXES: .c .o

DEBUG = ./build/debug
RELEASE = ./build/release

vpath %.c src
vpath %.h src

ifeq ($(MODE), release)
    CFLAGS = -std=c11 -pedantic -W -Wall -Wextra -Werror
    OUT_DIR = $(RELEASE)
    vpath %.o $(RELEASE)
else
    OUT_DIR = $(DEBUG)
    vpath %.o $(DEBUG)
endif

objects = $(OUT_DIR)/dirwalker.o
prog = $(OUT_DIR)/dirwalker

all: $(prog)

$(prog): $(objects)
	$(CC) $(CFLAGS) $(objects) -o $@

$(OUT_DIR)/%.o: %.c
	@mkdir -p $(OUT_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

run: $(prog)
	./$(prog) -h

clean:
	@rm -rf $(DEBUG)/* $(RELEASE)/* dirwalker

.PHONY: all clean run