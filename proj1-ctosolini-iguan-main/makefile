# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -g

LDFLAGS = -lm

# Executable name
TARGET = allocate

# Source files
SOURCES = allocate.c process.c memory_allocation.c paged_allocation.c virtual_allocation.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile object files from source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove build artifacts
clean:
	rm -f $(TARGET) $(OBJECTS)

