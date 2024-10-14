#ifndef VIRTUAL_ALLOCATION_H
#define VIRTUAL_ALLOCATION_H
#include "paged_allocation.h"

#define MIN_PAGE_TO_RUN 4

int allocate_virtual(FrameManager* fm, ProcessManager* pm, Process* process_to_allocate); 
void allocate_num_pages(FrameManager* fm, Process* process_to_allocate, int target_pages);
int allocate_frames_virtual(FrameManager* fm, Process* process, int num_frames);
void release_frame_virtual(FrameManager* fm, int frame_index);
void update_process_frames(Process* process, int frames_released);
void evict_pages_virtual(FrameManager* fm, ProcessManager* pm, Process* process_to_allocate);
void release_frames_virtual(FrameManager* fm, Process* process);

#endif // VIRTUAL_ALLOCATION_H