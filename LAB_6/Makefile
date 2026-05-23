CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC_DIR = src
BUILD_DIR = build/debug

all: $(BUILD_DIR)/gen $(BUILD_DIR)/view $(BUILD_DIR)/sort_index

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/gen: $(SRC_DIR)/gen.c $(SRC_DIR)/index.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/gen.c

$(BUILD_DIR)/view: $(SRC_DIR)/view.c $(SRC_DIR)/index.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/view.c

$(BUILD_DIR)/sort_index: $(SRC_DIR)/sort_index.c $(SRC_DIR)/index.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/sort_index.c

clean:
	rm -f $(BUILD_DIR)/gen $(BUILD_DIR)/view $(BUILD_DIR)/sort_index