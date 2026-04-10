CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
TARGET = dirwalk
SRCS = src/dirwalker.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean