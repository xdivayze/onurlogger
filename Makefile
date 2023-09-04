CC = gcc
CFLAGS = -O3 -Wall --std=c99
LDFLAGS = -lX11 -lXi
SRC = main.c
TARGET = main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean: 
	rm -f $(TARGET)

.PHONY: all clean
