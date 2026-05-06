#include "ipc.h"
#include "logger.h"
#include "process.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static IPCMessage queue[IPC_QUEUE_SIZE];
static int message_count = 0;

void ipc_init(void) {
    memset(queue, 0, sizeof(queue));
    message_count = 0;
}

int ipc_send(int from_pid, int to_pid, const char *message_text) {
    if (message_text == NULL || message_text[0] == '\0') {
        log_event("IPC send failed: empty message.");
        return 0;
    }

    if (message_count >= IPC_QUEUE_SIZE) {
        log_event("IPC send failed: queue full.");
        return 0;
    }

    PCB *from = find_process(from_pid);
    PCB *to = find_process(to_pid);
    if (from == NULL || to == NULL || from->state == STATE_TERMINATED || to->state == STATE_TERMINATED) {
        log_event("IPC send failed: invalid PID(s). from=%d to=%d", from_pid, to_pid);
        return 0;
    }

    queue[message_count].from_pid = from_pid;
    queue[message_count].to_pid = to_pid;
    snprintf(queue[message_count].message, sizeof(queue[message_count].message), "%s", message_text);
    queue[message_count].message[sizeof(queue[message_count].message) - 1] = '\0';
    queue[message_count].timestamp = time(NULL);
    message_count++;

    log_event("IPC message sent: from PID=%d to PID=%d | %s", from_pid, to_pid, message_text);
    return 1;
}

int ipc_get_message_count(void) {
    return message_count;
}

int ipc_copy_messages(IPCMessage *out, int max_messages) {
    int count;

    if (out == NULL || max_messages <= 0) {
        return 0;
    }

    count = message_count < max_messages ? message_count : max_messages;
    for (int i = 0; i < count; i++) {
        out[i] = queue[i];
    }

    return count;
}

void ipc_messages_to_string(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    size_t used = 0;
    buffer[0] = '\0';

    used += snprintf(buffer + used, size - used, "IPC Message Queue (%d message(s))\n", message_count);
    used += snprintf(buffer + used, size - used, "FROM  TO    TIME                 MESSAGE\n");
    used += snprintf(buffer + used, size - used, "---------------------------------------------------------------\n");

    for (int i = 0; i < message_count && used < size; i++) {
        char time_buf[64] = "N/A";
        struct tm *tm_info = localtime(&queue[i].timestamp);
        if (tm_info != NULL) {
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        }

        used += snprintf(buffer + used, size - used,
                         "%-5d %-5d %-20s %s\n",
                         queue[i].from_pid,
                         queue[i].to_pid,
                         time_buf,
                         queue[i].message);
    }

    if (message_count == 0 && used < size) {
        snprintf(buffer + used, size - used, "No IPC messages in queue.\n");
    }
}
