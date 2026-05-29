CC     = gcc
CFLAGS = -Wall -Wextra

SRC = src
OUT = build/debug

all: $(OUT)/server $(OUT)/client

$(OUT)/server: $(SRC)/server.c $(SRC)/protocol.c $(SRC)/protocol.h
	$(CC) $(CFLAGS) -o $(OUT)/server $(SRC)/server.c $(SRC)/protocol.c

$(OUT)/client: $(SRC)/client.c $(SRC)/protocol.c $(SRC)/protocol.h
	$(CC) $(CFLAGS) -o $(OUT)/client $(SRC)/client.c $(SRC)/protocol.c

clean:
	rm -f $(OUT)/server $(OUT)/client