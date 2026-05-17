CC = gcc
CFLAGS = -Wall -Wextra -g -I$(SRCDIR)
TARGET = lab4
SRCDIR = src
BUILDDIR = build/debug

SOURCES = $(SRCDIR)/parent.c $(SRCDIR)/ipc_utils.c \
          $(SRCDIR)/producer.c $(SRCDIR)/consumer.c
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

all: $(BUILDDIR) $(BUILDDIR)/$(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/common.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

run: $(BUILDDIR)/$(TARGET)
	./$(BUILDDIR)/$(TARGET)

.PHONY: all clean run