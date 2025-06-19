CC = gcc
CFLAGS = -static -g -Wall
TARGET = usdSh

all: $(TARGET)

$(TARGET): ucuShell.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
