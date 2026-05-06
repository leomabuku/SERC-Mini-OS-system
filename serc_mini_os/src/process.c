#include "process.h"
#include "logger.h"
#include "memory.h"

static PCB process_table[MAX_PROCESSES];
static int process_count = 0;
static int next_pid = 1;

static int is_valid_existing_state(ProcessState state) {
    return state != STATE_TERMINATED;
}

void process_table_init(void) {
    memset(process_table, 0, sizeof(process_table));
    process_count = 0;
    next_pid = 1;
}

int create_process(const char *name,
                   const char *service_type,
                   int burst_time,
                   int priority,
                   int memory_required,
                   const int max_need[RESOURCE_TYPES]) {
    if (process_count >= MAX_PROCESSES ||
        name == NULL || name[0] == '\0' ||
        service_type == NULL || service_type[0] == '\0' ||
        max_need == NULL ||
        burst_time <= 0 || priority <= 0 || memory_required <= 0) {
        return -1;
    }

    PCB *p = &process_table[process_count];
    memset(p, 0, sizeof(*p));

    p->pid = next_pid++;
    snprintf(p->name, sizeof(p->name), "%s", name);
    snprintf(p->service_type, sizeof(p->service_type), "%s", service_type);
    p->burst_time = burst_time;
    p->remaining_time = burst_time;
    p->priority = priority;
    p->memory_required = memory_required;
    p->memory_start = -1;
    p->is_paged = 0;
    p->page_count = 0;
    p->internal_fragmentation = 0;
    p->state = STATE_NEW;
    p->waiting_time = 0;
    p->turnaround_time = 0;
    p->completion_time = 0;
    p->arrival_order = process_count;

    for (int i = 0; i < RESOURCE_TYPES; i++) {
        p->max_need[i] = max_need[i] < 0 ? 0 : max_need[i];
        p->allocation[i] = 0;
    }
    for (int i = 0; i < MAX_PROCESS_PAGES; i++) {
        p->page_table[i] = -1;
    }

    process_count++;
    log_event("Process created: PID=%d, name=%s, service=%s, burst=%d, priority=%d, memory=%d",
              p->pid, p->name, p->service_type, p->burst_time, p->priority, p->memory_required);
    return p->pid;
}

PCB *find_process(int pid) {
    for (int i = 0; i < process_count; i++) {
        if (process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

int suspend_process(int pid) {
    PCB *p = find_process(pid);
    if (p == NULL || !is_valid_existing_state(p->state) || p->state == STATE_SUSPENDED) {
        return 0;
    }

    if (p->state == STATE_NEW) {
        return 0;
    }

    p->state = STATE_SUSPENDED;
    log_event("Process suspended: PID=%d", pid);
    return 1;
}

int resume_process(int pid) {
    PCB *p = find_process(pid);
    if (p == NULL || p->state != STATE_SUSPENDED) {
        return 0;
    }

    p->state = STATE_READY;
    log_event("Process resumed: PID=%d", pid);
    return 1;
}

int terminate_process(int pid) {
    PCB *p = find_process(pid);
    if (p == NULL || p->state == STATE_TERMINATED) {
        return 0;
    }

    p->state = STATE_TERMINATED;
    p->remaining_time = 0;
    free_memory_by_pid(pid);
    log_event("Process terminated: PID=%d", pid);
    return 1;
}

PCB *get_processes(void) {
    return process_table;
}

int get_process_count(void) {
    return process_count;
}

int get_active_process_count(void) {
    int count = 0;
    for (int i = 0; i < process_count; i++) {
        if (process_table[i].state != STATE_TERMINATED) {
            count++;
        }
    }
    return count;
}

void reset_all_process_metrics(void) {
    for (int i = 0; i < process_count; i++) {
        process_table[i].waiting_time = 0;
        process_table[i].turnaround_time = 0;
        process_table[i].completion_time = 0;

        if (process_table[i].state != STATE_TERMINATED) {
            process_table[i].remaining_time = process_table[i].burst_time;

            if (process_table[i].state == STATE_NEW || process_table[i].state == STATE_RUNNING) {
                process_table[i].state = STATE_READY;
            }
        }
    }
}

void process_table_to_string(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    size_t used = 0;
    used += snprintf(buffer + used, size - used,
                     "PID  NAME                 TYPE         BT RT PR MEM START MODE   STATE        WT  TAT CT  ALLOC/MAX (C,V,S)\n");
    used += snprintf(buffer + used, size - used,
                     "-----------------------------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < process_count && used < size; i++) {
        PCB *p = &process_table[i];
        used += snprintf(buffer + used, size - used,
                         "%-4d %-20s %-12s %-2d %-2d %-2d %-3d %-5d %-6s %-12s %-3d %-3d %-3d (%d,%d,%d)/(%d,%d,%d)\n",
                         p->pid, p->name, p->service_type, p->burst_time, p->remaining_time,
                         p->priority, p->memory_required, p->memory_start,
                         p->is_paged ? "PAGED" : "CONTIG",
                         state_to_string(p->state),
                         p->waiting_time, p->turnaround_time, p->completion_time,
                         p->allocation[0], p->allocation[1], p->allocation[2],
                         p->max_need[0], p->max_need[1], p->max_need[2]);
    }
}
