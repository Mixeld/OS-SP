CC = gcc
CFLAGS = -Wall -Wextra -g -I$(SRCDIR)
TARGET = parent
SRCDIR = src
BUILDDIR = build

SOURCES = $(SRCDIR)/parent.c $(SRCDIR)/semaphore.c $(SRCDIR)/queue.c \
          $(SRCDIR)/hash.c $(SRCDIR)/producer.c $(SRCDIR)/consumer.c
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/common.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run