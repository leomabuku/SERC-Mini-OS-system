#include "system_state.h"
#include "logger.h"
#include "deadlock.h"
#include "ipc.h"
#include "file_manager.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static char g_last_comparison_report[32768];

static void append_to_buffer(char *buffer, size_t size, size_t *used, const char *fmt, ...) {
    va_list args;
    int written;

    if (buffer == NULL || size == 0 || used == NULL || *used >= size) {
        return;
    }

    va_start(args, fmt);
    written = vsnprintf(buffer + *used, size - *used, fmt, args);
    va_end(args);

    if (written < 0) {
        return;
    }

    if ((size_t)written >= size - *used) {
        *used = size - 1;
    } else {
        *used += (size_t)written;
    }
}

static void reset_core_state_only(void) {
    process_table_init();
    memory_init();
    deadlock_init();
    ipc_init();
    scheduler_set_last_result(NULL);
    g_last_comparison_report[0] = '\0';
}

void serc_init(void) {
    reset_core_state_only();
    logger_init();
    file_manager_init();
    log_event("SERC Mini-OS initialized.");
}

int serc_add_task(const char *name,
                  const char *service_type,
                  int burst_time,
                  int priority,
                  int memory_required,
                  const int max_need[RESOURCE_TYPES],
                  MemoryStrategy strategy,
                  char *message,
                  size_t size) {
    int pid = create_process(name, service_type, burst_time, priority, memory_required, max_need);
    if (pid < 0) {
        snprintf(message, size, "Failed to create task. Check process count or input values.");
        return 0;
    }

    {
        int start = allocate_memory(pid, memory_required, strategy);
        if (start < 0) {
            terminate_process(pid);
            snprintf(message, size, "Task created but memory allocation failed. Task rolled back.");
            return 0;
        }

        snprintf(message, size,
                 "Task created successfully. PID=%d, memory start=%d, strategy=%s",
                 pid, start, memory_strategy_to_string(strategy));
    }

    return 1;
}

int serc_suspend_task(int pid, char *message, size_t size) {
    if (suspend_process(pid)) {
        snprintf(message, size, "PID %d suspended.", pid);
        return 1;
    }

    snprintf(message, size, "Could not suspend PID %d.", pid);
    return 0;
}

int serc_resume_task(int pid, char *message, size_t size) {
    if (resume_process(pid)) {
        snprintf(message, size, "PID %d resumed.", pid);
        return 1;
    }

    snprintf(message, size, "Could not resume PID %d.", pid);
    return 0;
}

int serc_terminate_task(int pid, char *message, size_t size) {
    int release_all[RESOURCE_TYPES] = {999, 999, 999};

    serc_release_resources(pid, release_all, message, size);

    if (terminate_process(pid)) {
        snprintf(message, size, "PID %d terminated and memory/resources released.", pid);
        return 1;
    }

    snprintf(message, size, "Could not terminate PID %d.", pid);
    return 0;
}

int serc_request_resources(int pid, const int request[RESOURCE_TYPES], char *message, size_t size) {
    return request_resources_for_process(pid, request, message, size);
}

void serc_release_resources(int pid, const int release[RESOURCE_TYPES], char *message, size_t size) {
    release_resources_for_process(pid, release, message, size);
}

int serc_send_message(int from_pid, int to_pid, const char *text, char *message, size_t size) {
    if (ipc_send(from_pid, to_pid, text)) {
        snprintf(message, size, "IPC message sent from PID %d to PID %d.", from_pid, to_pid);
        return 1;
    }

    snprintf(message, size, "Failed to send IPC message. Check PIDs.");
    return 0;
}

void serc_run_scheduler(SchedulerType type, int quantum, char *message, size_t size) {
    ScheduleResult result;

    memset(&result, 0, sizeof(result));
    run_scheduler(type, quantum, &result);
    schedule_result_to_string(&result, message, size);
}

void serc_compare_schedulers(int quantum, char *message, size_t size) {
    SchedulerType types[] = {SCHED_FCFS, SCHED_SJF, SCHED_PRIORITY, SCHED_RR};
    ScheduleResult results[4];
    int result_count = (int)(sizeof(results) / sizeof(results[0]));
    int best_wait_index = -1;
    int best_turn_index = -1;
    size_t used = 0;
    int i;

    if (message == NULL || size == 0) {
        return;
    }

    message[0] = '\0';

    if (quantum <= 0) {
        quantum = RR_QUANTUM_DEFAULT;
    }

    for (i = 0; i < result_count; i++) {
        memset(&results[i], 0, sizeof(results[i]));
        run_scheduler_preview(types[i], quantum, &results[i]);

        if (results[i].process_count == 0) {
            continue;
        }

        if (best_wait_index < 0 ||
            results[i].average_waiting_time < results[best_wait_index].average_waiting_time) {
            best_wait_index = i;
        }

        if (best_turn_index < 0 ||
            results[i].average_turnaround_time < results[best_turn_index].average_turnaround_time) {
            best_turn_index = i;
        }
    }

    append_to_buffer(message, size, &used,
                     "=== Scheduling Algorithm Comparison ===\n"
                     "Non-mutating preview: processes remain READY so algorithms can be compared fairly.\n"
                     "Round Robin quantum: %d\n\n",
                     quantum);

    append_to_buffer(message, size, &used,
                     "Algorithm              Tasks  Average Waiting Time  Average Turnaround Time  CPU Utilization  Total Time  Segments\n"
                     "------------------------------------------------------------------------------------------------------------------\n");

    for (i = 0; i < result_count; i++) {
        append_to_buffer(message, size, &used,
                         "%-22s %-6d %-21.2f %-24.2f %-16.2f %-11d %-8d\n",
                         scheduler_to_string(results[i].type),
                         results[i].process_count,
                         results[i].average_waiting_time,
                         results[i].average_turnaround_time,
                         results[i].cpu_utilization,
                         results[i].total_time,
                         results[i].segment_count);
    }

    if (best_wait_index < 0 || best_turn_index < 0) {
        append_to_buffer(message, size, &used,
                         "\nNo READY processes were available. Load demo data or create tasks before comparing.\n");
        scheduler_set_last_result(NULL);
        log_event("Scheduler comparison requested with no READY processes.");
        return;
    }

    append_to_buffer(message, size, &used,
                     "\nBest waiting time: %s (%.2f)\n"
                     "Best turnaround time: %s (%.2f)\n",
                     scheduler_to_string(results[best_wait_index].type),
                     results[best_wait_index].average_waiting_time,
                     scheduler_to_string(results[best_turn_index].type),
                     results[best_turn_index].average_turnaround_time);

    append_to_buffer(message, size, &used,
                     "\nGantt Preview for %s:\n",
                     scheduler_to_string(results[best_wait_index].type));
    for (i = 0; i < results[best_wait_index].segment_count; i++) {
        ScheduleSegment seg = results[best_wait_index].segments[i];
        append_to_buffer(message, size, &used,
                         "PID %d : %d -> %d\n",
                         seg.pid,
                         seg.start_time,
                         seg.end_time);
    }

    scheduler_set_last_result(&results[best_wait_index]);
    snprintf(g_last_comparison_report, sizeof(g_last_comparison_report), "%s", message);
    log_event("Scheduler comparison generated: best_wait=%s avg_wait=%.2f, best_turn=%s avg_turn=%.2f",
              scheduler_to_string(results[best_wait_index].type),
              results[best_wait_index].average_waiting_time,
              scheduler_to_string(results[best_turn_index].type),
              results[best_turn_index].average_turnaround_time);
}

int serc_check_deadlock_safety(char *message, size_t size) {
    int safe;

    if (message == NULL || size == 0) {
        return 0;
    }

    safe = bankers_is_safe(message, size);
    log_event("Deadlock safety check: %s", safe ? "SAFE" : "UNSAFE");
    return safe;
}

void serc_load_demo_data(char *message, size_t size) {
    char temp[256];

    int t1[RESOURCE_TYPES]  = {2, 1, 2};
    int t2[RESOURCE_TYPES]  = {1, 2, 1};
    int t3[RESOURCE_TYPES]  = {2, 2, 1};
    int t4[RESOURCE_TYPES]  = {1, 1, 3};
    int t5[RESOURCE_TYPES]  = {2, 1, 1};
    int t6[RESOURCE_TYPES]  = {1, 1, 2};
    int t7[RESOURCE_TYPES]  = {3, 1, 1};
    int t8[RESOURCE_TYPES]  = {1, 2, 2};
    int t9[RESOURCE_TYPES]  = {2, 1, 2};
    int t10[RESOURCE_TYPES] = {1, 1, 1};
    int t11[RESOURCE_TYPES] = {2, 2, 2};
    int t12[RESOURCE_TYPES] = {1, 1, 2};

    int req1[RESOURCE_TYPES] = {1, 1, 1};
    int req2[RESOURCE_TYPES] = {1, 0, 1};
    int req3[RESOURCE_TYPES] = {0, 1, 1};
    int req4[RESOURCE_TYPES] = {1, 1, 0};

    reset_core_state_only();
    log_event("Demo dataset reset and loading started.");

    serc_add_task("Ambulance Dispatch",     "AMBULANCE", 4, 1,  70, t1,  MEM_FIRST_FIT, temp, sizeof(temp));
    serc_add_task("Fire Alert",             "FIRE",      6, 2,  90, t2,  MEM_BEST_FIT,  temp, sizeof(temp));
    serc_add_task("Police Patrol",          "POLICE",    3, 3,  60, t3,  MEM_WORST_FIT, temp, sizeof(temp));
    serc_add_task("Traffic Control",        "POLICE",    5, 2,  85, t4,  MEM_FIRST_FIT, temp, sizeof(temp));
    serc_add_task("Rescue Coordination",    "AMBULANCE", 7, 1,  95, t5,  MEM_BEST_FIT,  temp, sizeof(temp));
    serc_add_task("Fire Station Relay",     "FIRE",      5, 2,  70, t6,  MEM_FIRST_FIT, temp, sizeof(temp));
    serc_add_task("Checkpoint Monitoring",  "POLICE",    8, 3,  90, t7,  MEM_WORST_FIT, temp, sizeof(temp));
    serc_add_task("Medical Supply Run",     "AMBULANCE", 4, 2,  55, t8,  MEM_BEST_FIT,  temp, sizeof(temp));
    serc_add_task("Incident Logging",       "POLICE",    2, 4,  50, t9,  MEM_FIRST_FIT, temp, sizeof(temp));
    serc_add_task("Hazmat Response",        "FIRE",      6, 1, 100, t10, MEM_WORST_FIT, temp, sizeof(temp));
    serc_add_task("Emergency Hotline",      "AMBULANCE", 3, 2,  60, t11, MEM_FIRST_FIT, temp, sizeof(temp));
    serc_add_task("Crowd Management",       "POLICE",    5, 3,  75, t12, MEM_BEST_FIT,  temp, sizeof(temp));

    serc_request_resources(1,  req1, temp, sizeof(temp));
    serc_request_resources(2,  req2, temp, sizeof(temp));
    serc_request_resources(5,  req3, temp, sizeof(temp));
    serc_request_resources(10, req4, temp, sizeof(temp));

    serc_send_message(1,  4,  "Redirect traffic around accident zone.", temp, sizeof(temp));
    serc_send_message(2,  10, "Fire suppression team ready for deployment.", temp, sizeof(temp));
    serc_send_message(5,  8,  "Medical support requested at assembly point.", temp, sizeof(temp));
    serc_send_message(11, 3,  "Caller details shared with nearest patrol unit.", temp, sizeof(temp));

    snprintf(message, size,
             "Extended demo data loaded successfully: 12 emergency tasks, 4 resource allocations, and 4 IPC messages created.");
    log_event("Extended demo dataset loaded successfully.");
}

void serc_full_status_report(char *buffer, size_t size) {
    char proc[4096];
    char mem[2048];
    char res[512];
    char ipc[2048];

    process_table_to_string(proc, sizeof(proc));
    memory_map_to_string(mem, sizeof(mem));
    resource_status_to_string(res, sizeof(res));
    ipc_messages_to_string(ipc, sizeof(ipc));

    snprintf(buffer, size,
             "=== SERC MINI-OS STATUS REPORT ===\n\n%s\n%s\n%s\n%s",
             proc, mem, res, ipc);
}

void serc_logs_to_string(char *buffer, size_t size) {
    read_log_file(buffer, size);
}

int serc_save_status_snapshot(char *saved_name, size_t saved_name_size, char *message, size_t size) {
    char report[32768];
    int ok;

    serc_full_status_report(report, sizeof(report));
    ok = file_manager_save_text("status", "txt", report, saved_name, saved_name_size);
    if (message != NULL && size > 0) {
        snprintf(message, size, ok ? "Status snapshot saved: %s" : "%s", saved_name);
    }
    return ok;
}

int serc_save_schedule_report(char *saved_name, size_t saved_name_size, char *message, size_t size) {
    char report[32768];
    int ok;

    if (g_last_comparison_report[0] != '\0') {
        snprintf(report, sizeof(report), "%s", g_last_comparison_report);
    } else {
        serc_get_last_schedule_summary(report, sizeof(report));
    }

    ok = file_manager_save_text("schedule", "txt", report, saved_name, saved_name_size);
    if (message != NULL && size > 0) {
        snprintf(message, size, ok ? "Schedule report saved: %s" : "%s", saved_name);
    }
    return ok;
}

int serc_list_data_files(char *buffer, size_t size) {
    return file_manager_list_files(buffer, size);
}

int serc_read_data_file(const char *name, char *buffer, size_t size) {
    return file_manager_read_file(name, buffer, size);
}

void serc_get_last_schedule_summary(char *buffer, size_t size) {
    const ScheduleResult *last = scheduler_get_last_result();

    if (buffer == NULL || size == 0) {
        return;
    }

    if (last == NULL) {
        snprintf(buffer, size, "No scheduling run has been executed yet.");
        return;
    }

    schedule_result_to_string(last, buffer, size);
}

void serc_get_last_schedule_computation(char *buffer, size_t size) {
    const ScheduleResult *last = scheduler_get_last_result();

    if (buffer == NULL || size == 0) {
        return;
    }

    if (last == NULL) {
        snprintf(buffer, size, "No scheduling run has been executed yet.");
        return;
    }

    schedule_result_computation_to_string(last, buffer, size);
}

int serc_has_last_schedule(void) {
    return scheduler_has_last_result();
}

int serc_copy_last_schedule(ScheduleResult *result_out) {
    return scheduler_copy_last_result(result_out);
}
