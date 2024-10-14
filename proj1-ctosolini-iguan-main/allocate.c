#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include "process.h"
#include "memory_allocation.h"
#include "paged_allocation.h"
#include "virtual_allocation.h"

void round_robin_scheduler(ProcessManager* pm, MemoryManager* mm, FrameManager* fm);
int allocate_memory(Process* process, ProcessManager* pm, MemoryManager* mm, FrameManager* fm);
void print_performance_stats(ProcessManager pm);
int allocate_infinite(Process *process);
int allocate_first_fit(MemoryManager *mm, Process *process);
void free_memory(MemoryManager *mm, Process *process);
void load_processes(ProcessManager* pm);
void execute_process(Process* process_to_run, ProcessManager *pm, MemoryManager *mm, FrameManager *fm);
void print_process_status(MemoryStrategy strategy, ProcessManager *pm, Process *process_to_run, MemoryManager *mm, FrameManager *fm);

int main (int argc, char* argv[]) {
    // hello
    
    ProcessManager pm;
    Process* processes = NULL;
    int num_processes;
    MemoryStrategy memory_strategy = INFINITE; // Default memory strategy
    int quantum; 

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "f:m:q:")) != -1) {
        switch (opt) {
            case 'f':
                processes = init_processes(optarg, &num_processes);
                if (!processes) {
                    perror("Error: Failed to initialise processes\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':
                if (strcmp(optarg, "infinite") == 0) {
                    memory_strategy = INFINITE;
                } else if (strcmp(optarg, "first-fit") == 0) {
                    memory_strategy = FIRST_FIT;
                } else if (strcmp(optarg, "paged") == 0) {
                    memory_strategy = PAGED;
                } 
                else if (strcmp(optarg, "virtual") == 0) {
                    memory_strategy = VIRTUAL;
                }
                else {
                    fprintf(stderr, "Error: Invalid memory strategy %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;

            case 'q':
                quantum = atol(optarg);
                if (quantum < 1 || quantum > 3) {
                    fprintf(stderr, "Error: Invalid quantum time of %d\n", quantum);
                    exit(EXIT_FAILURE);
                }
                break;
            }
        }

    // Initialise the process manager
    init_process_manager(&pm, num_processes, quantum);
    pm.processes = processes;
    MemoryManager* mm = create_memory_manager();
    FrameManager fm;
    init_frames(&fm);
    mm = init_memory_manager(mm, memory_strategy);
    

    round_robin_scheduler(&pm, mm, &fm);
    print_performance_stats(pm);

    // Free memory
    free_process_manager(pm);
    free_memory_manager(mm);

    return 0;
}


void load_processes(ProcessManager* pm) {
    // Add all arrived processes to the scheduler queue
    while (pm->next_process_index < pm->num_processes &&
            pm->processes[pm->next_process_index].time_arrived <= pm->simulation_time) {
        pm->processes[pm->next_process_index].status = READY;
        enqueue(&pm->current_processes, pm->next_process_index);
            pm->next_process_index++;
    }
}


void execute_process(Process* process_to_run, ProcessManager *pm, MemoryManager *mm, FrameManager *fm) {

    // Run the process for the quantum time
    process_to_run->remaining_time -= pm->quantum;
    pm->simulation_time += pm->quantum;
    
    // Check if the process has finished
    if (process_to_run->remaining_time <= 0) {

        process_to_run->status = FINISHED;

        // Record completion time for performance statistics
        process_to_run->completion_time = pm->simulation_time;
        process_to_run->remaining_time = 0;
        pm->completed_processes++;

        // If the process was finished, print the evicted frames
        if ((mm->strategy == PAGED) | (mm->strategy == VIRTUAL)) {
            printf("%d,%s,evicted-frames=", pm->simulation_time, get_status_string(EVICTED));
            print_mem_frames(process_to_run->frames, process_to_run->num_frames);
            release_frames(fm, process_to_run);
            
            if (process_to_run->frames) {
                free(process_to_run->frames);
                process_to_run->frames = NULL;
            }
        }

        load_processes(pm);
        printf("%d,%s,process-name=%s,proc-remaining=%d\n", pm->simulation_time, get_status_string(process_to_run->status),
                    process_to_run->name, pm->current_processes.size);
        
        if (mm->strategy == FIRST_FIT) {
            mm->mem_available += process_to_run->memory_requirement;
            free_memory(mm, process_to_run); // Free the allocated memory
        }
        pm->interrupted_process_index = NOT_INTERRUPTED;
        
    } 
}


void round_robin_scheduler(ProcessManager* pm, MemoryManager* mm, FrameManager* fm) {

    pm->interrupted_process_index = NOT_INTERRUPTED;    
    // Add all arrived processes to the scheduler queue
    // Run the scheduler until all processes are completed
    while (pm->completed_processes < pm->num_processes) {
        
        load_processes(pm);

        Process* process_to_run = NULL;
        // Requeue the process that was interrupted by the quantum
        int was_interrupted = pm->interrupted_process_index; // before resetting, record whether or not the process was interrupted 
        if (pm->interrupted_process_index != NOT_INTERRUPTED) {
            pm->processes[pm->interrupted_process_index].status = READY;
            enqueue(&pm->current_processes, pm->interrupted_process_index);
            pm->interrupted_process_index = NOT_INTERRUPTED; 
        }

        // Run the next process in the scheduler queue
        if (!is_scheduler_empty(&pm->current_processes)) {
            int process_index = dequeue(&pm->current_processes);
            process_to_run = &pm->processes[process_index];
            

            // Allocate memory for the process if it has not been allocated
            if (!process_to_run->memory_allocated && allocate_memory(process_to_run, pm, mm, fm)) {
                process_to_run->memory_allocated = ALLOCATED;
            } 
            
            if (process_to_run->memory_allocated) {
                
                if (was_interrupted != process_index) { // was previous process (was_interrupted) the same as current process?
                    process_to_run->status = RUNNING;
                    print_process_status(mm->strategy, pm, process_to_run, mm, fm);     
                }
                pm->interrupted_process_index = process_index;
                pm->last_used_times[process_index] = pm->simulation_time;
                execute_process(process_to_run, pm, mm, fm);

            } else {
                // Memory allocation failed, re-enqueue the process to the tail.
                enqueue(&pm->current_processes, process_index);
            }

        // If there are no processes to run, increment the simulation time
        } else {
            pm->simulation_time += pm->quantum;
        }
    }
} 


int allocate_memory(Process* process_to_run, ProcessManager* pm, MemoryManager* mm, FrameManager* fm) {
    int allocated = NOT_ALLOCATED;

    switch (mm->strategy) {
        case INFINITE:
            allocated = allocate_infinite(process_to_run);
            break;

        case FIRST_FIT:
            allocated = allocate_first_fit(mm, process_to_run);
            break;

        case PAGED:
            allocated = allocate_pages(fm, pm, process_to_run);   
            break;

        case VIRTUAL:
            allocated = allocate_virtual(fm, pm, process_to_run);
            break;
        default:
            fprintf(stderr, "Unsupported memory strategy\n");
            exit(EXIT_FAILURE);
    }

    return allocated;

}



int allocate_infinite(Process *process) {
    // Infinite allocation logic
    return ALLOCATED;
}


int allocate_first_fit(MemoryManager *mm, Process *process) {
    assert(mm != NULL && mm->head != NULL);
    MemoryAddress *curr = mm->head;

    while (curr) {
        if (curr->use == HOLE && curr->length >= process->memory_requirement) {
            int mem_leftover = curr->length - process->memory_requirement;
            
            // Allocate memory by resizing the current block
            curr->length = process->memory_requirement;
            curr->use = PROCESS;
            process->memory_block = curr;
            process->memory_allocated = ALLOCATED;

            mm->mem_available -= process->memory_requirement; 

            // If there's leftover memory, create a new hole after the current block
            if (mem_leftover > 0) {
                MemoryAddress *new_hole = malloc(sizeof(MemoryAddress));
                if (!new_hole) {
                    perror("Failed to allocate memory for new hole");
                    exit(EXIT_FAILURE);
                }
                new_hole->starting_address = curr->starting_address + process->memory_requirement;
                new_hole->length = mem_leftover;
                new_hole->use = HOLE;
                new_hole->nxt = curr->nxt;
                curr->nxt = new_hole;
            }

            return ALLOCATED;
        }
        curr = curr->nxt;
    }

    // No suitable block found
    process->memory_block = NULL;
    return NOT_ALLOCATED;
}

void free_memory(MemoryManager *mm, Process *process) {
    MemoryAddress *block = process->memory_block;
    // No memory was allocated, nothing to free
    if (block == NULL) return; 
    // Mark the block as free
    block->use = HOLE; 

    // Merge adjacent free blocks
    MemoryAddress *curr = mm->head, *prev = NULL;
    while (curr != NULL) {
        if (curr->use == HOLE && prev != NULL && prev->use == HOLE) {
            // Merge current into prev
            prev->length += curr->length;
            prev->nxt = curr->nxt;
            // Free the struct of the merged block
            free(curr); 
            // Continue from the merged block
            curr = prev->nxt; 
        } else {
            prev = curr;
            curr = curr->nxt;
        }
    }
    process->memory_block = NULL;
    process->memory_allocated = NOT_ALLOCATED;
}


// Print status statements for any memory allocation type
// If finished: print special finished line 

void print_process_status(MemoryStrategy strategy, ProcessManager *pm, Process *process_to_run, MemoryManager *mm, FrameManager *fm) {

    double mem_percent;
    double fmem_percent;
    double vmem_percent;

    switch (strategy) {
        case INFINITE:

            printf("%d,%s,process-name=%s,remaining-time=%d\n", pm->simulation_time, 
            get_status_string(process_to_run->status),process_to_run->name, process_to_run->remaining_time);
            break;

        case FIRST_FIT:

            mem_percent = (double) (MAX_KB_AVAILABLE -  mm->mem_available)/MAX_KB_AVAILABLE * 100;

            printf("%d,%s,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", pm->simulation_time, 
                get_status_string(process_to_run->status), process_to_run->name, process_to_run->remaining_time, 
                    (int) ceil(mem_percent), process_to_run->memory_block->starting_address); 
            break;

        case PAGED:
            fmem_percent = (double) (fm->frames_in_use)/TOTAL_FRAMES * 100.0;

            printf("%d,%s,process-name=%s,remaining-time=%d,mem-usage=%d%%,", pm->simulation_time, 
                get_status_string(process_to_run->status), process_to_run->name, process_to_run->remaining_time, 
                    (int) ceil(fmem_percent)); 
            printf("mem-frames=");
            print_mem_frames(process_to_run->frames, process_to_run->num_frames);
            break;

        case VIRTUAL:
            vmem_percent = (double) (fm->frames_in_use)/TOTAL_FRAMES * 100.0;

            printf("%d,%s,process-name=%s,remaining-time=%d,mem-usage=%d%%,", pm->simulation_time, 
                get_status_string(process_to_run->status), process_to_run->name, process_to_run->remaining_time, 
                    (int) ceil(vmem_percent)); 
            printf("mem-frames=");
            print_mem_frames(process_to_run->frames, process_to_run->num_frames);
            break;
        default:
            fprintf(stderr, "Unsupported memory strategy\n");
            exit(EXIT_FAILURE);
    }

}

void print_performance_stats(ProcessManager pm) {

    int total_turnaround_time = 0;
    int average_turnaround_time;
    double total_overhead_time = 0;
    double max_overhead_time = 0;
    double average_overhead_time;

    for (int i = 0; i < pm.num_processes; i++) {
        int turnaround_time = pm.processes[i].completion_time - pm.processes[i].time_arrived;
        double overhead_time = (double)turnaround_time/pm.processes[i].service_time;
        // Find the maximum overhead time
        if (overhead_time > max_overhead_time) {
            max_overhead_time = overhead_time;
        }
        total_turnaround_time += turnaround_time;
        total_overhead_time += overhead_time;
    }

    // Calculate the average turnaround and overhead time
    average_turnaround_time = (int)ceil((double)total_turnaround_time / pm.num_processes);
    average_overhead_time = ceil(total_overhead_time / pm.num_processes * 100) / 100.0;

    printf("Turnaround time %d\n", average_turnaround_time);
    printf("Time overhead %.2f %.2f\n", max_overhead_time, average_overhead_time);
    printf("Makespan %d\n", pm.simulation_time);

}