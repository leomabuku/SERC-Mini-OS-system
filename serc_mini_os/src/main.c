#include "system_state.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void read_line(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    if (fgets(buffer, (int)size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

static int read_int(const char *prompt) {
    char line[64];
    printf("%s", prompt);
    read_line(line, sizeof(line));
    return atoi(line);
}

static void uppercase_in_place(char *s) {
    if (s == NULL) return;
    for (int i = 0; s[i] != '\0'; i++) {
        s[i] = (char)toupper((unsigned char)s[i]);
    }
}

static void pause_console(void) {
    char line[8];
    printf("\nPress Enter to continue...");
    read_line(line, sizeof(line));
}

static void menu(void) {
    printf("\n===== SERC MINI-OS MENU =====\n");
    printf("1. Load demo data\n");
    printf("2. Create task\n");
    printf("3. Suspend task\n");
    printf("4. Resume task\n");
    printf("5. Terminate task\n");
    printf("6. Run FCFS\n");
    printf("7. Run SJF\n");
    printf("8. Run Priority Scheduling\n");
    printf("9. Run Round Robin\n");
    printf("10. Compare all schedulers\n");
    printf("11. Request resources\n");
    printf("12. Release resources\n");
    printf("13. Send IPC message\n");
    printf("14. Check deadlock safety\n");
    printf("15. View status\n");
    printf("16. View logs\n");
    printf("17. Save status snapshot\n");
    printf("18. Save schedule report\n");
    printf("19. List saved data files\n");
    printf("20. Read saved data file\n");
    printf("0. Exit\n");
}

static MemoryStrategy read_memory_strategy(void) {
    int strategy_choice = read_int("Memory strategy (0 First Fit, 1 Best Fit, 2 Worst Fit, 3 Paging): ");
    if (strategy_choice < 0 || strategy_choice > 3) {
        printf("Invalid strategy selected. Defaulting to First Fit.\n");
        strategy_choice = 0;
    }
    return (MemoryStrategy)strategy_choice;
}

static void create_task_flow(void) {
    char msg[8192];
    char name[64];
    char type[32];
    int max_need[RESOURCE_TYPES];

    printf("Task name: ");
    read_line(name, sizeof(name));

    printf("Service type (AMBULANCE/FIRE/POLICE): ");
    read_line(type, sizeof(type));
    uppercase_in_place(type);

    int burst = read_int("Burst time: ");
    int priority = read_int("Priority (1 highest): ");
    int memory = read_int("Memory required: ");

    max_need[0] = read_int("Max Communication Channels: ");
    max_need[1] = read_int("Max Vehicles: ");
    max_need[2] = read_int("Max Staff Units: ");

    serc_add_task(name, type, burst, priority, memory, max_need,
                  read_memory_strategy(), msg, sizeof(msg));

    printf("%s\n", msg);
}

static void request_resources_flow(void) {
    char msg[8192];
    int req[RESOURCE_TYPES];
    int pid = read_int("PID: ");

    req[0] = read_int("Request Communication Channels: ");
    req[1] = read_int("Request Vehicles: ");
    req[2] = read_int("Request Staff Units: ");

    serc_request_resources(pid, req, msg, sizeof(msg));
    printf("%s\n", msg);
}

static void release_resources_flow(void) {
    char msg[8192];
    int rel[RESOURCE_TYPES];
    int pid = read_int("PID: ");

    rel[0] = read_int("Release Communication Channels: ");
    rel[1] = read_int("Release Vehicles: ");
    rel[2] = read_int("Release Staff Units: ");

    serc_release_resources(pid, rel, msg, sizeof(msg));
    printf("%s\n", msg);
}

static void send_ipc_flow(void) {
    char msg[8192];
    char text[128];
    int from = read_int("From PID: ");
    int to = read_int("To PID: ");

    printf("Message: ");
    read_line(text, sizeof(text));

    serc_send_message(from, to, text, msg, sizeof(msg));
    printf("%s\n", msg);
}

static void read_data_file_flow(void) {
    char name[260];
    char msg[16384];

    printf("Saved file name: ");
    read_line(name, sizeof(name));
    serc_read_data_file(name, msg, sizeof(msg));
    printf("%s\n", msg);
}

int main(void) {
    serc_init();
    printf("SERC Mini-OS Console Started. Log file: %s\n", get_log_file_path());

    while (1) {
        char msg[8192];
        int choice;

        menu();
        choice = read_int("Enter choice: ");

        if (choice == 0) {
            printf("Exiting.\n");
            break;
        }

        switch (choice) {
            case 1:
                serc_load_demo_data(msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 2:
                create_task_flow();
                break;
            case 3:
                serc_suspend_task(read_int("PID: "), msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 4:
                serc_resume_task(read_int("PID: "), msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 5:
                serc_terminate_task(read_int("PID: "), msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 6:
                serc_run_scheduler(SCHED_FCFS, RR_QUANTUM_DEFAULT, msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 7:
                serc_run_scheduler(SCHED_SJF, RR_QUANTUM_DEFAULT, msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 8:
                serc_run_scheduler(SCHED_PRIORITY, RR_QUANTUM_DEFAULT, msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 9: {
                int quantum = read_int("Quantum: ");
                serc_run_scheduler(SCHED_RR, quantum, msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            }
            case 10:
                serc_compare_schedulers(read_int("Round Robin quantum for comparison: "), msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 11:
                request_resources_flow();
                break;
            case 12:
                release_resources_flow();
                break;
            case 13:
                send_ipc_flow();
                break;
            case 14:
                serc_check_deadlock_safety(msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 15:
                serc_full_status_report(msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 16:
                serc_logs_to_string(msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 17: {
                char saved[260];
                serc_save_status_snapshot(saved, sizeof(saved), msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            }
            case 18: {
                char saved[260];
                serc_save_schedule_report(saved, sizeof(saved), msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            }
            case 19:
                serc_list_data_files(msg, sizeof(msg));
                printf("%s\n", msg);
                break;
            case 20:
                read_data_file_flow();
                break;
            default:
                printf("Invalid choice.\n");
                break;
        }

        pause_console();
    }

    return 0;
}
