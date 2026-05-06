#include "memory.h"
#include "logger.h"
#include "process.h"

static MemorySegment segments[MAX_SEGMENTS];
static int segment_count = 0;
static MemoryFrame frames[TOTAL_FRAMES];

static void insert_segment(int index, MemorySegment seg) {
    if (segment_count >= MAX_SEGMENTS) {
        return;
    }

    for (int i = segment_count; i > index; i--) {
        segments[i] = segments[i - 1];
    }

    segments[index] = seg;
    segment_count++;
}

static void compact_free_neighbors(void) {
    for (int i = 0; i < segment_count - 1;) {
        if (segments[i].is_free && segments[i + 1].is_free) {
            segments[i].size += segments[i + 1].size;
            for (int j = i + 1; j < segment_count - 1; j++) {
                segments[j] = segments[j + 1];
            }
            segment_count--;
        } else {
            i++;
        }
    }
}

static void reset_process_paging_fields(PCB *p) {
    if (p == NULL) {
        return;
    }

    p->is_paged = 0;
    p->page_count = 0;
    p->internal_fragmentation = 0;
    for (int i = 0; i < MAX_PROCESS_PAGES; i++) {
        p->page_table[i] = -1;
    }
}

static void init_frame_table(void) {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        frames[i].frame_number = i;
        frames[i].start = i * PAGE_SIZE;
        frames[i].size = PAGE_SIZE;
        frames[i].is_free = 1;
        frames[i].pid = -1;
        frames[i].page_number = -1;
    }
}

void memory_init(void) {
    memset(segments, 0, sizeof(segments));
    segment_count = 1;
    segments[0].start = 0;
    segments[0].size = TOTAL_MEMORY;
    segments[0].is_free = 1;
    segments[0].pid = -1;
    segments[0].is_page = 0;
    segments[0].page_number = -1;
    init_frame_table();
}

static int overlaps_paged_frames(int start, int size) {
    int end = start + size;

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        int frame_end = frames[i].start + frames[i].size;
        if (!frames[i].is_free && frames[i].page_number >= 0 &&
            start < frame_end && end > frames[i].start) {
            return 1;
        }
    }

    return 0;
}

static int find_segment_index(int size, MemoryStrategy strategy) {
    int chosen = -1;

    if (strategy == MEM_FIRST_FIT) {
        for (int i = 0; i < segment_count; i++) {
            if (segments[i].is_free && segments[i].size >= size &&
                !overlaps_paged_frames(segments[i].start, size)) {
                return i;
            }
        }
    } else if (strategy == MEM_BEST_FIT) {
        int best_size = TOTAL_MEMORY + 1;
        for (int i = 0; i < segment_count; i++) {
            if (segments[i].is_free && segments[i].size >= size &&
                segments[i].size < best_size &&
                !overlaps_paged_frames(segments[i].start, size)) {
                best_size = segments[i].size;
                chosen = i;
            }
        }
    } else {
        int worst_size = -1;
        for (int i = 0; i < segment_count; i++) {
            if (segments[i].is_free && segments[i].size >= size &&
                segments[i].size > worst_size &&
                !overlaps_paged_frames(segments[i].start, size)) {
                worst_size = segments[i].size;
                chosen = i;
            }
        }
    }

    return chosen;
}

static void reserve_frames_for_contiguous(int pid, int start, int size) {
    int end = start + size;

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        int frame_end = frames[i].start + frames[i].size;
        if (start < frame_end && end > frames[i].start) {
            frames[i].is_free = 0;
            frames[i].pid = pid;
            frames[i].page_number = -1;
        }
    }
}

static int count_free_page_frames(void) {
    int count = 0;

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (frames[i].is_free) {
            count++;
        }
    }

    return count;
}

static int allocate_paged_memory(int pid, int size) {
    PCB *p;
    int pages;
    int first_frame = -1;
    int assigned = 0;

    if (size <= 0) {
        return -1;
    }

    pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (pages <= 0 || pages > MAX_PROCESS_PAGES ||
        count_free_page_frames() < pages ||
        get_memory_free() < pages * PAGE_SIZE) {
        log_event("Paging allocation failed: PID=%d requested=%d pages=%d", pid, size, pages);
        return -1;
    }

    p = find_process(pid);
    if (p == NULL) {
        return -1;
    }

    reset_process_paging_fields(p);

    for (int i = 0; i < TOTAL_FRAMES && assigned < pages; i++) {
        if (!frames[i].is_free) {
            continue;
        }

        frames[i].is_free = 0;
        frames[i].pid = pid;
        frames[i].page_number = assigned;
        p->page_table[assigned] = i;
        if (first_frame < 0) {
            first_frame = i;
        }
        assigned++;
    }

    p->is_paged = 1;
    p->page_count = pages;
    p->internal_fragmentation = pages * PAGE_SIZE - size;
    p->memory_start = (first_frame >= 0) ? frames[first_frame].start : -1;
    if (p->state == STATE_NEW) {
        p->state = STATE_READY;
    }

    log_event("Paging memory allocated: PID=%d pages=%d frames=%d internal_frag=%d",
              pid, pages, assigned, p->internal_fragmentation);
    return p->memory_start;
}

int allocate_memory(int pid, int size, MemoryStrategy strategy) {
    int index;
    int start;
    MemorySegment chosen;

    if (size <= 0) {
        return -1;
    }

    if (strategy == MEM_PAGING) {
        return allocate_paged_memory(pid, size);
    }

    if (size > get_memory_free()) {
        log_event("Memory allocation failed: PID=%d requested=%d available=%d strategy=%s",
                  pid, size, get_memory_free(), memory_strategy_to_string(strategy));
        return -1;
    }

    index = find_segment_index(size, strategy);
    if (index < 0) {
        log_event("Memory allocation failed: PID=%d, requested=%d, strategy=%s",
                  pid, size, memory_strategy_to_string(strategy));
        return -1;
    }

    chosen = segments[index];
    if (chosen.size > size && segment_count >= MAX_SEGMENTS) {
        log_event("Memory allocation failed: segment table full for PID=%d", pid);
        return -1;
    }

    start = chosen.start;

    segments[index].is_free = 0;
    segments[index].pid = pid;
    segments[index].size = size;
    segments[index].is_page = 0;
    segments[index].page_number = -1;

    if (chosen.size > size) {
        MemorySegment remaining;
        remaining.start = chosen.start + size;
        remaining.size = chosen.size - size;
        remaining.is_free = 1;
        remaining.pid = -1;
        remaining.is_page = 0;
        remaining.page_number = -1;
        insert_segment(index + 1, remaining);
    }

    PCB *p = find_process(pid);
    if (p != NULL) {
        reset_process_paging_fields(p);
        p->memory_start = start;
        if (p->state == STATE_NEW) {
            p->state = STATE_READY;
        }
    }

    reserve_frames_for_contiguous(pid, start, size);

    log_event("Memory allocated: PID=%d, start=%d, size=%d, strategy=%s",
              pid, start, size, memory_strategy_to_string(strategy));
    return start;
}

void free_memory_by_pid(int pid) {
    int released = 0;

    for (int i = 0; i < segment_count; i++) {
        if (!segments[i].is_free && segments[i].pid == pid) {
            segments[i].is_free = 1;
            segments[i].pid = -1;
            segments[i].is_page = 0;
            segments[i].page_number = -1;
            released = 1;
        }
    }

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!frames[i].is_free && frames[i].pid == pid) {
            frames[i].is_free = 1;
            frames[i].pid = -1;
            frames[i].page_number = -1;
            released = 1;
        }
    }

    if (released) {
        PCB *p = find_process(pid);
        if (p != NULL) {
            p->memory_start = -1;
            reset_process_paging_fields(p);
        }

        compact_free_neighbors();
        log_event("Memory released: PID=%d", pid);
    }
}

int get_memory_used(void) {
    int used = 0;

    for (int i = 0; i < segment_count; i++) {
        if (!segments[i].is_free) {
            used += segments[i].size;
        }
    }

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!frames[i].is_free && frames[i].page_number >= 0) {
            used += frames[i].size;
        }
    }

    return used;
}

int get_memory_free(void) {
    int free_mem = TOTAL_MEMORY - get_memory_used();
    return free_mem < 0 ? 0 : free_mem;
}

int get_fragment_count(void) {
    int count = 0;
    for (int i = 0; i < segment_count; i++) {
        if (segments[i].is_free) {
            count++;
        }
    }
    return count;
}

int get_memory_segment_count(void) {
    return segment_count;
}

int copy_memory_segments(MemorySegment *out, int max_segments) {
    int count;

    if (out == NULL || max_segments <= 0) {
        return 0;
    }

    count = segment_count < max_segments ? segment_count : max_segments;
    for (int i = 0; i < count; i++) {
        out[i] = segments[i];
    }

    return count;
}

int copy_memory_frames(MemoryFrame *out, int max_frames) {
    int count;

    if (out == NULL || max_frames <= 0) {
        return 0;
    }

    count = TOTAL_FRAMES < max_frames ? TOTAL_FRAMES : max_frames;
    for (int i = 0; i < count; i++) {
        out[i] = frames[i];
    }

    return count;
}

int get_paging_internal_fragmentation(void) {
    PCB *table = get_processes();
    int count = get_process_count();
    int total = 0;

    for (int i = 0; i < count; i++) {
        if (table[i].state != STATE_TERMINATED && table[i].is_paged) {
            total += table[i].internal_fragmentation;
        }
    }

    return total;
}

void memory_map_to_string(char *buffer, size_t size) {
    size_t used = 0;

    if (buffer == NULL || size == 0) {
        return;
    }

    buffer[0] = '\0';

    used += snprintf(buffer + used, size - used,
                     "Memory Used: %d / %d | Free: %d | External Fragments: %d | Paging Internal Fragmentation: %d\n",
                     get_memory_used(), TOTAL_MEMORY, get_memory_free(), get_fragment_count(),
                     get_paging_internal_fragmentation());
    used += snprintf(buffer + used, size - used,
                     "Contiguous Segments\nSTART  SIZE  STATUS  PID\n-------------------------\n");

    for (int i = 0; i < segment_count && used < size; i++) {
        used += snprintf(buffer + used, size - used,
                         "%-6d %-5d %-7s %-3d\n",
                         segments[i].start, segments[i].size,
                         segments[i].is_free ? "FREE" : "USED",
                         segments[i].pid);
    }

    used += snprintf(buffer + used, size - used,
                     "\nPaging Frames\nFRAME  START  STATUS  PID  PAGE\n--------------------------------\n");
    for (int i = 0; i < TOTAL_FRAMES && used < size; i++) {
        used += snprintf(buffer + used, size - used,
                         "%-6d %-6d %-7s %-4d %-4d\n",
                         frames[i].frame_number,
                         frames[i].start,
                         frames[i].is_free ? "FREE" : "USED",
                         frames[i].pid,
                         frames[i].page_number);
    }
}

const char *memory_strategy_to_string(MemoryStrategy strategy) {
    switch (strategy) {
        case MEM_FIRST_FIT: return "First Fit";
        case MEM_BEST_FIT: return "Best Fit";
        case MEM_WORST_FIT: return "Worst Fit";
        case MEM_PAGING: return "Paging";
        default: return "Unknown";
    }
}
