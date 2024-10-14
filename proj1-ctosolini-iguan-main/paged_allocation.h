#ifndef PAGED_ALLOCATION_H
#define PAGED_ALLOCATION_H
#include "process.h"

#define FRAME_SIZE 4 
#define TOTAL_FRAMES (MAX_KB_AVAILABLE/FRAME_SIZE)
#define NOT_IN_USE -1
#define MIN_PAGE_ALLOCATION 4 // for virtual: minimum number of pages required to be allocated for a process to be able to run.

typedef struct Frame {
    int page_number;
    int is_allocated;
    int last_used_time;
} Frame;

typedef struct FrameManager {
    Frame frames[TOTAL_FRAMES];
    int frames_in_use;
} FrameManager;  

void init_frames(FrameManager* fm);
int allocate_pages(FrameManager* fm, ProcessManager* pm, Process* process_to_allocate);
int allocate_frames(FrameManager* fm, Process* process, int required_pages);
void release_frames(FrameManager* fm, Process* process);
void release_frame(Frame* frame);
void print_eviction_notice(ProcessManager* pm, Process* process);


#endif // PAGED_ALLOCATION_H