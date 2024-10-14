#ifndef PROCESS_H
#define PROCESS_H

#include "memory_allocation.h"

#define MAX_PROCESS_NAME 8
#define INIT_CAPACITY 10
#define NUM_PROCESS_INPUTS 4 // number of input statistics per process read in 
#define ALLOCATED 1
#define NOT_ALLOCATED 0
#define NOT_INTERRUPTED -1

typedef enum {
    READY,
    RUNNING,
    FINISHED,
    EVICTED
} Status;

typedef struct {
    int time_arrived;
    char name[MAX_PROCESS_NAME];
    int service_time;
    int remaining_time;
    int completion_time;
    int memory_requirement;
    int memory_allocated; // Flag to indicate if memory has been allocated
    MemoryAddress *memory_block; // Pointer to the memory block allocated to the process
    int* frames; // Array of frames indices allocated to the process
    int num_frames; // Number of frames allocated to the process
    Status status;
} Process;

typedef struct {
    int* arr;
    int size;
    int capacity;
    int head;
    int tail;
} CircularQueue;


typedef struct {
    Process* processes;
    int* last_used_times;
    int num_processes;
    int simulation_time;
    int quantum;
    CircularQueue current_processes; // Circular queue of processes to be run
    int completed_processes;
    int next_process_index;
    int interrupted_process_index;
} ProcessManager;


Process* init_processes (char* filename, int* num_processes);
void init_process_manager(ProcessManager* pm, int num_processes, int quantum);
void init_scheduler_queue(CircularQueue* queue, int capacity);
void enqueue(CircularQueue* cq, int process_index);
int dequeue(CircularQueue* cq);
int is_scheduler_empty(CircularQueue* cq);
void free_pages(Process* process);
const char* get_status_string(Status status);
void print_mem_frames(int* frames, int num_frames);
int time_last_used(Process* process);
int compare_process(Process* p1, Process* p2);
Process* find_lru_process(ProcessManager* pm, Process* cur);
void free_process_manager(ProcessManager pm);

#endif // PROCESS_H