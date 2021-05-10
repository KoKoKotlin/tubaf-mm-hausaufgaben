SHELL = /bin/bash
CC = gcc
FLAGS = -Wall -lm -O3

OBJECTS = main.o lib/bitmap.o
TARGET = mandel.out

all: $(TARGET)

debug: FLAGS += -DDEBUG -g
debug: $(TARGET)

normal: FLAGS += -DNORMAL
normal: $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(FLAGS)

main.o : lib/bitmap.h
bitmap.o : lib/bitmap.h

%.o : %.c
	$(CC) -c $(FLAGS) -o $@ $<

.PHONY : clean
clean :
	rm -rf $(TARGET) $(OBJECTS) *.bmp