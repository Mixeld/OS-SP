CC = gcc
CFLAGS = -W -Wall -Wextra -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable

BUILD_DIR = build/debug
SRC_DIR = src

TARGET = lab3

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(BUILD_DIR)/parent.o $(BUILD_DIR)/child.o | $(BUILD_DIR)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/$(TARGET)

.PHONY: all clean run