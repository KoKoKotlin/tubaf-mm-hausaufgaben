SHELL = /bin/bash
CC = gcc
FLAGS = -Wall

OBJECTS = main.o lib/bitmap.o
TARGET = alpha_blender.out

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS)

main.o : lib/bitmap.h
bitmap.o : lib/bitmap.h

%.o : %.c
	$(CC) -c $(FLAGS) -o $@ $<

.PHONY : clean
clean :
	rm -rf $(TARGET) $(OBJECTS)