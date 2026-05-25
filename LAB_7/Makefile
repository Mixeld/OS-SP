CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE
SRC_DIR = src
BUILD_DIR = build/debug

HEADERS = $(SRC_DIR)/record.h \
          $(SRC_DIR)/file_ops.h \
          $(SRC_DIR)/locks.h \
          $(SRC_DIR)/utils.h \
          $(SRC_DIR)/commands.h


OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/commands.o \
       $(BUILD_DIR)/file_ops.o \
       $(BUILD_DIR)/locks.o \
       $(BUILD_DIR)/utils.o

all: $(BUILD_DIR)/lab7

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


$(BUILD_DIR)/lab7: $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $@

$(BUILD_DIR)/commands.o: $(SRC_DIR)/commands.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/commands.c -o $@

$(BUILD_DIR)/file_ops.o: $(SRC_DIR)/file_ops.c $(SRC_DIR)/file_ops.h $(SRC_DIR)/record.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/file_ops.c -o $@

$(BUILD_DIR)/locks.o: $(SRC_DIR)/locks.c $(SRC_DIR)/locks.h $(SRC_DIR)/record.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/locks.c -o $@

$(BUILD_DIR)/utils.o: $(SRC_DIR)/utils.c $(SRC_DIR)/utils.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/utils.c -o $@

clean:
	rm -f $(OBJS) $(BUILD_DIR)/lab7