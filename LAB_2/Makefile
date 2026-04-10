CC = gcc
CFLAGS = -W -Wall -Wextra -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable

BUILD_DIR = build/debug
SRC_DIR = src

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TARGETS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%,$(SOURCES))

all: $(TARGETS) copy-env

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o
	$(CC) $< -o $@

copy-env: $(BUILD_DIR)
	cp env $(BUILD_DIR)/

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean