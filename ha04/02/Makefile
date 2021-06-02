SHELL = /bin/bash
CC = gcc
FLAGS = -Wall -fopenmp -O3
LDFLAGS=-lm -lgomp

OBJECTS = main.o bitmap.o compress.o decompress.o dctquant.o
TARGET = dct

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o : %.c
	$(CC) -c $(FLAGS) -o $@ $<

.PHONY : clean
clean :
	rm -rf $(TARGET) $(OBJECTS)