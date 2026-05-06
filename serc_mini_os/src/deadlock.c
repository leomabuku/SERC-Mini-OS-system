#include "deadlock.h"
#include "logger.h"
#include "process.h"

#include <stdio.h>
#include <string.h>

static int total_resources[RESOURCE_TYPES] = {8, 6, 10};
static int available[RESOURCE_TYPES] = {8, 6, 10};

void deadlock_init(void) {
    deadlock_reset_available();
}

void deadlock_reset_available(void) {
    for (int i = 0; i < RESOURCE_TYPES; i++) {
        available[i] = total_resources[i];
    }
}

static void recalculate_available(void) {
    deadlock_reset_available();

    PCB *table = get_processes();
    int count = get_process_count();

    for (int i = 0; i < count; i++) {
        if (table[i].state != STATE_TERMINATED) {
            for (int r = 0; r < RESOURCE_TYPES; r++) {
                available[r] -= table[i].allocation[r];
                if (available[r] < 0) {
                    available[r] = 0;
                }
            }
        }
    }
}

void resource_status_to_string(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    recalculate_available();

    snprintf(buffer, size,
             "Resources\n"
             "Communication Channels: total=%d available=%d\n"
             "Vehicles: total=%d available=%d\n"
             "Staff Units: total=%d available=%d\n",
             total_resources[0], available[0],
             total_resources[1], available[1],
             total_resources[2], available[2]);
}

void copy_resource_snapshot(int total_out[RESOURCE_TYPES],
                            int available_out[RESOURCE_TYPES],
                            int allocated_out[RESOURCE_TYPES]) {
    recalculate_available();

    for (int r = 0; r < RESOURCE_TYPES; r++) {
        if (total_out != NULL) {
            total_out[r] = total_resources[r];
        }
        if (available_out != NULL) {
            available_out[r] = available[r];
        }
        if (allocated_out != NULL) {
            allocated_out[r] = total_resources[r] - available[r];
        }
    }
}

int bankers_is_safe(char *message, size_t size) {
    PCB *table = get_processes();
    int n = get_process_count();
    int work[RESOURCE_TYPES];
    int finish[MAX_PROCESSES] = {0};
    int safe_sequence[MAX_PROCESSES];
    int safe_count = 0;

    if (message == NULL || size == 0) {
        return 0;
    }

    recalculate_available();

    for (int r = 0; r < RESOURCE_TYPES; r++) {
        work[r] = available[r];
    }

    while (1) {
        int found = 0;

        for (int i = 0; i < n; i++) {
            if (table[i].state == STATE_TERMINATED) {
                finish[i] = 1;
                continue;
            }

            if (finish[i]) {
                continue;
            }

            int can_finish = 1;
            for (int r = 0; r < RESOURCE_TYPES; r++) {
                int need = table[i].max_need[r] - table[i].allocation[r];
                if (need > work[r]) {
                    can_finish = 0;
                    break;
                }
            }

            if (can_finish) {
                for (int r = 0; r < RESOURCE_TYPES; r++) {
                    work[r] += table[i].allocation[r];
                }
                finish[i] = 1;
                safe_sequence[safe_count++] = table[i].pid;
                found = 1;
            }
        }

        if (!found) {
            break;
        }
    }

    for (int i = 0; i < n; i++) {
        if (!finish[i]) {
            snprintf(message, size, "Unsafe state detected. Request denied to avoid deadlock.");
            return 0;
        }
    }

    size_t used = 0;
    used += snprintf(message + used, size - used, "Safe state. Safe sequence: ");
    for (int i = 0; i < safe_count && used < size; i++) {
        used += snprintf(message + used, size - used, "P%d%s",
                         safe_sequence[i], (i == safe_count - 1) ? "" : " -> ");
    }

    return 1;
}

int request_resources_for_process(int pid, const int request[RESOURCE_TYPES], char *message, size_t size) {
    if (message == NULL || size == 0) {
        return 0;
    }
    if (request == NULL) {
        snprintf(message, size, "Invalid request.");
        return 0;
    }

    PCB *p = find_process(pid);
    if (p == NULL || p->state == STATE_TERMINATED) {
        snprintf(message, size, "Process not found or terminated.");
        return 0;
    }

    recalculate_available();

    for (int r = 0; r < RESOURCE_TYPES; r++) {
        int need = p->max_need[r] - p->allocation[r];
        if (request[r] < 0) {
            snprintf(message, size, "Negative resource request is invalid.");
            return 0;
        }
        if (request[r] > need) {
            snprintf(message, size, "Request exceeds declared maximum need.");
            return 0;
        }
        if (request[r] > available[r]) {
            p->state = STATE_WAITING;
            snprintf(message, size, "Resources unavailable now. Process moved to WAITING state.");
            log_event("Resource request delayed: PID=%d request=(%d,%d,%d)", pid, request[0], request[1], request[2]);
            return 0;
        }
    }

    for (int r = 0; r < RESOURCE_TYPES; r++) {
        p->allocation[r] += request[r];
        available[r] -= request[r];
    }

    char safe_msg[256];
    memset(safe_msg, 0, sizeof(safe_msg));
    if (!bankers_is_safe(safe_msg, sizeof(safe_msg))) {
        for (int r = 0; r < RESOURCE_TYPES; r++) {
            p->allocation[r] -= request[r];
            available[r] += request[r];
        }
        snprintf(message, size, "%s", safe_msg);
        log_event("Banker's request denied: PID=%d request=(%d,%d,%d)", pid, request[0], request[1], request[2]);
        return 0;
    }

    if (p->state != STATE_TERMINATED && p->state != STATE_SUSPENDED) {
        p->state = STATE_READY;
    }

    snprintf(message, size, "Resource request granted. %s", safe_msg);
    log_event("Resources granted: PID=%d request=(%d,%d,%d)", pid, request[0], request[1], request[2]);
    return 1;
}

void release_resources_for_process(int pid, const int release[RESOURCE_TYPES], char *message, size_t size) {
    if (message == NULL || size == 0) {
        return;
    }

    PCB *p = find_process(pid);
    if (p == NULL) {
        snprintf(message, size, "Process not found.");
        return;
    }
    if (release == NULL) {
        snprintf(message, size, "Invalid release request.");
        return;
    }

    for (int r = 0; r < RESOURCE_TYPES; r++) {
        int actual_release = release[r];
        if (actual_release < 0) {
            actual_release = 0;
        }
        if (actual_release > p->allocation[r]) {
            actual_release = p->allocation[r];
        }
        p->allocation[r] -= actual_release;
    }

    recalculate_available();

    if (p->state == STATE_WAITING) {
        p->state = STATE_READY;
    }

    snprintf(message, size, "Resources released for PID %d.", pid);
    log_event("Resources released: PID=%d release=(%d,%d,%d)", pid, release[0], release[1], release[2]);
}
