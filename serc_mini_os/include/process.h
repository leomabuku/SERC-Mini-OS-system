#ifndef PROCESS_H
#define PROCESS_H

#include "common.h"

typedef struct {
    int pid;
    char name[64];
    char service_type[32];
    int burst_time;
    int remaining_time;
    int priority;
    int memory_required;
    int memory_start;
    int is_paged;
    int page_count;
    int page_table[MAX_PROCESS_PAGES];
    int internal_fragmentation;
    ProcessState state;
    int waiting_time;
    int turnaround_time;
    int completion_time;
    int arrival_order;
    int max_need[RESOURCE_TYPES];
    int allocation[RESOURCE_TYPES];
} PCB;

void process_table_init(void);
int create_process(const char *name,
                   const char *service_type,
                   int burst_time,
                   int priority,
                   int memory_required,
                   const int max_need[RESOURCE_TYPES]);
int suspend_process(int pid);
int resume_process(int pid);
int terminate_process(int pid);
PCB *find_process(int pid);
PCB *get_processes(void);
int get_process_count(void);
int get_active_process_count(void);
void reset_all_process_metrics(void);
void process_table_to_string(char *buffer, size_t size);

#endif
