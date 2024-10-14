#include "paged_allocation.h"


void init_frames(FrameManager* fm) {

    fm->frames_in_use = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {

        fm->frames[i].page_number = NOT_IN_USE;
        fm->frames[i].is_allocated = NOT_ALLOCATED;
    }
}

int allocate_pages(FrameManager* fm, ProcessManager* pm, Process* process_to_allocate) {
    
    int required_frames = ceil((double) process_to_allocate->memory_requirement / FRAME_SIZE);

    if (!process_to_allocate->frames) {
        int* temp_frames = (int*) malloc(required_frames * sizeof(int));
        if (!temp_frames) {
            perror("Error: Could not allocate memory for frames array of a process.");
            return NOT_ALLOCATED;
        }
        process_to_allocate->frames = temp_frames;
        process_to_allocate->num_frames = required_frames;

    }

    int max_allocatable = TOTAL_FRAMES - fm->frames_in_use;
    
    if (max_allocatable >= required_frames) {
        return allocate_frames(fm, process_to_allocate, required_frames);
    } else {
        while (max_allocatable < required_frames) {
            Process* lru_process = find_lru_process(pm, process_to_allocate);
            if (lru_process == NULL) {
                perror("No suitable LRU process found for eviction.\n");
                return NOT_ALLOCATED;
            }
            // evict frames in the LRU process
            max_allocatable += lru_process->num_frames;
            lru_process->memory_allocated = NOT_ALLOCATED;
            print_eviction_notice(pm, lru_process);
            release_frames(fm, lru_process);
        }

        
    }

    return allocate_frames(fm, process_to_allocate, required_frames);
}

int allocate_frames(FrameManager* fm, Process* process, int required_pages) {

    int allocated = 0;
    int i = 0;
    while (i < TOTAL_FRAMES && allocated < required_pages) {
        if (fm->frames[i].is_allocated == NOT_ALLOCATED) {
            fm->frames[i].page_number = process->frames[allocated];
            fm->frames[i].is_allocated = ALLOCATED;
            process->frames[allocated] = i;
            fm->frames_in_use++;
            allocated++;
        }
        i++;
    }
    process->num_frames = allocated;
    
    if (allocated < required_pages) {
        // release the frames that have been allocated
        release_frames(fm, process);
        return NOT_ALLOCATED;
    }
    return ALLOCATED; 
}

void release_frames(FrameManager* fm, Process* process) {

    for (int i = 0; i < process->num_frames; i++) {
        release_frame(&fm->frames[process->frames[i]]);
        fm->frames_in_use--;
    }
    
    process->num_frames = 0;
}

void release_frame(Frame* frame) {

    frame->page_number = NOT_IN_USE;
    frame->is_allocated = NOT_ALLOCATED;
    frame->last_used_time = NOT_IN_USE;
}

void print_eviction_notice(ProcessManager* pm, Process* process) {

    printf("%d,%s,evicted-frames=", pm->simulation_time, get_status_string(EVICTED));
    print_mem_frames(process->frames, process->num_frames);
}


