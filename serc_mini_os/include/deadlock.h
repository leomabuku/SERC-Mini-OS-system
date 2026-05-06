#ifndef DEADLOCK_H
#define DEADLOCK_H

#include "process.h"

void deadlock_init(void);
void deadlock_reset_available(void);
void resource_status_to_string(char *buffer, size_t size);
void copy_resource_snapshot(int total_out[RESOURCE_TYPES],
                            int available_out[RESOURCE_TYPES],
                            int allocated_out[RESOURCE_TYPES]);
int request_resources_for_process(int pid, const int request[RESOURCE_TYPES], char *message, size_t size);
void release_resources_for_process(int pid, const int release[RESOURCE_TYPES], char *message, size_t size);
int bankers_is_safe(char *message, size_t size);

#endif
