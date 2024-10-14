#include "process.h"

Process* init_processes (char* filename, int* num_processes) {

    FILE* fp = NULL;
    Process* processes = NULL;
    *num_processes = 0;
    int init_capacity = INIT_CAPACITY;
    
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    processes = (Process*) malloc(init_capacity * sizeof(Process));

    if (!processes) {
        perror("Error: Could not allocate memory for processes.");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%d %s %d %d", &processes[*num_processes].time_arrived, 
    processes[*num_processes].name, &processes[*num_processes].remaining_time, 
    &processes[*num_processes].memory_requirement) == NUM_PROCESS_INPUTS) {

        // Record service time as the initial remaining time for performance statistics
        processes[*num_processes].service_time = processes[*num_processes].remaining_time;
        processes[*num_processes].memory_allocated = NOT_ALLOCATED;
        processes[*num_processes].frames = NULL;
        processes[*num_processes].num_frames = 0;

        (*num_processes)++;

        if (*num_processes == init_capacity) {
            init_capacity *= 2;
            // Resize the array to double the capacity
            Process* temp_processes = (Process*)realloc(processes, init_capacity * sizeof(Process));
            if (!temp_processes) {
                perror("Error: Could not reallocate memory when initialising processes.");
                free(processes);
                fclose(fp);
                exit(EXIT_FAILURE);
            }
            processes = temp_processes;
        }
    }

    fclose(fp);

    // trim the memory to the exact number of processes
    int final_capacity = *num_processes;
    Process* final_processes = (Process*)realloc(processes, final_capacity * sizeof(Process));
    if (!final_processes && *num_processes > 0) {
        perror("Error: Could not cut down memory when initialising processes.");
        free(processes);
        return NULL;
    }

    processes = final_processes;
    return processes;
}

void init_process_manager(ProcessManager* pm, int num_processes, int quantum) {

    pm->simulation_time = 0;
    pm->completed_processes = 0;
    pm->next_process_index = 0;
    pm->num_processes = num_processes;
    pm->quantum = quantum;
    init_scheduler_queue(&pm->current_processes, pm->num_processes);
    pm->last_used_times = (int*) malloc(num_processes * sizeof(int));
    for (int i = 0; i < num_processes; i++) {
        pm->last_used_times[i] = __INT_MAX__;
    }
}

//  Circular queue implementation was adapted from https://www.programiz.com/dsa/circular-queue
void init_scheduler_queue(CircularQueue* queue, int capacity) {
    queue->arr = (int*)malloc(capacity * sizeof(int));
    if (!queue->arr) {
        perror("Failed to allocate memory for scheduler queue");
        exit(EXIT_FAILURE);
    }
    queue->capacity = capacity;
    queue->head = -1; 
    queue->tail = -1; 
    queue->size = 0;
}

void enqueue(CircularQueue* cq, int process_index) {
    if (cq->size == cq->capacity) {
        perror("Error: Attempting to enqueue to a full queue\n");
        exit(EXIT_FAILURE);
    }
    if (cq->head == -1) {
        cq->head = 0;
    }
    // Add the process index to the tail of the circular queue
    cq->tail = (cq->tail + 1) % cq->capacity;
    cq->arr[cq->tail] = process_index;
    cq->size++;

}

int dequeue(CircularQueue* cq) {
    if (cq->size == 0) {
        perror("Queue is empty. Cannot dequeue.\n");
        return -1; 
    }
    // Get the process index stored at the head of the circular queue
    
    int process_index = cq->arr[cq->head];
    if (cq->head == cq->tail) {
        cq->head = -1;
        cq->tail = -1;
    } else {
        cq->head = (cq->head + 1) % cq->capacity; 
    }
    
    cq->size--; 

    return process_index;
}

int is_scheduler_empty(CircularQueue* cq) {
    return cq->size == 0;
}
const char* get_status_string(Status status) {
    switch (status) {
        case READY:
            return "READY";
            break;
        case RUNNING:
            return "RUNNING";
            break;
        case FINISHED:
            return "FINISHED";
            break;
        case EVICTED:
            return "EVICTED";
            break;
        default:
            return "ERROR: Invalid status type.";
    }
}

void print_mem_frames(int* frames, int num_frames) {

    printf("["); 

    if (num_frames == 0) {
        printf("ZEROFRAMES");
        printf("]\n");
        return;
    }

    if (num_frames > 0) {
        // Print the first frame without a leading comma
        printf("%d", frames[0]);

        for (int i = 1; i < num_frames; i++) {
            printf(",%d", frames[i]);
        }
    }

    printf("]\n");
}

int compare_process(Process* p1, Process* p2) {

    // Accoring to project specifications section 6, it can be assumed that process names are distinct.
    if (strcmp(p1->name, p2->name) == 0) {
        return SAME;
    }

    return NOT_SAME;
}

/* Retrieve Least Recently Used (LRU) process based on smallest simulation time of last run. */
Process* find_lru_process(ProcessManager* pm, Process* cur) {
    int min_time = __INT_MAX__;
    int lru_index = 0;

    for (int i = 0; i < pm->num_processes; i++) {
        // Find out the last time the process ran
        Process* lru_process = &pm->processes[i];

        // If the process has no frames allocated, skip it
        if (lru_process->num_frames == 0) {
            continue;
        }
        // if the process is the same as the process we are trying to assign memory to, skip it
        if (compare_process(lru_process, cur) == SAME) {
            continue;
        } 
        int last_used_time = pm->last_used_times[i];
        if (last_used_time < min_time) {
            min_time = last_used_time;
            lru_index = i;
        }
            
    }
    Process* lru_process = &pm->processes[lru_index];
    return lru_process;
}

void free_process_manager(ProcessManager pm) {

    if (pm.processes != NULL) {
        free(pm.processes);
        pm.processes = NULL;
    }

    if (pm.last_used_times != NULL) {
        free(pm.last_used_times);
        pm.last_used_times = NULL;
    }
    if (pm.current_processes.arr != NULL) {
        free(pm.current_processes.arr);
        pm.current_processes.arr = NULL;
    }

}