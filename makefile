CC = gcc
CFLAGS = -Wall
LIBS = -lm -fopenmp
TARGET = par
SRC = parallel.c
serial_SRC = serial.c
serial_TARGET = serial

all: $(TARGET) $(serial_TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(serial_TARGET): $(serial_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(TARGET) $(serial_TARGET)

.PHONY: all clean
