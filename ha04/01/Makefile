SHELL = /bin/bash
CC = gcc
FLAGS = -Wall
LDFLAGS=-lm

OBJECTS = main.o bitmap.o compress.o decompress.o dctquant.o
TARGET = dct

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o : %.c
	$(CC) -c $(FLAGS) -o $@ $<

.PHONY : clean
clean :
	rm -rf $(TARGET) $(OBJECTS) *.bmp *.bin