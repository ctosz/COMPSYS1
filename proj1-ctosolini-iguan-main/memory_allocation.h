#ifndef MEMORY_ALLOCATION_H
#define MEMORY_ALLOCATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <assert.h>

// Constants for memory allocation sizes and states
#define MIN_MEMORY_ADDRESS_SIZE = 1 // in kb
#define MAX_KB_AVAILABLE 2048
#define HOLE 1 // memory space is free
#define PROCESS 0 // memory space not free
#define ALLOCATED 1
#define NOT_ALLOCATED 0
#define SAME 1 // Comparison is true
#define NOT_SAME 0 // Comparison is false


typedef enum {
    INFINITE,
    FIRST_FIT,
    PAGED,
    VIRTUAL
} MemoryStrategy;

typedef struct MemoryAddress MemoryAddress;

typedef struct MemoryAddress {
    int use; // if 1, it is a hole/free space. if 0, used for a process
    int starting_address;
    int length;
    struct MemoryAddress *nxt;
    struct MemoryAddress *prev;
} MemoryAddress;


typedef struct MemoryManager {
    MemoryAddress* head; // Pointer to the first memory block
    MemoryAddress* tail; // Pointer to the last memory block
    int mem_available; // Amount of memory still free to be allocated 
    MemoryStrategy strategy; // The strategy used to allocate memory
} MemoryManager;

// Function prototypes for managing the memory
MemoryManager* create_memory_manager(void);
MemoryManager* init_memory_manager(MemoryManager* mm, MemoryStrategy strategy);
void free_memory_manager(MemoryManager* mm);
int compare_mem_address(MemoryAddress* a1, MemoryAddress* a2);

#endif // MEMORY_MANAGER_H