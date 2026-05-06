#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system_state.h"
#include "logger.h"

#define APP_CLASS_NAME "SercMiniOSWindow"
#define APP_TITLE      "SERC Mini-OS - Smart Emergency Response Center"

/* Navigation */
#define IDC_NAV_TASKS       1001
#define IDC_NAV_SCHED       1002
#define IDC_NAV_RESOURCES   1003
#define IDC_NAV_IPC         1004
#define IDC_NAV_SYSTEM      1005

/* Output */
#define IDC_OUTPUT          1100

/* Task section */
#define IDC_TASK_NAME       1201
#define IDC_TASK_TYPE       1202
#define IDC_TASK_BURST      1203
#define IDC_TASK_PRIORITY   1204
#define IDC_TASK_MEMORY     1205
#define IDC_TASK_MAXC       1206
#define IDC_TASK_MAXV       1207
#define IDC_TASK_MAXS       1208
#define IDC_TASK_STRATEGY   1209
#define IDC_TASK_ADD        1210
#define IDC_TASK_DEMO       1211
#define IDC_TASK_PID        1212
#define IDC_TASK_SUSPEND    1213
#define IDC_TASK_RESUME     1214
#define IDC_TASK_TERMINATE  1215

/* Scheduling section */
#define IDC_SCHED_QUANTUM   1301
#define IDC_SCHED_FCFS      1302
#define IDC_SCHED_SJF       1303
#define IDC_SCHED_PRIORITY  1304
#define IDC_SCHED_RR        1305

/* Resource section */
#define IDC_RES_PID         1401
#define IDC_RES_REQC        1402
#define IDC_RES_REQV        1403
#define IDC_RES_REQS        1404
#define IDC_RES_REQUEST     1405
#define IDC_RES_RELEASE     1406

/* IPC section */
#define IDC_IPC_FROM        1501
#define IDC_IPC_TO          1502
#define IDC_IPC_TEXT        1503
#define IDC_IPC_SEND        1504

/* System section */
#define IDC_SYS_STATUS      1601
#define IDC_SYS_LOGS        1602
#define IDC_SYS_CLEAR       1603
#define IDC_SYS_RESET       1604

typedef enum {
    SECTION_TASKS = 0,
    SECTION_SCHEDULING,
    SECTION_RESOURCES,
    SECTION_IPC,
    SECTION_SYSTEM
} GuiSection;

static HFONT g_font = NULL;
static HFONT g_title_font = NULL;
static HFONT g_mono_font = NULL;

static HBRUSH g_brush_bg = NULL;
static HBRUSH g_brush_panel = NULL;
static HBRUSH g_brush_output = NULL;

static HWND g_output = NULL;
static HWND g_section_title = NULL;

static HWND g_panel_tasks = NULL;
static HWND g_panel_sched = NULL;
static HWND g_panel_resources = NULL;
static HWND g_panel_ipc = NULL;
static HWND g_panel_system = NULL;

static GuiSection g_current_section = SECTION_TASKS;

/* ----------------------------- Color / Theme ----------------------------- */

static COLORREF COLOR_BG = RGB(241, 246, 249);
static COLORREF COLOR_PANEL = RGB(255, 255, 255);
static COLORREF COLOR_OUTPUT = RGB(248, 250, 252);
static COLORREF COLOR_TEXT = RGB(25, 35, 45);
static COLORREF COLOR_MUTED = RGB(90, 105, 120);
/* ----------------------------- Utility Helpers ----------------------------- */

static void set_control_font(HWND control, HFONT font) {
    if (control && font) {
        SendMessageA(control, WM_SETFONT, (WPARAM)font, TRUE);
    }
}

static HWND create_label(HWND parent, const char *text, int x, int y, int w, int h, HFONT font) {
    HWND control = CreateWindowA(
        "STATIC", text,
        WS_CHILD | WS_VISIBLE,
        x, y, w, h,
        parent, NULL, GetModuleHandle(NULL), NULL
    );
    set_control_font(control, font ? font : g_font);
    return control;
}

static HWND create_group_box(HWND parent, const char *text, int x, int y, int w, int h) {
    HWND control = CreateWindowA(
        "BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        x, y, w, h,
        parent, NULL, GetModuleHandle(NULL), NULL
    );
    set_control_font(control, g_font);
    return control;
}

static HWND create_edit(HWND parent, int id, const char *text, int x, int y, int w, int h) {
    HWND control = CreateWindowExA(
        0, "EDIT", text,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        x, y, w, h,
        parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL
    );
    set_control_font(control, g_font);
    return control;
}

static HWND create_output_box(HWND parent, int id, int x, int y, int w, int h) {
    HWND control = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
        x, y, w, h,
        parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL
    );
    set_control_font(control, g_mono_font);
    return control;
}

static HWND create_button(HWND parent, int id, const char *text, int x, int y, int w, int h) {
    HWND control = CreateWindowA(
        "BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, y, w, h,
        parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL
    );
    set_control_font(control, g_font);
    return control;
}

static HWND create_combo(HWND parent, int id, int x, int y, int w, int h, const char **items, int count) {
    HWND combo = CreateWindowA(
        "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, w, h,
        parent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL
    );
    int i;
    set_control_font(combo, g_font);

    for (i = 0; i < count; i++) {
        SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)items[i]);
    }
    SendMessageA(combo, CB_SETCURSEL, 0, 0);
    return combo;
}

static HWND create_panel(HWND parent, int x, int y, int w, int h) {
    HWND panel = CreateWindowExA(
        0, "STATIC", "",
        WS_CHILD | WS_VISIBLE,
        x, y, w, h,
        parent, NULL, GetModuleHandle(NULL), NULL
    );
    set_control_font(panel, g_font);
    return panel;
}

static void replace_output(const char *text) {
    if (g_output) {
        SetWindowTextA(g_output, text ? text : "");
    }
}

static void append_output(const char *text) {
    int len;
    if (!g_output || !text) return;
    len = GetWindowTextLengthA(g_output);
    SendMessageA(g_output, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(g_output, EM_REPLACESEL, 0, (LPARAM)text);
    SendMessageA(g_output, EM_REPLACESEL, 0, (LPARAM)"\r\n\r\n");
}

static void get_text_value(HWND parent, int id, char *buf, int size) {
    HWND control;
    if (!buf || size <= 0) return;
    buf[0] = '\0';
    control = GetDlgItem(parent, id);
    if (control) {
        GetWindowTextA(control, buf, size);
        buf[size - 1] = '\0';
    }
}

static int get_int_value(HWND parent, int id) {
    char buf[64];
    get_text_value(parent, id, buf, sizeof(buf));
    return atoi(buf);
}

static int validate_non_negative_int(HWND parent, int id, const char *field_name) {
    char buf[64];
    char msg[256];
    int i;

    get_text_value(parent, id, buf, sizeof(buf));

    if (buf[0] == '\0') {
        snprintf(msg, sizeof(msg), "%s cannot be empty.", field_name);
        MessageBoxA(parent, msg, "Validation Error", MB_OK | MB_ICONWARNING);
        return 0;
    }

    for (i = 0; buf[i] != '\0'; i++) {
        if (buf[i] < '0' || buf[i] > '9') {
            snprintf(msg, sizeof(msg), "%s must be a non-negative integer.", field_name);
            MessageBoxA(parent, msg, "Validation Error", MB_OK | MB_ICONWARNING);
            return 0;
        }
    }

    return 1;
}

static void show_status_report(void) {
    char buffer[16384];
    memset(buffer, 0, sizeof(buffer));
    serc_full_status_report(buffer, sizeof(buffer));
    replace_output(buffer);
}

static void show_logs_report(void) {
    char buffer[16384];
    memset(buffer, 0, sizeof(buffer));
    serc_logs_to_string(buffer, sizeof(buffer));
    replace_output(buffer);
}

static MemoryStrategy get_selected_strategy(HWND parent) {
    int index = (int)SendMessageA(GetDlgItem(parent, IDC_TASK_STRATEGY), CB_GETCURSEL, 0, 0);
    if (index < 0) index = 0;
    return (MemoryStrategy)index;
}

static const char *get_selected_service(HWND parent) {
    static char text[32];
    HWND combo = GetDlgItem(parent, IDC_TASK_TYPE);
    int index;

    memset(text, 0, sizeof(text));
    if (!combo) {
        strcpy(text, "AMBULANCE");
        return text;
    }

    index = (int)SendMessageA(combo, CB_GETCURSEL, 0, 0);
    if (index < 0) index = 0;

    SendMessageA(combo, CB_GETLBTEXT, index, (LPARAM)text);
    text[sizeof(text) - 1] = '\0';
    return text;
}

/* ----------------------------- Section Switching ----------------------------- */

static void hide_all_panels(void) {
    ShowWindow(g_panel_tasks, SW_HIDE);
    ShowWindow(g_panel_sched, SW_HIDE);
    ShowWindow(g_panel_resources, SW_HIDE);
    ShowWindow(g_panel_ipc, SW_HIDE);
    ShowWindow(g_panel_system, SW_HIDE);
}

static void switch_section(GuiSection section) {
    hide_all_panels();
    g_current_section = section;

    switch (section) {
        case SECTION_TASKS:
            SetWindowTextA(g_section_title, "Task Management");
            ShowWindow(g_panel_tasks, SW_SHOW);
            break;
        case SECTION_SCHEDULING:
            SetWindowTextA(g_section_title, "CPU Scheduling");
            ShowWindow(g_panel_sched, SW_SHOW);
            break;
        case SECTION_RESOURCES:
            SetWindowTextA(g_section_title, "Resource / Deadlock Handling");
            ShowWindow(g_panel_resources, SW_SHOW);
            break;
        case SECTION_IPC:
            SetWindowTextA(g_section_title, "Inter-Process Communication");
            ShowWindow(g_panel_ipc, SW_SHOW);
            break;
        case SECTION_SYSTEM:
            SetWindowTextA(g_section_title, "System View");
            ShowWindow(g_panel_system, SW_SHOW);
            break;
    }
}

/* ----------------------------- Build Task Panel ----------------------------- */

static void build_task_panel(HWND parent) {
    const char *types[] = {"AMBULANCE", "FIRE", "POLICE"};
    const char *strategies[] = {"First Fit", "Best Fit", "Worst Fit"};

    create_group_box(parent, "Create Emergency Task", 10, 10, 660, 150);
    create_label(parent, "Task Name", 30, 40, 80, 22, NULL);
    create_edit(parent, IDC_TASK_NAME, "Ambulance Dispatch", 120, 38, 180, 25);

    create_label(parent, "Service", 330, 40, 60, 22, NULL);
    create_combo(parent, IDC_TASK_TYPE, 395, 38, 130, 160, types, 3);

    create_label(parent, "Burst", 30, 78, 50, 22, NULL);
    create_edit(parent, IDC_TASK_BURST, "4", 120, 76, 70, 25);

    create_label(parent, "Priority", 220, 78, 55, 22, NULL);
    create_edit(parent, IDC_TASK_PRIORITY, "1", 285, 76, 70, 25);

    create_label(parent, "Memory", 385, 78, 55, 22, NULL);
    create_edit(parent, IDC_TASK_MEMORY, "120", 450, 76, 75, 25);

    create_label(parent, "Max Comm", 30, 115, 75, 22, NULL);
    create_edit(parent, IDC_TASK_MAXC, "2", 120, 113, 60, 25);

    create_label(parent, "Max Vehicles", 205, 115, 85, 22, NULL);
    create_edit(parent, IDC_TASK_MAXV, "1", 300, 113, 60, 25);

    create_label(parent, "Max Staff", 385, 115, 65, 22, NULL);
    create_edit(parent, IDC_TASK_MAXS, "2", 460, 113, 60, 25);

    create_group_box(parent, "Allocation / Process Actions", 10, 175, 660, 120);
    create_label(parent, "Memory Strategy", 30, 205, 105, 22, NULL);
    create_combo(parent, IDC_TASK_STRATEGY, 145, 203, 140, 160, strategies, 3);

    create_button(parent, IDC_TASK_ADD, "Add Task", 310, 203, 100, 30);
    create_button(parent, IDC_TASK_DEMO, "Load Demo Data", 425, 203, 120, 30);

    create_label(parent, "PID", 30, 248, 35, 22, NULL);
    create_edit(parent, IDC_TASK_PID, "1", 70, 246, 70, 25);
    create_button(parent, IDC_TASK_SUSPEND, "Suspend", 165, 244, 100, 30);
    create_button(parent, IDC_TASK_RESUME, "Resume", 280, 244, 100, 30);
    create_button(parent, IDC_TASK_TERMINATE, "Terminate", 395, 244, 110, 30);
}

/* --------------------------- Build Scheduling Panel -------------------------- */

static void build_sched_panel(HWND parent) {
    create_group_box(parent, "Run Scheduling Algorithms", 10, 10, 660, 170);
    create_label(parent, "Choose an algorithm to schedule READY tasks.", 30, 40, 320, 22, NULL);

    create_button(parent, IDC_SCHED_FCFS, "Run FCFS", 30, 80, 120, 34);
    create_button(parent, IDC_SCHED_SJF, "Run SJF", 170, 80, 120, 34);
    create_button(parent, IDC_SCHED_PRIORITY, "Run Priority", 310, 80, 135, 34);

    create_label(parent, "Round Robin Quantum", 30, 130, 130, 22, NULL);
    create_edit(parent, IDC_SCHED_QUANTUM, "2", 170, 128, 70, 25);
    create_button(parent, IDC_SCHED_RR, "Run Round Robin", 265, 125, 150, 34);
}

/* --------------------------- Build Resource Panel --------------------------- */

static void build_resource_panel(HWND parent) {
    create_group_box(parent, "Request / Release Resources", 10, 10, 660, 190);

    create_label(parent, "PID", 30, 45, 35, 22, NULL);
    create_edit(parent, IDC_RES_PID, "1", 80, 43, 70, 25);

    create_label(parent, "Communication Channels", 30, 85, 150, 22, NULL);
    create_edit(parent, IDC_RES_REQC, "1", 200, 83, 70, 25);

    create_label(parent, "Vehicles", 30, 122, 70, 22, NULL);
    create_edit(parent, IDC_RES_REQV, "0", 200, 120, 70, 25);

    create_label(parent, "Staff Units", 30, 159, 70, 22, NULL);
    create_edit(parent, IDC_RES_REQS, "1", 200, 157, 70, 25);

    create_button(parent, IDC_RES_REQUEST, "Request Resources", 330, 82, 150, 34);
    create_button(parent, IDC_RES_RELEASE, "Release Resources", 330, 126, 150, 34);
}

/* ------------------------------ Build IPC Panel ----------------------------- */

static void build_ipc_panel(HWND parent) {
    create_group_box(parent, "Inter-Process Communication", 10, 10, 660, 160);

    create_label(parent, "From PID", 30, 45, 60, 22, NULL);
    create_edit(parent, IDC_IPC_FROM, "1", 100, 43, 70, 25);

    create_label(parent, "To PID", 200, 45, 50, 22, NULL);
    create_edit(parent, IDC_IPC_TO, "2", 255, 43, 70, 25);

    create_label(parent, "Message", 30, 88, 60, 22, NULL);
    create_edit(parent, IDC_IPC_TEXT, "Coordinate traffic diversion.", 100, 86, 340, 25);

    create_button(parent, IDC_IPC_SEND, "Send IPC Message", 100, 125, 150, 34);
}

/* ---------------------------- Build System Panel ---------------------------- */

static void build_system_panel(HWND parent) {
    create_group_box(parent, "System Utilities", 10, 10, 660, 160);
    create_label(parent, "Inspect system status, read logs, clear the output, or reset the simulator.", 30, 45, 500, 22, NULL);

    create_button(parent, IDC_SYS_STATUS, "View Status", 30, 90, 110, 34);
    create_button(parent, IDC_SYS_LOGS, "View Logs", 155, 90, 110, 34);
    create_button(parent, IDC_SYS_CLEAR, "Clear Output", 280, 90, 110, 34);
    create_button(parent, IDC_SYS_RESET, "Reset System", 405, 90, 110, 34);
}

/* ------------------------------ Build Main UI ------------------------------ */

static void build_main_ui(HWND hwnd) {
    create_label(hwnd, "SERC Mini-OS", 20, 18, 170, 28, g_title_font);
    g_section_title = create_label(hwnd, "Task Management", 190, 20, 360, 26, g_title_font);

    create_group_box(hwnd, "Menu", 10, 60, 140, 250);
    create_button(hwnd, IDC_NAV_TASKS, "Task Mgmt", 28, 95, 105, 34);
    create_button(hwnd, IDC_NAV_SCHED, "Scheduling", 28, 137, 105, 34);
    create_button(hwnd, IDC_NAV_RESOURCES, "Resources", 28, 179, 105, 34);
    create_button(hwnd, IDC_NAV_IPC, "IPC", 28, 221, 105, 34);
    create_button(hwnd, IDC_NAV_SYSTEM, "System", 28, 263, 105, 34);

    g_panel_tasks = create_panel(hwnd, 170, 60, 690, 300);
    g_panel_sched = create_panel(hwnd, 170, 60, 690, 300);
    g_panel_resources = create_panel(hwnd, 170, 60, 690, 300);
    g_panel_ipc = create_panel(hwnd, 170, 60, 690, 300);
    g_panel_system = create_panel(hwnd, 170, 60, 690, 300);

    build_task_panel(g_panel_tasks);
    build_sched_panel(g_panel_sched);
    build_resource_panel(g_panel_resources);
    build_ipc_panel(g_panel_ipc);
    build_system_panel(g_panel_system);

    create_group_box(hwnd, "Output / Status", 10, 370, 850, 225);
    g_output = create_output_box(hwnd, IDC_OUTPUT, 25, 395, 820, 180);
}

/* ------------------------------- App Actions ------------------------------- */

static void do_add_task(void) {
    char msg[16384];
    char name[64];
    int max_need[RESOURCE_TYPES];

    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_BURST, "Burst time") ||
        !validate_non_negative_int(g_panel_tasks, IDC_TASK_PRIORITY, "Priority") ||
        !validate_non_negative_int(g_panel_tasks, IDC_TASK_MEMORY, "Memory") ||
        !validate_non_negative_int(g_panel_tasks, IDC_TASK_MAXC, "Max communication channels") ||
        !validate_non_negative_int(g_panel_tasks, IDC_TASK_MAXV, "Max vehicles") ||
        !validate_non_negative_int(g_panel_tasks, IDC_TASK_MAXS, "Max staff units")) {
        return;
    }

    get_text_value(g_panel_tasks, IDC_TASK_NAME, name, sizeof(name));
    if (name[0] == '\0') {
        MessageBoxA(g_panel_tasks, "Task name cannot be empty.", "Validation Error", MB_OK | MB_ICONWARNING);
        return;
    }

    max_need[0] = get_int_value(g_panel_tasks, IDC_TASK_MAXC);
    max_need[1] = get_int_value(g_panel_tasks, IDC_TASK_MAXV);
    max_need[2] = get_int_value(g_panel_tasks, IDC_TASK_MAXS);

    memset(msg, 0, sizeof(msg));
    serc_add_task(
        name,
        get_selected_service(g_panel_tasks),
        get_int_value(g_panel_tasks, IDC_TASK_BURST),
        get_int_value(g_panel_tasks, IDC_TASK_PRIORITY),
        get_int_value(g_panel_tasks, IDC_TASK_MEMORY),
        max_need,
        get_selected_strategy(g_panel_tasks),
        msg,
        sizeof(msg)
    );
    append_output(msg);
}

static void do_load_demo(void) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_load_demo_data(msg, sizeof(msg));
    append_output(msg);
}

static void do_suspend(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_suspend_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
}

static void do_resume(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_resume_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
}

static void do_terminate(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_terminate_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
}

static void do_run_scheduler(int algorithm, int quantum) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_run_scheduler(algorithm, quantum, msg, sizeof(msg));
    append_output(msg);
}

static void do_request_resources(void) {
    char msg[16384];
    int req[RESOURCE_TYPES];

    if (!validate_non_negative_int(g_panel_resources, IDC_RES_PID, "PID") ||
        !validate_non_negative_int(g_panel_resources, IDC_RES_REQC, "Communication channels") ||
        !validate_non_negative_int(g_panel_resources, IDC_RES_REQV, "Vehicles") ||
        !validate_non_negative_int(g_panel_resources, IDC_RES_REQS, "Staff units")) {
        return;
    }

    req[0] = get_int_value(g_panel_resources, IDC_RES_REQC);
    req[1] = get_int_value(g_panel_resources, IDC_RES_REQV);
    req[2] = get_int_value(g_panel_resources, IDC_RES_REQS);

    memset(msg, 0, sizeof(msg));
    serc_request_resources(get_int_value(g_panel_resources, IDC_RES_PID), req, msg, sizeof(msg));
    append_output(msg);
}

static void do_release_resources(void) {
    char msg[16384];
    int rel[RESOURCE_TYPES];

    if (!validate_non_negative_int(g_panel_resources, IDC_RES_PID, "PID") ||
        !validate_non_negative_int(g_panel_resources, IDC_RES_REQC, "Communication channels") ||
        !validate_non_negative_int(g_panel_resources, IDC_RES_REQV, "Vehicles") ||
        !validate_non_negative_int(g_panel_resources, IDC_RES_REQS, "Staff units")) {
        return;
    }

    rel[0] = get_int_value(g_panel_resources, IDC_RES_REQC);
    rel[1] = get_int_value(g_panel_resources, IDC_RES_REQV);
    rel[2] = get_int_value(g_panel_resources, IDC_RES_REQS);

    memset(msg, 0, sizeof(msg));
    serc_release_resources(get_int_value(g_panel_resources, IDC_RES_PID), rel, msg, sizeof(msg));
    append_output(msg);
}

static void do_send_ipc(void) {
    char msg[16384];
    char text[128];

    if (!validate_non_negative_int(g_panel_ipc, IDC_IPC_FROM, "From PID") ||
        !validate_non_negative_int(g_panel_ipc, IDC_IPC_TO, "To PID")) {
        return;
    }

    get_text_value(g_panel_ipc, IDC_IPC_TEXT, text, sizeof(text));
    if (text[0] == '\0') {
        MessageBoxA(g_panel_ipc, "IPC message cannot be empty.", "Validation Error", MB_OK | MB_ICONWARNING);
        return;
    }

    memset(msg, 0, sizeof(msg));
    serc_send_message(
        get_int_value(g_panel_ipc, IDC_IPC_FROM),
        get_int_value(g_panel_ipc, IDC_IPC_TO),
        text,
        msg,
        sizeof(msg)
    );
    append_output(msg);
}

static void do_reset_system(void) {
    serc_init();
    append_output("System reset completed.");
    show_status_report();
}

/* ----------------------------- Fonts / Resources ----------------------------- */

static void init_fonts(void) {
    g_font = CreateFontA(
        18, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Segoe UI"
    );

    g_title_font = CreateFontA(
        26, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Segoe UI"
    );

    g_mono_font = CreateFontA(
        18, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN,
        "Consolas"
    );
}

static void init_brushes(void) {
    g_brush_bg = CreateSolidBrush(COLOR_BG);
    g_brush_panel = CreateSolidBrush(COLOR_PANEL);
    g_brush_output = CreateSolidBrush(COLOR_OUTPUT);
}

static void destroy_fonts(void) {
    if (g_font) { DeleteObject(g_font); g_font = NULL; }
    if (g_title_font) { DeleteObject(g_title_font); g_title_font = NULL; }
    if (g_mono_font) { DeleteObject(g_mono_font); g_mono_font = NULL; }
}

static void destroy_brushes(void) {
    if (g_brush_bg) { DeleteObject(g_brush_bg); g_brush_bg = NULL; }
    if (g_brush_panel) { DeleteObject(g_brush_panel); g_brush_panel = NULL; }
    if (g_brush_output) { DeleteObject(g_brush_output); g_brush_output = NULL; }
}

/* ------------------------------ Window Procedure ------------------------------ */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            init_fonts();
            init_brushes();
            build_main_ui(hwnd);
            serc_init();
            switch_section(SECTION_TASKS);
            replace_output("SERC Mini-OS GUI started.\r\n\r\nUse the menu on the left to navigate.");
            return 0;

        case WM_COMMAND: {
            int id = LOWORD(wParam);

            switch (id) {
                case IDC_NAV_TASKS:      switch_section(SECTION_TASKS); return 0;
                case IDC_NAV_SCHED:      switch_section(SECTION_SCHEDULING); return 0;
                case IDC_NAV_RESOURCES:  switch_section(SECTION_RESOURCES); return 0;
                case IDC_NAV_IPC:        switch_section(SECTION_IPC); return 0;
                case IDC_NAV_SYSTEM:     switch_section(SECTION_SYSTEM); return 0;

                case IDC_TASK_ADD:       do_add_task(); return 0;
                case IDC_TASK_DEMO:      do_load_demo(); return 0;
                case IDC_TASK_SUSPEND:   do_suspend(); return 0;
                case IDC_TASK_RESUME:    do_resume(); return 0;
                case IDC_TASK_TERMINATE: do_terminate(); return 0;

                case IDC_SCHED_FCFS:     do_run_scheduler(SCHED_FCFS, RR_QUANTUM_DEFAULT); return 0;
                case IDC_SCHED_SJF:      do_run_scheduler(SCHED_SJF, RR_QUANTUM_DEFAULT); return 0;
                case IDC_SCHED_PRIORITY: do_run_scheduler(SCHED_PRIORITY, RR_QUANTUM_DEFAULT); return 0;
                case IDC_SCHED_RR:
                    if (!validate_non_negative_int(g_panel_sched, IDC_SCHED_QUANTUM, "Quantum")) return 0;
                    do_run_scheduler(SCHED_RR, get_int_value(g_panel_sched, IDC_SCHED_QUANTUM));
                    return 0;

                case IDC_RES_REQUEST:    do_request_resources(); return 0;
                case IDC_RES_RELEASE:    do_release_resources(); return 0;

                case IDC_IPC_SEND:       do_send_ipc(); return 0;

                case IDC_SYS_STATUS:     show_status_report(); return 0;
                case IDC_SYS_LOGS:       show_logs_report(); return 0;
                case IDC_SYS_CLEAR:      replace_output(""); return 0;
                case IDC_SYS_RESET:      do_reset_system(); return 0;
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND control = (HWND)lParam;

            SetTextColor(hdc, COLOR_TEXT);
            SetBkMode(hdc, TRANSPARENT);

            if (control == g_section_title) {
                return (LRESULT)g_brush_bg;
            }

            if (GetParent(control) == g_panel_tasks ||
                GetParent(control) == g_panel_sched ||
                GetParent(control) == g_panel_resources ||
                GetParent(control) == g_panel_ipc ||
                GetParent(control) == g_panel_system) {
                return (LRESULT)g_brush_panel;
            }

            return (LRESULT)g_brush_bg;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            HWND control = (HWND)lParam;
            SetTextColor(hdc, COLOR_TEXT);
            SetBkColor(hdc, (control == g_output) ? COLOR_OUTPUT : COLOR_PANEL);
            return (LRESULT)((control == g_output) ? g_brush_output : g_brush_panel);
        }

        case WM_ERASEBKGND: {
            RECT rc;
            HDC hdc = (HDC)wParam;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, g_brush_bg);
            return 1;
        }

        case WM_DESTROY:
            destroy_fonts();
            destroy_brushes();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* --------------------------------- WinMain --------------------------------- */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;

    (void)hPrevInstance;
    (void)lpCmdLine;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    hwnd = CreateWindowA(
        APP_CLASS_NAME,
        APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        890, 660,
        NULL, NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBoxA(NULL, "Failed to create main window.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}

#else

int main(void) {
    return 0;
}

#endif