#include "memory_allocation.h"

// Keep track of memory usage: data structure to represent state of memory
// Each entry in the linked list is a memory address of space >=1KB

MemoryManager* create_memory_manager() {

    MemoryManager* mm = (MemoryManager*)malloc(sizeof(MemoryManager));
    assert(mm != NULL); 
    mm->head = NULL;
    mm->tail = NULL;
    mm->mem_available = MAX_KB_AVAILABLE;

    return mm;
}

// starts with 1 element of size MAX_KB_AVAILABLE
MemoryManager* init_memory_manager(MemoryManager* mm, MemoryStrategy strategy) {

    MemoryAddress* new;
    new = (MemoryAddress*)malloc(sizeof(*new));

    assert(new != NULL && mm != NULL);

    new->use = HOLE;
    new->length = MAX_KB_AVAILABLE;
    new->nxt = new->prev = NULL;
    new->starting_address = 0; // starts at 0, ends at length. [0, 2048)

    mm->head = mm->tail = new;
    mm->strategy = strategy;
    return mm;
}

void free_memory_manager(MemoryManager* mm) {

    MemoryAddress* curr;
    MemoryAddress* prev;
    
    assert(mm != NULL);

    curr = mm->head;
    while(curr) {
        prev = curr;
        curr = curr->nxt;
        free(prev);
    }

    free(mm);
}

int compare_mem_address(MemoryAddress* a1, MemoryAddress* a2) {

    if (a1->length == a2->length && a1->starting_address == a2->starting_address && a1->use == a2->use) {
        return SAME;
    }
    else {
        return NOT_SAME;
    }
}


