CC = gcc
CFLAGS = -Wall -pthread -g
LDFLAGS = -pthread -lrt
TARGET = lab51
SRCDIR = src
BUILDDIR = build/debug

SOURCES = $(SRCDIR)/parent.c \
          $(SRCDIR)/queue.c \
          $(SRCDIR)/producer.c \
          $(SRCDIR)/consumer.c

OBJECTS = $(addprefix $(BUILDDIR)/, $(notdir $(SOURCES:.c=.o)))

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(BUILDDIR)/$@

clean:
	rm -rf $(BUILDDIR)

run: $(TARGET)
	./$(BUILDDIR)/$(TARGET)

.PHONY: all clean run