#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <stddef.h>

#define MAX_SCHEDULE_SEGMENTS (MAX_PROCESSES * 32)

typedef enum {
    SCHED_FCFS = 0,
    SCHED_SJF,
    SCHED_PRIORITY,
    SCHED_RR
} SchedulerType;

typedef struct {
    int pid;
    int burst_time;
    int start_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int was_scheduled;
} ProcessScheduleMetric;

typedef struct {
    int pid;
    int start_time;
    int end_time;
} ScheduleSegment;

typedef struct {
    SchedulerType type;

    int process_ids[MAX_PROCESSES];
    int process_count;

    int execution_slices[MAX_SCHEDULE_SEGMENTS];
    int execution_pids[MAX_SCHEDULE_SEGMENTS];
    int slice_count;

    ScheduleSegment segments[MAX_SCHEDULE_SEGMENTS];
    int segment_count;

    ProcessScheduleMetric metrics[MAX_PROCESSES];
    int metric_count;

    int total_busy_time;
    int total_time;

    double average_waiting_time;
    double average_turnaround_time;
    double cpu_utilization;
} ScheduleResult;

/* Runs the selected scheduler and fills the result structure */
void run_scheduler(SchedulerType type, int quantum, ScheduleResult *result);

/* Runs the selected scheduler without mutating process state */
void run_scheduler_preview(SchedulerType type, int quantum, ScheduleResult *result);

/* Existing summary text output */
void schedule_result_to_string(const ScheduleResult *result, char *buffer, size_t size);

/* New: detailed computation report for viva / explanation */
void schedule_result_computation_to_string(const ScheduleResult *result, char *buffer, size_t size);

/* Scheduler display name */
const char *scheduler_to_string(SchedulerType type);

/* New: store/retrieve the most recent scheduler run for GUI visualization */
void scheduler_set_last_result(const ScheduleResult *result);
const ScheduleResult *scheduler_get_last_result(void);

/* New: helpers for GUI */
int scheduler_has_last_result(void);
int scheduler_copy_last_result(ScheduleResult *result_out);

#endif
