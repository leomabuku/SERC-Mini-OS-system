#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

typedef enum {
    MEM_FIRST_FIT = 0,
    MEM_BEST_FIT,
    MEM_WORST_FIT,
    MEM_PAGING
} MemoryStrategy;

typedef struct {
    int start;
    int size;
    int is_free;
    int pid;
    int is_page;
    int page_number;
} MemorySegment;

typedef struct {
    int frame_number;
    int start;
    int size;
    int is_free;
    int pid;
    int page_number;
} MemoryFrame;

void memory_init(void);
int allocate_memory(int pid, int size, MemoryStrategy strategy);
void free_memory_by_pid(int pid);
int get_memory_used(void);
int get_memory_free(void);
int get_fragment_count(void);
int get_memory_segment_count(void);
int copy_memory_segments(MemorySegment *out, int max_segments);
int copy_memory_frames(MemoryFrame *out, int max_frames);
int get_paging_internal_fragmentation(void);
void memory_map_to_string(char *buffer, size_t size);
const char *memory_strategy_to_string(MemoryStrategy strategy);

#endif
