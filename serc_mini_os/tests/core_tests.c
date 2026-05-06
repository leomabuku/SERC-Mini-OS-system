#include "system_state.h"
#include "process.h"
#include "memory.h"
#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require_true(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void require_contains(const char *haystack, const char *needle) {
    char message[256];

    if (strstr(haystack, needle) != NULL) {
        return;
    }

    snprintf(message, sizeof(message), "expected report to contain '%s'", needle);
    require_true(0, message);
}

static void test_scheduler_comparison_is_non_mutating(void) {
    char message[32768];
    int active_before;

    serc_init();
    serc_load_demo_data(message, sizeof(message));
    active_before = get_active_process_count();
    require_true(active_before == 12, "demo data should create 12 active emergency tasks");

    memset(message, 0, sizeof(message));
    serc_compare_schedulers(RR_QUANTUM_DEFAULT, message, sizeof(message));

    require_contains(message, "FCFS");
    require_contains(message, "SJF");
    require_contains(message, "Priority Scheduling");
    require_contains(message, "Round Robin");
    require_contains(message, "Average Waiting Time");
    require_contains(message, "CPU Utilization");
    require_contains(message, "Best waiting time");
    require_true(get_active_process_count() == active_before,
                 "scheduler comparison must not terminate or mutate active processes");
}

static void test_deadlock_safety_check_is_visible(void) {
    char message[4096];

    serc_init();
    serc_load_demo_data(message, sizeof(message));

    memset(message, 0, sizeof(message));
    require_true(serc_check_deadlock_safety(message, sizeof(message)) == 1,
                 "demo resource allocation should be safe");
    require_contains(message, "Safe state");
    require_contains(message, "Safe sequence");
}

static void test_gantt_result_persists_after_scheduler_terminates_processes(void) {
    char message[32768];
    ScheduleResult last;

    serc_init();
    serc_load_demo_data(message, sizeof(message));

    memset(message, 0, sizeof(message));
    serc_run_scheduler(SCHED_SJF, RR_QUANTUM_DEFAULT, message, sizeof(message));

    require_true(get_active_process_count() == 0,
                 "scheduler run should terminate scheduled demo processes");
    require_true(serc_has_last_schedule() == 1,
                 "last schedule should remain available after process termination");
    require_true(serc_copy_last_schedule(&last) == 1,
                 "last schedule copy should succeed after process termination");
    require_true(last.segment_count == 12,
                 "persistent Gantt schedule should include all 12 demo segments");
}

static void test_paging_allocates_and_releases_frames(void) {
    char message[4096];
    int max_need[RESOURCE_TYPES] = {1, 1, 1};
    MemoryFrame frames[TOTAL_FRAMES];
    int frame_count;
    int pid;

    serc_init();
    require_true(serc_add_task("Paged Hotline", "AMBULANCE", 4, 1, 130,
                               max_need, MEM_PAGING, message, sizeof(message)) == 1,
                 "paging task should be created");

    pid = get_processes()[0].pid;
    require_true(get_processes()[0].is_paged == 1,
                 "PCB should mark paging allocation");
    require_true(get_processes()[0].page_count == 3,
                 "130 bytes should require 3 pages with 64-byte pages");
    require_true(get_processes()[0].internal_fragmentation == 62,
                 "paging should track internal fragmentation");
    require_true(get_paging_internal_fragmentation() == 62,
                 "global paging fragmentation should include the process waste");

    frame_count = copy_memory_frames(frames, TOTAL_FRAMES);
    require_true(frame_count == TOTAL_FRAMES, "frame snapshot should include every frame");
    require_true(frames[0].pid == pid && frames[0].page_number == 0,
                 "first allocated frame should map process page 0");
    require_true(frames[1].pid == pid && frames[1].page_number == 1,
                 "second allocated frame should map process page 1");
    require_true(frames[2].pid == pid && frames[2].page_number == 2,
                 "third allocated frame should map process page 2");

    serc_terminate_task(pid, message, sizeof(message));
    require_true(get_paging_internal_fragmentation() == 0,
                 "terminating a paged process should release paging fragmentation");
}

static void test_file_manager_writes_lists_and_reads_reports(void) {
    char message[32768];
    char listing[32768];
    char preview[32768];
    char saved_name[260];

    serc_init();
    serc_load_demo_data(message, sizeof(message));
    serc_compare_schedulers(RR_QUANTUM_DEFAULT, message, sizeof(message));

    memset(saved_name, 0, sizeof(saved_name));
    require_true(serc_save_status_snapshot(saved_name, sizeof(saved_name), message, sizeof(message)) == 1,
                 "status snapshot should be saved to data folder");
    require_contains(saved_name, "status_");

    memset(saved_name, 0, sizeof(saved_name));
    require_true(serc_save_schedule_report(saved_name, sizeof(saved_name), message, sizeof(message)) == 1,
                 "schedule report should be saved to data folder");
    require_contains(saved_name, "schedule_");

    memset(listing, 0, sizeof(listing));
    require_true(serc_list_data_files(listing, sizeof(listing)) >= 2,
                 "data file listing should include saved files");
    require_contains(listing, "status_");
    require_contains(listing, "schedule_");

    memset(preview, 0, sizeof(preview));
    require_true(serc_read_data_file(saved_name, preview, sizeof(preview)) == 1,
                 "saved schedule report should be readable");
    require_contains(preview, "Scheduling Algorithm Comparison");
}

int main(void) {
    test_scheduler_comparison_is_non_mutating();
    test_deadlock_safety_check_is_visible();
    test_gantt_result_persists_after_scheduler_terminates_processes();
    test_paging_allocates_and_releases_frames();
    test_file_manager_writes_lists_and_reads_reports();
    printf("core_tests: all tests passed\n");
    return 0;
}
