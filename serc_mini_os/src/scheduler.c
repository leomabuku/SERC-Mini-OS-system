#include "scheduler.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    int pid;
    int burst_time;
    int remaining_time;
    int priority;
    int arrival_order;
} SchedulerWorkItem;

static ScheduleResult g_last_result;
static int g_has_last_result = 0;

static int is_schedulable(const PCB *p) {
    return p != NULL && p->state == STATE_READY && p->burst_time > 0;
}

static int compare_fcfs(const SchedulerWorkItem *a, const SchedulerWorkItem *b) {
    return a->arrival_order - b->arrival_order;
}

static int compare_sjf(const SchedulerWorkItem *a, const SchedulerWorkItem *b) {
    if (a->burst_time == b->burst_time) {
        return compare_fcfs(a, b);
    }
    return a->burst_time - b->burst_time;
}

static int compare_priority(const SchedulerWorkItem *a, const SchedulerWorkItem *b) {
    if (a->priority == b->priority) {
        return compare_fcfs(a, b);
    }
    return a->priority - b->priority;
}

static void sort_items(SchedulerWorkItem *list, int count, SchedulerType type) {
    int i, j;

    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            int cmp = 0;

            if (type == SCHED_FCFS) {
                cmp = compare_fcfs(&list[j], &list[j + 1]);
            } else if (type == SCHED_SJF) {
                cmp = compare_sjf(&list[j], &list[j + 1]);
            } else {
                cmp = compare_priority(&list[j], &list[j + 1]);
            }

            if (cmp > 0) {
                SchedulerWorkItem tmp = list[j];
                list[j] = list[j + 1];
                list[j + 1] = tmp;
            }
        }
    }
}

static void init_result(SchedulerType type, ScheduleResult *result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->type = type;
}

static void append_slice(ScheduleResult *result, int pid, int slice) {
    int max_slices;

    if (result == NULL) {
        return;
    }

    max_slices = (int)(sizeof(result->execution_pids) / sizeof(result->execution_pids[0]));
    if (result->slice_count < max_slices) {
        result->execution_pids[result->slice_count] = pid;
        result->execution_slices[result->slice_count] = slice;
        result->slice_count++;
    }
}

static void append_segment(ScheduleResult *result, int pid, int start_time, int end_time) {
    if (result == NULL) {
        return;
    }

    if (result->segment_count < MAX_SCHEDULE_SEGMENTS) {
        result->segments[result->segment_count].pid = pid;
        result->segments[result->segment_count].start_time = start_time;
        result->segments[result->segment_count].end_time = end_time;
        result->segment_count++;
    }
}

static ProcessScheduleMetric *find_metric_slot(ScheduleResult *result, int pid) {
    int i;

    if (result == NULL) {
        return NULL;
    }

    for (i = 0; i < result->metric_count; i++) {
        if (result->metrics[i].pid == pid) {
            return &result->metrics[i];
        }
    }

    if (result->metric_count < MAX_PROCESSES) {
        ProcessScheduleMetric *m = &result->metrics[result->metric_count++];
        memset(m, 0, sizeof(*m));
        m->pid = pid;
        return m;
    }

    return NULL;
}

static void capture_metric_from_item(ScheduleResult *result,
                                     const SchedulerWorkItem *item,
                                     int first_start_time,
                                     int completion_time) {
    ProcessScheduleMetric *m;

    if (result == NULL || item == NULL) {
        return;
    }

    m = find_metric_slot(result, item->pid);
    if (m == NULL) {
        return;
    }

    m->pid = item->pid;
    m->burst_time = item->burst_time;
    m->start_time = first_start_time;
    m->completion_time = completion_time;
    m->turnaround_time = completion_time; /* arrival assumed 0 in this simulator */
    m->waiting_time = m->turnaround_time - m->burst_time;
    m->was_scheduled = 1;
}

static void finalize_result_metrics(ScheduleResult *result,
                                    SchedulerWorkItem *ready,
                                    int ready_count,
                                    int current_time,
                                    int total_burst) {
    double total_wait = 0.0;
    double total_turn = 0.0;
    int i;

    if (result == NULL) {
        return;
    }

    result->process_count = ready_count;
    result->total_busy_time = total_burst;
    result->total_time = current_time;

    for (i = 0; i < ready_count; i++) {
        result->process_ids[i] = ready[i].pid;
    }

    for (i = 0; i < result->metric_count; i++) {
        total_wait += result->metrics[i].waiting_time;
        total_turn += result->metrics[i].turnaround_time;
    }

    result->average_waiting_time =
        (result->metric_count > 0) ? (total_wait / result->metric_count) : 0.0;

    result->average_turnaround_time =
        (result->metric_count > 0) ? (total_turn / result->metric_count) : 0.0;

    result->cpu_utilization =
        (current_time > 0) ? (((double)total_burst / (double)current_time) * 100.0) : 0.0;
}

static int collect_ready_items(SchedulerWorkItem *items, int max_items) {
    PCB *table;
    int process_count;
    int ready_count = 0;
    int i;

    if (items == NULL || max_items <= 0) {
        return 0;
    }

    table = get_processes();
    process_count = get_process_count();

    for (i = 0; i < process_count && ready_count < max_items; i++) {
        if (is_schedulable(&table[i])) {
            items[ready_count].pid = table[i].pid;
            items[ready_count].burst_time = table[i].burst_time;
            items[ready_count].remaining_time = table[i].burst_time;
            items[ready_count].priority = table[i].priority;
            items[ready_count].arrival_order = table[i].arrival_order;
            ready_count++;
        }
    }

    return ready_count;
}

static void compute_schedule(SchedulerType type,
                             int quantum,
                             SchedulerWorkItem *ready,
                             int ready_count,
                             ScheduleResult *result) {
    int current_time = 0;
    int total_burst = 0;
    int i;

    if (result == NULL) {
        return;
    }

    init_result(type, result);
    result->process_count = ready_count;

    if (ready_count == 0) {
        return;
    }

    for (i = 0; i < ready_count; i++) {
        total_burst += ready[i].burst_time;
    }

    if (type == SCHED_RR) {
        int queue[MAX_PROCESSES];
        int head = 0;
        int tail = 0;
        int count = 0;
        int first_start_time[MAX_PROCESSES];

        if (quantum <= 0) {
            quantum = RR_QUANTUM_DEFAULT;
        }

        for (i = 0; i < ready_count; i++) {
            queue[tail] = i;
            tail = (tail + 1) % MAX_PROCESSES;
            count++;
            first_start_time[i] = -1;
        }

        while (count > 0) {
            int idx = queue[head];
            SchedulerWorkItem *p;
            int start_time;
            int slice;
            int end_time;

            head = (head + 1) % MAX_PROCESSES;
            count--;

            p = &ready[idx];
            if (p->remaining_time <= 0) {
                continue;
            }

            start_time = current_time;
            if (first_start_time[idx] < 0) {
                first_start_time[idx] = start_time;
            }

            slice = (p->remaining_time < quantum) ? p->remaining_time : quantum;
            end_time = start_time + slice;

            append_slice(result, p->pid, slice);
            append_segment(result, p->pid, start_time, end_time);

            current_time = end_time;
            p->remaining_time -= slice;

            if (p->remaining_time > 0) {
                queue[tail] = idx;
                tail = (tail + 1) % MAX_PROCESSES;
                count++;
            } else {
                capture_metric_from_item(result, p, first_start_time[idx], current_time);
            }
        }
    } else {
        sort_items(ready, ready_count, type);

        for (i = 0; i < ready_count; i++) {
            SchedulerWorkItem *p = &ready[i];
            int start_time = current_time;
            int end_time = start_time + p->burst_time;

            append_slice(result, p->pid, p->burst_time);
            append_segment(result, p->pid, start_time, end_time);

            current_time = end_time;
            p->remaining_time = 0;

            capture_metric_from_item(result, p, start_time, end_time);
        }
    }

    finalize_result_metrics(result, ready, ready_count, current_time, total_burst);
}

static void apply_result_to_process_table(const ScheduleResult *result) {
    int i;

    if (result == NULL) {
        return;
    }

    for (i = 0; i < result->metric_count; i++) {
        const ProcessScheduleMetric *m = &result->metrics[i];
        PCB *p = find_process(m->pid);

        if (p == NULL || !m->was_scheduled || p->state == STATE_TERMINATED) {
            continue;
        }

        p->state = STATE_RUNNING;
        p->waiting_time = m->waiting_time;
        p->turnaround_time = m->turnaround_time;
        p->completion_time = m->completion_time;
        p->remaining_time = 0;
        terminate_process(p->pid);
    }
}

void run_scheduler_preview(SchedulerType type, int quantum, ScheduleResult *result) {
    SchedulerWorkItem ready[MAX_PROCESSES];
    int ready_count;

    if (result == NULL) {
        return;
    }

    ready_count = collect_ready_items(ready, MAX_PROCESSES);
    compute_schedule(type, quantum, ready, ready_count, result);
}

void run_scheduler(SchedulerType type, int quantum, ScheduleResult *result) {
    SchedulerWorkItem ready[MAX_PROCESSES];
    int ready_count;

    if (result == NULL) {
        return;
    }

    reset_all_process_metrics();
    ready_count = collect_ready_items(ready, MAX_PROCESSES);
    compute_schedule(type, quantum, ready, ready_count, result);

    if (ready_count == 0) {
        log_event("Scheduler executed: %s | no READY processes available",
                  scheduler_to_string(type));
        scheduler_set_last_result(result);
        return;
    }

    apply_result_to_process_table(result);

    log_event("Scheduler executed: %s | processes=%d | avg_wait=%.2f | avg_turn=%.2f | cpu=%.2f%%",
              scheduler_to_string(type),
              ready_count,
              result->average_waiting_time,
              result->average_turnaround_time,
              result->cpu_utilization);

    scheduler_set_last_result(result);
}

void schedule_result_to_string(const ScheduleResult *result, char *buffer, size_t size) {
    size_t used = 0;
    int i;

    if (buffer == NULL || size == 0 || result == NULL) {
        return;
    }

    buffer[0] = '\0';

    used += snprintf(buffer + used, size - used,
                     "Scheduler: %s\n"
                     "Average Waiting Time: %.2f\n"
                     "Average Turnaround Time: %.2f\n"
                     "CPU Utilization: %.2f%%\n",
                     scheduler_to_string(result->type),
                     result->average_waiting_time,
                     result->average_turnaround_time,
                     result->cpu_utilization);

    if (result->process_count == 0) {
        snprintf(buffer + used, size - used,
                 "\nNo READY processes were available for scheduling.\n");
        return;
    }

    used += snprintf(buffer + used, size - used, "\nExecution Order:\n");
    for (i = 0; i < result->slice_count && used < size; i++) {
        used += snprintf(buffer + used, size - used,
                         "[%d] PID %d ran for %d unit(s)\n",
                         i + 1,
                         result->execution_pids[i],
                         result->execution_slices[i]);
    }

    used += snprintf(buffer + used, size - used,
                     "\nPer-Process Metrics:\n"
                     "PID   Burst   Start   Completion   Turnaround   Waiting\n"
                     "--------------------------------------------------------\n");

    for (i = 0; i < result->metric_count && used < size; i++) {
        const ProcessScheduleMetric *m = &result->metrics[i];
        if (m->was_scheduled) {
            used += snprintf(buffer + used, size - used,
                             "%-5d %-7d %-7d %-12d %-12d %-8d\n",
                             m->pid,
                             m->burst_time,
                             m->start_time,
                             m->completion_time,
                             m->turnaround_time,
                             m->waiting_time);
        }
    }

    used += snprintf(buffer + used, size - used, "\nGantt Sequence:\n");
    for (i = 0; i < result->segment_count && used < size; i++) {
        used += snprintf(buffer + used, size - used,
                         "PID %d : %d -> %d\n",
                         result->segments[i].pid,
                         result->segments[i].start_time,
                         result->segments[i].end_time);
    }
}

void schedule_result_computation_to_string(const ScheduleResult *result, char *buffer, size_t size) {
    size_t used = 0;
    double total_wait = 0.0;
    double total_turn = 0.0;
    int i;

    if (buffer == NULL || size == 0 || result == NULL) {
        return;
    }

    buffer[0] = '\0';

    used += snprintf(buffer + used, size - used,
                     "Scheduling Algorithm: %s\n\n",
                     scheduler_to_string(result->type));

    if (result->process_count == 0) {
        snprintf(buffer + used, size - used,
                 "No READY processes were available for scheduling.\n");
        return;
    }

    used += snprintf(buffer + used, size - used,
                     "Execution Slices:\n");
    for (i = 0; i < result->segment_count && used < size; i++) {
        used += snprintf(buffer + used, size - used,
                         "Step %d: PID %d executed from time %d to %d (duration = %d)\n",
                         i + 1,
                         result->segments[i].pid,
                         result->segments[i].start_time,
                         result->segments[i].end_time,
                         result->segments[i].end_time - result->segments[i].start_time);
    }

    used += snprintf(buffer + used, size - used,
                     "\nPer Process Computation:\n"
                     "PID  Burst  Start  Completion  Turnaround  Waiting\n"
                     "---------------------------------------------------\n");

    for (i = 0; i < result->metric_count && used < size; i++) {
        const ProcessScheduleMetric *m = &result->metrics[i];
        if (!m->was_scheduled) {
            continue;
        }

        total_wait += m->waiting_time;
        total_turn += m->turnaround_time;

        used += snprintf(buffer + used, size - used,
                         "%-4d %-6d %-6d %-11d %-11d %-8d\n",
                         m->pid,
                         m->burst_time,
                         m->start_time,
                         m->completion_time,
                         m->turnaround_time,
                         m->waiting_time);
    }

    used += snprintf(buffer + used, size - used,
                     "\nFormulae Used:\n"
                     "Turnaround Time = Completion Time - Arrival Time\n"
                     "Waiting Time    = Turnaround Time - Burst Time\n"
                     "CPU Utilization = (Busy Time / Total Time) * 100\n");

    used += snprintf(buffer + used, size - used,
                     "\nAverage Waiting Time = %.2f / %d = %.2f\n",
                     total_wait,
                     (result->metric_count > 0 ? result->metric_count : 1),
                     result->average_waiting_time);

    used += snprintf(buffer + used, size - used,
                     "Average Turnaround Time = %.2f / %d = %.2f\n",
                     total_turn,
                     (result->metric_count > 0 ? result->metric_count : 1),
                     result->average_turnaround_time);

    used += snprintf(buffer + used, size - used,
                     "CPU Utilization = (%d / %d) * 100 = %.2f%%\n",
                     result->total_busy_time,
                     result->total_time,
                     result->cpu_utilization);
}

const char *scheduler_to_string(SchedulerType type) {
    switch (type) {
        case SCHED_FCFS:
            return "FCFS";
        case SCHED_SJF:
            return "SJF";
        case SCHED_PRIORITY:
            return "Priority Scheduling";
        case SCHED_RR:
            return "Round Robin";
        default:
            return "Unknown";
    }
}

void scheduler_set_last_result(const ScheduleResult *result) {
    if (result == NULL) {
        memset(&g_last_result, 0, sizeof(g_last_result));
        g_has_last_result = 0;
        return;
    }

    memcpy(&g_last_result, result, sizeof(ScheduleResult));
    g_has_last_result = 1;
}

const ScheduleResult *scheduler_get_last_result(void) {
    if (!g_has_last_result) {
        return NULL;
    }
    return &g_last_result;
}

int scheduler_has_last_result(void) {
    return g_has_last_result;
}

int scheduler_copy_last_result(ScheduleResult *result_out) {
    if (!g_has_last_result || result_out == NULL) {
        return 0;
    }

    memcpy(result_out, &g_last_result, sizeof(ScheduleResult));
    return 1;
}
