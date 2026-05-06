#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "process.h"
#include "memory.h"
#include "scheduler.h"

void serc_init(void);

int serc_add_task(const char *name,
                  const char *service_type,
                  int burst_time,
                  int priority,
                  int memory_required,
                  const int max_need[RESOURCE_TYPES],
                  MemoryStrategy strategy,
                  char *message,
                  size_t size);

int serc_suspend_task(int pid, char *message, size_t size);
int serc_resume_task(int pid, char *message, size_t size);
int serc_terminate_task(int pid, char *message, size_t size);

int serc_request_resources(int pid, const int request[RESOURCE_TYPES], char *message, size_t size);
void serc_release_resources(int pid, const int release[RESOURCE_TYPES], char *message, size_t size);

int serc_send_message(int from_pid, int to_pid, const char *text, char *message, size_t size);

void serc_run_scheduler(SchedulerType type, int quantum, char *message, size_t size);
void serc_compare_schedulers(int quantum, char *message, size_t size);
int serc_check_deadlock_safety(char *message, size_t size);

/* Demo / reports */
void serc_load_demo_data(char *message, size_t size);
void serc_full_status_report(char *buffer, size_t size);
void serc_logs_to_string(char *buffer, size_t size);
int serc_save_status_snapshot(char *saved_name, size_t saved_name_size, char *message, size_t size);
int serc_save_schedule_report(char *saved_name, size_t saved_name_size, char *message, size_t size);
int serc_list_data_files(char *buffer, size_t size);
int serc_read_data_file(const char *name, char *buffer, size_t size);

/* New: scheduling visualization + computation support */
void serc_get_last_schedule_summary(char *buffer, size_t size);
void serc_get_last_schedule_computation(char *buffer, size_t size);
int serc_has_last_schedule(void);
int serc_copy_last_schedule(ScheduleResult *result_out);

#endif
