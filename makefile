CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = scheduler
SRC = scheduler.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean