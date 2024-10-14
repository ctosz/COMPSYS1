#include "virtual_allocation.h"

// A process can be executed if at least 4 of its pages are allocated

int allocate_virtual(FrameManager *fm, ProcessManager *pm, Process *process_to_allocate)
{

    int total_required = ceil((double)process_to_allocate->memory_requirement / FRAME_SIZE);

    // If a process requires less than 4 pages, all pages need to be allocated
    int min_requirement = total_required < MIN_PAGE_ALLOCATION ? total_required : MIN_PAGE_ALLOCATION;

    if (process_to_allocate->frames == NULL)
    {
        process_to_allocate->frames = malloc(total_required * sizeof(int));
        assert(process_to_allocate->frames);
        if (!process_to_allocate->frames)
        {
            printf("Fail to allocate virtual memory for process %s\n", process_to_allocate->name);
            return NOT_ALLOCATED;
        }
    }
    // Determine how many frames are available
    int max_allocatable = TOTAL_FRAMES - fm->frames_in_use;

    if (max_allocatable >= total_required)
    {
        return allocate_frames_virtual(fm, process_to_allocate, total_required);
    }

    // Allocate as many frames as possible, even if less than total required
    if (max_allocatable < total_required && max_allocatable >= min_requirement)
    {
        return allocate_frames_virtual(fm, process_to_allocate, max_allocatable);
    }
    // Otherwise just allocate the minimum requirement to run
    if (max_allocatable >= min_requirement)
    {
        return allocate_frames_virtual(fm, process_to_allocate, min_requirement);
    }

    int frames_needed = min_requirement - max_allocatable;
    int num_to_evict = frames_needed;
    // Evict pages
    while (frames_needed > 0)
    {
        Process *lru_process = find_lru_process(pm, process_to_allocate);
        if (lru_process == NULL)
        {
            perror("No suitable LRU process found for eviction.\n");
            return NOT_ALLOCATED;
        }
        // Evict the frames of the LRU process
        int frames_to_release;
        if (lru_process->num_frames <= frames_needed)
        {
            // release all frames of the LRU process
            frames_to_release = lru_process->num_frames;
            lru_process->memory_allocated = NOT_ALLOCATED;
        }
        else
        {
            // release the number of frames needed
            frames_to_release = frames_needed;
        }

        for (int i = 0; i < frames_to_release; i++)
        {
            release_frame_virtual(fm, lru_process->frames[i]);
            frames_needed--;
            max_allocatable++;
        }

        // Update the process's frames array
        update_process_frames(lru_process, frames_to_release);
        
        // If the LRU process has less than 4 frames, mark it as not allocated
        if (lru_process->num_frames < MIN_PAGE_ALLOCATION)
        {
            lru_process->memory_allocated = NOT_ALLOCATED;
        }
    }

    int allocated = allocate_frames_virtual(fm, process_to_allocate, min_requirement);
    printf("%d,%s,evicted-frames=", pm->simulation_time, get_status_string(EVICTED));
    // Only print the frames that were evicted
    print_mem_frames(process_to_allocate->frames, num_to_evict);

    return allocated;
}

void release_frame_virtual(FrameManager *fm, int frame_index)
{
    fm->frames[frame_index].is_allocated = NOT_ALLOCATED;
    fm->frames[frame_index].page_number = NOT_IN_USE;
    fm->frames_in_use--;
}

void release_frames_virtual(FrameManager *fm, Process *process)
{
    for (int i = 0; i < process->num_frames; i++)
    {
        release_frame_virtual(fm, process->frames[i]);
        fm->frames_in_use--;
    }
    process->frames = NULL;
    process->num_frames = 0;
}

void update_process_frames(Process *process, int frames_released)
{
    // Shift remaining frame indices in the process's frame array to close gaps
    memmove(process->frames, process->frames + frames_released, (process->num_frames - frames_released) * sizeof(int));
    process->num_frames -= frames_released;
}

int allocate_frames_virtual(FrameManager *fm, Process *process, int num_frames)
{

    // Allows top up of frames for a process

    int existing_frames = process->num_frames;

    int allocated = 0;
    for (int i = 0; i < TOTAL_FRAMES && allocated < num_frames; i++)
    {
        if (fm->frames[i].is_allocated == NOT_ALLOCATED)
        {
            fm->frames[i].is_allocated = ALLOCATED;
            fm->frames[i].page_number = process->frames[allocated];
            process->frames[allocated] = i;
            allocated++;
        }
    }
    process->num_frames += allocated;
    fm->frames_in_use += allocated;

    if (allocated < num_frames)
    {
        // release the frames that were already allocated
        for (int i = existing_frames; i < process->num_frames; i++)
        {
            release_frame_virtual(fm, process->frames[i]);
        }
        return NOT_ALLOCATED;
    }
    return ALLOCATED;
}
