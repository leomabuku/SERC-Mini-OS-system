#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESSES 32
#define TOTAL_MEMORY 1024
#define PAGE_SIZE 64
#define TOTAL_FRAMES (TOTAL_MEMORY / PAGE_SIZE)
#define MAX_PROCESS_PAGES TOTAL_FRAMES
#define MAX_SEGMENTS 64
#define MAX_LOG_LINE 512
#define IPC_QUEUE_SIZE 64
#define RESOURCE_TYPES 3
#define RR_QUANTUM_DEFAULT 2

typedef enum {
    STATE_NEW = 0,
    STATE_READY,
    STATE_RUNNING,
    STATE_WAITING,
    STATE_SUSPENDED,
    STATE_TERMINATED
} ProcessState;

static inline const char *state_to_string(ProcessState state) {
    switch (state) {
        case STATE_NEW: return "NEW";
        case STATE_READY: return "READY";
        case STATE_RUNNING: return "RUNNING";
        case STATE_WAITING: return "WAITING";
        case STATE_SUSPENDED: return "SUSPENDED";
        case STATE_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

#endif
