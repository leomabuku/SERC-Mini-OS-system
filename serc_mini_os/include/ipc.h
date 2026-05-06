#ifndef IPC_H
#define IPC_H

#include "common.h"

typedef struct {
    int from_pid;
    int to_pid;
    char message[128];
    time_t timestamp;
} IPCMessage;

void ipc_init(void);
int ipc_send(int from_pid, int to_pid, const char *message_text);
int ipc_get_message_count(void);
int ipc_copy_messages(IPCMessage *out, int max_messages);
void ipc_messages_to_string(char *buffer, size_t size);

#endif
