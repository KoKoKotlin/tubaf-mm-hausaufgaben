SHELL = /bin/bash
CC = gcc
FLAGS = -Wall -lm -march=native

OBJECTS = main.o lib/bitmap.o
TARGET = brightness_changer.out

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS)

main.o : lib/bitmap.h
bitmap.o : lib/bitmap.h

%.o : %.c
	$(CC) -c $(FLAGS) -o $@ $<

.PHONY : clean
clean :
	rm -rf $(TARGET) $(OBJECTS)