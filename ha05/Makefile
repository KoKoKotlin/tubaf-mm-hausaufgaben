CC=gcc
LD=gcc
CFLAGS=-c -Wall -O3 -Iinclude

# TODO: Insert your OS-specific libs for GLFW!
ifeq ($(OS),Windows_NT)
	LDFLAGS=-Lsrc -lSOIL -lglfw3 -ldl -lopengl32 -lgdi32 -lm
else
	LDFLAGS=-lglfw -lSOIL -ldl -lm
endif

TARGET=n.out
OBJECTS=$(patsubst %.c, %.o, $(wildcard src/*.c))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(TARGET) $(OBJECTS)
