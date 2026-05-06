#ifdef _WIN32

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system_state.h"
#include "logger.h"
#include "process.h"
#include "memory.h"
#include "scheduler.h"

#define APP_CLASS_NAME    "SercMiniOSWindow"
#define PANEL_CLASS_NAME  "SercMiniOSPanel"
#define CHART_CLASS_NAME  "SercScheduleChart"
#define APP_TITLE         "SERC Mini-OS - Smart Emergency Response Center"

#define IDT_REFRESH_TIMER 5001

/* Navigation */
#define IDC_NAV_TASKS       1001
#define IDC_NAV_SCHED       1002
#define IDC_NAV_RESOURCES   1003
#define IDC_NAV_IPC         1004
#define IDC_NAV_SYSTEM      1005

/* Output + summary */
#define IDC_OUTPUT          1100
#define IDC_PROCESS_LIST    1101
#define IDC_SUMMARY_STATS   1102

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
#define IDC_SCHED_SUMMARY   1306
#define IDC_SCHED_COMPUTE   1307
#define IDC_SCHED_CHART     1308
#define IDC_SCHED_METRICS   1309
#define IDC_SCHED_COMPARE   1310

/* Resource section */
#define IDC_RES_PID         1401
#define IDC_RES_REQC        1402
#define IDC_RES_REQV        1403
#define IDC_RES_REQS        1404
#define IDC_RES_REQUEST     1405
#define IDC_RES_RELEASE     1406
#define IDC_RES_CHECK       1407

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

/* ---------------- Theme ---------------- */

static COLORREF COLOR_BG           = RGB(241, 245, 251);
static COLORREF COLOR_PANEL        = RGB(255, 255, 255);
static COLORREF COLOR_OUTPUT       = RGB(248, 250, 253);
static COLORREF COLOR_SIDEBAR      = RGB(20, 36, 68);
static COLORREF COLOR_TITLE        = RGB(18, 43, 78);
static COLORREF COLOR_TEXT         = RGB(35, 47, 62);
static COLORREF COLOR_SUBTEXT      = RGB(94, 108, 125);
static COLORREF COLOR_NAV          = RGB(33, 84, 153);
static COLORREF COLOR_NAV_ACTIVE   = RGB(14, 131, 233);
static COLORREF COLOR_LIST_BG      = RGB(255, 255, 255);
static COLORREF COLOR_LIST_TEXT    = RGB(32, 44, 59);
static COLORREF COLOR_CHART_BG     = RGB(246, 249, 253);
static COLORREF COLOR_CHART_AXIS   = RGB(80, 96, 114);
static COLORREF COLOR_CHART_BORDER = RGB(212, 220, 232);
static COLORREF COLOR_CHART_GRID   = RGB(225, 232, 242);

static COLORREF CHART_COLORS[] = {
    RGB(0, 132, 255),
    RGB(21, 170, 91),
    RGB(141, 93, 246),
    RGB(245, 158, 11),
    RGB(236, 72, 153),
    RGB(239, 68, 68),
    RGB(14, 165, 233),
    RGB(99, 102, 241),
    RGB(16, 185, 129),
    RGB(234, 88, 12)
};

static COLORREF STATE_READY_BG      = RGB(220, 252, 231);
static COLORREF STATE_READY_TEXT    = RGB(22, 101, 52);
static COLORREF STATE_RUNNING_BG    = RGB(219, 234, 254);
static COLORREF STATE_RUNNING_TEXT  = RGB(30, 64, 175);
static COLORREF STATE_WAITING_BG    = RGB(254, 243, 199);
static COLORREF STATE_WAITING_TEXT  = RGB(146, 64, 14);
static COLORREF STATE_SUSP_BG       = RGB(237, 233, 254);
static COLORREF STATE_SUSP_TEXT     = RGB(91, 33, 182);
static COLORREF STATE_TERM_BG       = RGB(254, 226, 226);
static COLORREF STATE_TERM_TEXT     = RGB(153, 27, 27);
static COLORREF STATE_NEW_BG        = RGB(229, 231, 235);
static COLORREF STATE_NEW_TEXT      = RGB(55, 65, 81);

/* ---------------- Fonts / brushes ---------------- */

static HFONT g_font = NULL;
static HFONT g_title_font = NULL;
static HFONT g_mono_font = NULL;
static HFONT g_small_font = NULL;
static HFONT g_chart_font = NULL;

static HBRUSH g_brush_bg = NULL;
static HBRUSH g_brush_panel = NULL;
static HBRUSH g_brush_output = NULL;
static HBRUSH g_brush_sidebar = NULL;
static HBRUSH g_brush_chart = NULL;

/* ---------------- General window state ---------------- */

static GuiSection g_current_section = SECTION_TASKS;

/* Main shared controls */
static HWND g_section_title = NULL;
static HWND g_output = NULL;
static HWND g_process_list = NULL;
static HWND g_stats_label = NULL;

/* Main frame controls */
static HWND g_sidebar_group = NULL;
static HWND g_nav_tasks = NULL;
static HWND g_nav_sched = NULL;
static HWND g_nav_resources = NULL;
static HWND g_nav_ipc = NULL;
static HWND g_nav_system = NULL;
static HWND g_summary_group = NULL;
static HWND g_output_group = NULL;

/* Panels */
static HWND g_panel_tasks = NULL;
static HWND g_panel_sched = NULL;
static HWND g_panel_resources = NULL;
static HWND g_panel_ipc = NULL;
static HWND g_panel_system = NULL;

/* Task panel controls */
static HWND g_task_group_create = NULL;
static HWND g_task_group_actions = NULL;
static HWND g_task_lbl_name = NULL;
static HWND g_task_lbl_service = NULL;
static HWND g_task_lbl_burst = NULL;
static HWND g_task_lbl_priority = NULL;
static HWND g_task_lbl_memory = NULL;
static HWND g_task_lbl_maxc = NULL;
static HWND g_task_lbl_maxv = NULL;
static HWND g_task_lbl_maxs = NULL;
static HWND g_task_lbl_strategy = NULL;
static HWND g_task_lbl_pid = NULL;
static HWND g_task_name = NULL;
static HWND g_task_type = NULL;
static HWND g_task_burst = NULL;
static HWND g_task_priority = NULL;
static HWND g_task_memory = NULL;
static HWND g_task_maxc = NULL;
static HWND g_task_maxv = NULL;
static HWND g_task_maxs = NULL;
static HWND g_task_strategy = NULL;
static HWND g_task_add = NULL;
static HWND g_task_demo = NULL;
static HWND g_task_pid = NULL;
static HWND g_task_suspend = NULL;
static HWND g_task_resume = NULL;
static HWND g_task_terminate = NULL;

/* Scheduling panel controls */
static HWND g_sched_group = NULL;
static HWND g_sched_desc = NULL;
static HWND g_sched_lbl_quantum = NULL;
static HWND g_sched_quantum = NULL;
static HWND g_sched_fcfs = NULL;
static HWND g_sched_sjf = NULL;
static HWND g_sched_priority = NULL;
static HWND g_sched_rr = NULL;
static HWND g_sched_compare = NULL;
static HWND g_sched_show_summary = NULL;
static HWND g_sched_show_compute = NULL;
static HWND g_sched_chart_group = NULL;
static HWND g_sched_metrics = NULL;
static HWND g_sched_chart = NULL;

/* Resource panel controls */
static HWND g_res_group = NULL;
static HWND g_res_lbl_pid = NULL;
static HWND g_res_lbl_c = NULL;
static HWND g_res_lbl_v = NULL;
static HWND g_res_lbl_s = NULL;
static HWND g_res_pid = NULL;
static HWND g_res_reqc = NULL;
static HWND g_res_reqv = NULL;
static HWND g_res_reqs = NULL;
static HWND g_res_request = NULL;
static HWND g_res_release = NULL;
static HWND g_res_check = NULL;

/* IPC panel controls */
static HWND g_ipc_group = NULL;
static HWND g_ipc_lbl_from = NULL;
static HWND g_ipc_lbl_to = NULL;
static HWND g_ipc_lbl_text = NULL;
static HWND g_ipc_from = NULL;
static HWND g_ipc_to = NULL;
static HWND g_ipc_text = NULL;
static HWND g_ipc_send = NULL;

/* System panel controls */
static HWND g_sys_group = NULL;
static HWND g_sys_desc = NULL;
static HWND g_sys_status = NULL;
static HWND g_sys_logs = NULL;
static HWND g_sys_clear = NULL;
static HWND g_sys_reset = NULL;

/* ---------------- Helpers ---------------- */

static int is_nav_button(int id) {
    return id >= IDC_NAV_TASKS && id <= IDC_NAV_SYSTEM;
}

static int is_active_nav(int id) {
    if (g_current_section == SECTION_TASKS && id == IDC_NAV_TASKS) return 1;
    if (g_current_section == SECTION_SCHEDULING && id == IDC_NAV_SCHED) return 1;
    if (g_current_section == SECTION_RESOURCES && id == IDC_NAV_RESOURCES) return 1;
    if (g_current_section == SECTION_IPC && id == IDC_NAV_IPC) return 1;
    if (g_current_section == SECTION_SYSTEM && id == IDC_NAV_SYSTEM) return 1;
    return 0;
}

static COLORREF state_back_color(ProcessState state) {
    switch (state) {
        case STATE_READY: return STATE_READY_BG;
        case STATE_RUNNING: return STATE_RUNNING_BG;
        case STATE_WAITING: return STATE_WAITING_BG;
        case STATE_SUSPENDED: return STATE_SUSP_BG;
        case STATE_TERMINATED: return STATE_TERM_BG;
        case STATE_NEW: return STATE_NEW_BG;
        default: return COLOR_LIST_BG;
    }
}

static COLORREF state_text_color(ProcessState state) {
    switch (state) {
        case STATE_READY: return STATE_READY_TEXT;
        case STATE_RUNNING: return STATE_RUNNING_TEXT;
        case STATE_WAITING: return STATE_WAITING_TEXT;
        case STATE_SUSPENDED: return STATE_SUSP_TEXT;
        case STATE_TERMINATED: return STATE_TERM_TEXT;
        case STATE_NEW: return STATE_NEW_TEXT;
        default: return COLOR_LIST_TEXT;
    }
}

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

static HWND create_nav_button(HWND parent, int id, const char *text, int x, int y, int w, int h) {
    HWND control = CreateWindowA(
        "BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
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
        0, PANEL_CLASS_NAME, "",
        WS_CHILD | WS_VISIBLE,
        x, y, w, h,
        parent, NULL, GetModuleHandle(NULL), NULL
    );
    set_control_font(panel, g_font);
    return panel;
}

static HWND create_chart(HWND parent, int x, int y, int w, int h) {
    HWND chart = CreateWindowExA(
        WS_EX_CLIENTEDGE, CHART_CLASS_NAME, "",
        WS_CHILD | WS_VISIBLE,
        x, y, w, h,
        parent, (HMENU)(INT_PTR)IDC_SCHED_CHART, GetModuleHandle(NULL), NULL
    );
    set_control_font(chart, g_chart_font ? g_chart_font : g_small_font);
    return chart;
}

static void set_bounds(HWND handle, int x, int y, int w, int h) {
    if (handle) {
        MoveWindow(handle, x, y, w, h, TRUE);
    }
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
            SetWindowTextA(g_section_title, "Resource & Deadlock Control");
            ShowWindow(g_panel_resources, SW_SHOW);
            break;
        case SECTION_IPC:
            SetWindowTextA(g_section_title, "Inter-Process Communication");
            ShowWindow(g_panel_ipc, SW_SHOW);
            break;
        case SECTION_SYSTEM:
            SetWindowTextA(g_section_title, "System Utilities");
            ShowWindow(g_panel_system, SW_SHOW);
            break;
    }

    InvalidateRect(GetParent(g_section_title), NULL, TRUE);
}

static void update_process_summary(void) {
    char stats[256];
    PCB *table = get_processes();
    int count = get_process_count();
    int active = 0;
    int ready = 0;
    int waiting = 0;
    int suspended = 0;
    int terminated = 0;
    int i;

    if (!g_process_list || !g_stats_label) return;

    ListView_DeleteAllItems(g_process_list);

    for (i = 0; i < count; i++) {
        char pid[16], pr[16], mem[16];
        LVITEMA item;
        PCB *p = &table[i];

        if (p->state != STATE_TERMINATED) active++;
        if (p->state == STATE_READY) ready++;
        if (p->state == STATE_WAITING) waiting++;
        if (p->state == STATE_SUSPENDED) suspended++;
        if (p->state == STATE_TERMINATED) terminated++;

        memset(&item, 0, sizeof(item));
        item.mask = LVIF_TEXT;
        item.iItem = i;

        snprintf(pid, sizeof(pid), "%d", p->pid);
        item.pszText = pid;
        ListView_InsertItem(g_process_list, &item);

        ListView_SetItemText(g_process_list, i, 1, p->name);
        ListView_SetItemText(g_process_list, i, 2, (LPSTR)state_to_string(p->state));

        snprintf(pr, sizeof(pr), "%d", p->priority);
        ListView_SetItemText(g_process_list, i, 3, pr);

        snprintf(mem, sizeof(mem), "%d", p->memory_required);
        ListView_SetItemText(g_process_list, i, 4, mem);
    }

    snprintf(stats, sizeof(stats),
             "Processes: %d | Active: %d | Ready: %d | Waiting: %d | Suspended: %d | Terminated: %d | Memory: %d/%d used",
             count, active, ready, waiting, suspended, terminated, get_memory_used(), TOTAL_MEMORY);
    SetWindowTextA(g_stats_label, stats);
}

static void refresh_schedule_visuals(void) {
    char metrics[512];
    ScheduleResult last;

    if (!g_sched_metrics || !g_sched_chart) {
        return;
    }

    if (!serc_has_last_schedule() || !serc_copy_last_schedule(&last)) {
        SetWindowTextA(g_sched_metrics,
                       "No schedule run yet. Run FCFS, SJF, Priority, or Round Robin to generate the chart and computations.");
        InvalidateRect(g_sched_chart, NULL, TRUE);
        return;
    }

    snprintf(metrics, sizeof(metrics),
             "Algorithm: %s | Avg Wait: %.2f | Avg Turnaround: %.2f | CPU Utilization: %.2f%% | Segments: %d",
             scheduler_to_string(last.type),
             last.average_waiting_time,
             last.average_turnaround_time,
             last.cpu_utilization,
             last.segment_count);

    SetWindowTextA(g_sched_metrics, metrics);
    InvalidateRect(g_sched_chart, NULL, TRUE);
}

/* ---------------- Build panels ---------------- */

static void build_task_panel(HWND parent) {
    const char *types[] = {"AMBULANCE", "FIRE", "POLICE"};
    const char *strategies[] = {"First Fit", "Best Fit", "Worst Fit"};

    g_task_group_create  = create_group_box(parent, "Create Emergency Task", 10, 10, 500, 150);
    g_task_group_actions = create_group_box(parent, "Memory Strategy and Process Actions", 10, 170, 500, 120);

    g_task_lbl_name      = create_label(parent, "Task Name", 30, 40, 80, 22, NULL);
    g_task_lbl_service   = create_label(parent, "Service", 290, 40, 60, 22, NULL);
    g_task_lbl_burst     = create_label(parent, "Burst", 30, 80, 50, 22, NULL);
    g_task_lbl_priority  = create_label(parent, "Priority", 180, 80, 55, 22, NULL);
    g_task_lbl_memory    = create_label(parent, "Memory", 330, 80, 55, 22, NULL);
    g_task_lbl_maxc      = create_label(parent, "Max Comm", 30, 118, 75, 22, NULL);
    g_task_lbl_maxv      = create_label(parent, "Max Vehicles", 180, 118, 85, 22, NULL);
    g_task_lbl_maxs      = create_label(parent, "Max Staff", 350, 118, 65, 22, NULL);
    g_task_lbl_strategy  = create_label(parent, "Strategy", 30, 205, 60, 22, NULL);
    g_task_lbl_pid       = create_label(parent, "PID", 30, 247, 30, 22, NULL);

    g_task_name          = create_edit(parent, IDC_TASK_NAME, "Ambulance Dispatch", 120, 38, 150, 25);
    g_task_type          = create_combo(parent, IDC_TASK_TYPE, 360, 38, 120, 150, types, 3);
    g_task_burst         = create_edit(parent, IDC_TASK_BURST, "4", 120, 78, 60, 25);
    g_task_priority      = create_edit(parent, IDC_TASK_PRIORITY, "1", 240, 78, 60, 25);
    g_task_memory        = create_edit(parent, IDC_TASK_MEMORY, "120", 400, 78, 80, 25);
    g_task_maxc          = create_edit(parent, IDC_TASK_MAXC, "2", 120, 116, 55, 25);
    g_task_maxv          = create_edit(parent, IDC_TASK_MAXV, "1", 280, 116, 55, 25);
    g_task_maxs          = create_edit(parent, IDC_TASK_MAXS, "2", 420, 116, 55, 25);

    g_task_strategy      = create_combo(parent, IDC_TASK_STRATEGY, 95, 203, 140, 150, strategies, 3);
    g_task_add           = create_button(parent, IDC_TASK_ADD, "Add Task", 255, 202, 100, 32);
    g_task_demo          = create_button(parent, IDC_TASK_DEMO, "Load Demo", 370, 202, 110, 32);
    g_task_pid           = create_edit(parent, IDC_TASK_PID, "1", 65, 245, 70, 25);
    g_task_suspend       = create_button(parent, IDC_TASK_SUSPEND, "Suspend", 155, 243, 95, 32);
    g_task_resume        = create_button(parent, IDC_TASK_RESUME, "Resume", 260, 243, 95, 32);
    g_task_terminate     = create_button(parent, IDC_TASK_TERMINATE, "Terminate", 365, 243, 110, 32);
}

static void build_sched_panel(HWND parent) {
    g_sched_group        = create_group_box(parent, "Run Scheduling Algorithms", 10, 10, 540, 145);
    g_sched_desc         = create_label(parent, "Execute a scheduling algorithm on READY processes.", 30, 42, 350, 22, NULL);
    g_sched_fcfs         = create_button(parent, IDC_SCHED_FCFS, "FCFS", 30, 78, 100, 34);
    g_sched_sjf          = create_button(parent, IDC_SCHED_SJF, "SJF", 140, 78, 100, 34);
    g_sched_priority     = create_button(parent, IDC_SCHED_PRIORITY, "Priority", 250, 78, 120, 34);
    g_sched_compare      = create_button(parent, IDC_SCHED_COMPARE, "Compare All", 380, 78, 130, 34);
    g_sched_lbl_quantum  = create_label(parent, "Quantum", 30, 117, 60, 22, NULL);
    g_sched_quantum      = create_edit(parent, IDC_SCHED_QUANTUM, "2", 95, 115, 50, 25);
    g_sched_rr           = create_button(parent, IDC_SCHED_RR, "Round Robin", 155, 112, 130, 34);
    g_sched_show_summary = create_button(parent, IDC_SCHED_SUMMARY, "Summary", 295, 112, 110, 34);
    g_sched_show_compute = create_button(parent, IDC_SCHED_COMPUTE, "Compute", 415, 112, 100, 34);

    g_sched_chart_group  = create_group_box(parent, "Scheduling Visualization", 10, 165, 540, 220);
    g_sched_metrics      = create_label(parent,
                                        "No schedule run yet. Run FCFS, SJF, Priority, or Round Robin to generate the chart and computations.",
                                        30, 193, 640, 40, g_small_font);
    g_sched_chart        = create_chart(parent, 30, 240, 610, 125);
}

static void build_resource_panel(HWND parent) {
    g_res_group          = create_group_box(parent, "Request or Release Resources", 10, 10, 500, 225);
    g_res_lbl_pid        = create_label(parent, "PID", 30, 45, 35, 22, NULL);
    g_res_lbl_c          = create_label(parent, "Communication Channels", 30, 85, 150, 22, NULL);
    g_res_lbl_v          = create_label(parent, "Vehicles", 30, 123, 70, 22, NULL);
    g_res_lbl_s          = create_label(parent, "Staff Units", 30, 161, 70, 22, NULL);

    g_res_pid            = create_edit(parent, IDC_RES_PID, "1", 190, 43, 60, 25);
    g_res_reqc           = create_edit(parent, IDC_RES_REQC, "1", 190, 83, 60, 25);
    g_res_reqv           = create_edit(parent, IDC_RES_REQV, "0", 190, 121, 60, 25);
    g_res_reqs           = create_edit(parent, IDC_RES_REQS, "1", 190, 159, 60, 25);

    g_res_request        = create_button(parent, IDC_RES_REQUEST, "Request Resources", 310, 82, 150, 34);
    g_res_release        = create_button(parent, IDC_RES_RELEASE, "Release Resources", 310, 126, 150, 34);
    g_res_check          = create_button(parent, IDC_RES_CHECK, "Check Safety", 310, 170, 150, 34);
}

static void build_ipc_panel(HWND parent) {
    g_ipc_group          = create_group_box(parent, "Inter-Process Communication", 10, 10, 500, 170);
    g_ipc_lbl_from       = create_label(parent, "From PID", 30, 45, 60, 22, NULL);
    g_ipc_lbl_to         = create_label(parent, "To PID", 180, 45, 50, 22, NULL);
    g_ipc_lbl_text       = create_label(parent, "Message", 30, 90, 60, 22, NULL);

    g_ipc_from           = create_edit(parent, IDC_IPC_FROM, "1", 100, 43, 60, 25);
    g_ipc_to             = create_edit(parent, IDC_IPC_TO, "2", 240, 43, 60, 25);
    g_ipc_text           = create_edit(parent, IDC_IPC_TEXT, "Coordinate traffic diversion.", 100, 88, 320, 25);
    g_ipc_send           = create_button(parent, IDC_IPC_SEND, "Send IPC Message", 100, 127, 150, 34);
}

static void build_system_panel(HWND parent) {
    g_sys_group          = create_group_box(parent, "System Utilities", 10, 10, 500, 170);
    g_sys_desc           = create_label(parent, "Inspect status, read logs, clear output, or reset the simulator state.", 30, 45, 420, 22, NULL);
    g_sys_status         = create_button(parent, IDC_SYS_STATUS, "View Status", 30, 90, 110, 34);
    g_sys_logs           = create_button(parent, IDC_SYS_LOGS, "View Logs", 150, 90, 110, 34);
    g_sys_clear          = create_button(parent, IDC_SYS_CLEAR, "Clear Output", 270, 90, 110, 34);
    g_sys_reset          = create_button(parent, IDC_SYS_RESET, "Reset System", 390, 90, 110, 34);
}

static void build_summary_pane(HWND hwnd) {
    LVCOLUMNA col;

    g_summary_group = create_group_box(hwnd, "Live Process Summary", 720, 65, 250, 305);
    g_stats_label = create_label(hwnd, "No processes loaded.", 735, 90, 220, 40, g_small_font);

    g_process_list = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        735, 135, 220, 215,
        hwnd,
        (HMENU)(INT_PTR)IDC_PROCESS_LIST,
        GetModuleHandle(NULL),
        NULL
    );
    set_control_font(g_process_list, g_small_font);

    ListView_SetExtendedListViewStyle(g_process_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    ListView_SetBkColor(g_process_list, COLOR_LIST_BG);
    ListView_SetTextBkColor(g_process_list, COLOR_LIST_BG);
    ListView_SetTextColor(g_process_list, COLOR_LIST_TEXT);

    memset(&col, 0, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.pszText = "PID"; col.cx = 40; col.iSubItem = 0;
    ListView_InsertColumn(g_process_list, 0, &col);

    col.pszText = "Name"; col.cx = 105; col.iSubItem = 1;
    ListView_InsertColumn(g_process_list, 1, &col);

    col.pszText = "State"; col.cx = 90; col.iSubItem = 2;
    ListView_InsertColumn(g_process_list, 2, &col);

    col.pszText = "Pr"; col.cx = 34; col.iSubItem = 3;
    ListView_InsertColumn(g_process_list, 3, &col);

    col.pszText = "Mem"; col.cx = 50; col.iSubItem = 4;
    ListView_InsertColumn(g_process_list, 4, &col);
}

static void build_main_ui(HWND hwnd) {
    create_label(hwnd, "SERC Mini-OS", 18, 18, 170, 30, g_title_font);
    g_section_title = create_label(hwnd, "Task Management", 185, 22, 420, 28, g_title_font);

    g_sidebar_group = create_group_box(hwnd, "Menu", 15, 65, 145, 305);
    g_nav_tasks = create_nav_button(hwnd, IDC_NAV_TASKS, "Task Mgmt", 35, 102, 105, 36);
    g_nav_sched = create_nav_button(hwnd, IDC_NAV_SCHED, "Scheduling", 35, 146, 105, 36);
    g_nav_resources = create_nav_button(hwnd, IDC_NAV_RESOURCES, "Resources", 35, 190, 105, 36);
    g_nav_ipc = create_nav_button(hwnd, IDC_NAV_IPC, "IPC", 35, 234, 105, 36);
    g_nav_system = create_nav_button(hwnd, IDC_NAV_SYSTEM, "System", 35, 278, 105, 36);

    g_panel_tasks = create_panel(hwnd, 180, 65, 520, 305);
    g_panel_sched = create_panel(hwnd, 180, 65, 520, 305);
    g_panel_resources = create_panel(hwnd, 180, 65, 520, 305);
    g_panel_ipc = create_panel(hwnd, 180, 65, 520, 305);
    g_panel_system = create_panel(hwnd, 180, 65, 520, 305);

    build_task_panel(g_panel_tasks);
    build_sched_panel(g_panel_sched);
    build_resource_panel(g_panel_resources);
    build_ipc_panel(g_panel_ipc);
    build_system_panel(g_panel_system);

    build_summary_pane(hwnd);

    g_output_group = create_group_box(hwnd, "Output / Status", 15, 390, 955, 255);
    g_output = create_output_box(hwnd, IDC_OUTPUT, 30, 415, 925, 210);
}

/* ---------------- Layout ---------------- */

static void layout_task_panel(int w, int h) {
    int innerW = w - 20;
    int valueW = 70;
    int top2 = 170;

    set_bounds(g_task_group_create, 10, 10, innerW, 150);
    set_bounds(g_task_group_actions, 10, top2, innerW, h - top2 - 10);

    set_bounds(g_task_lbl_name, 30, 40, 80, 22);
    set_bounds(g_task_name, 120, 38, max(150, innerW / 3), 25);

    set_bounds(g_task_lbl_service, innerW / 2 + 10, 40, 60, 22);
    set_bounds(g_task_type, innerW / 2 + 80, 38, max(120, innerW / 4), 150);

    set_bounds(g_task_lbl_burst, 30, 80, 50, 22);
    set_bounds(g_task_burst, 120, 78, valueW, 25);

    set_bounds(g_task_lbl_priority, 220, 80, 55, 22);
    set_bounds(g_task_priority, 285, 78, valueW, 25);

    set_bounds(g_task_lbl_memory, innerW / 2 + 10, 80, 55, 22);
    set_bounds(g_task_memory, innerW / 2 + 80, 78, valueW + 15, 25);

    set_bounds(g_task_lbl_maxc, 30, 118, 75, 22);
    set_bounds(g_task_maxc, 120, 116, 55, 25);

    set_bounds(g_task_lbl_maxv, 200, 118, 85, 22);
    set_bounds(g_task_maxv, 295, 116, 55, 25);

    set_bounds(g_task_lbl_maxs, innerW / 2 + 10, 118, 65, 22);
    set_bounds(g_task_maxs, innerW / 2 + 80, 116, 55, 25);

    set_bounds(g_task_lbl_strategy, 30, top2 + 35, 60, 22);
    set_bounds(g_task_strategy, 95, top2 + 33, max(130, innerW / 4), 150);

    set_bounds(g_task_add, innerW / 2 - 10, top2 + 32, 100, 32);
    set_bounds(g_task_demo, innerW / 2 + 105, top2 + 32, 110, 32);

    set_bounds(g_task_lbl_pid, 30, top2 + 78, 30, 22);
    set_bounds(g_task_pid, 65, top2 + 76, 70, 25);
    set_bounds(g_task_suspend, 155, top2 + 74, 95, 32);
    set_bounds(g_task_resume, 260, top2 + 74, 95, 32);
    set_bounds(g_task_terminate, 365, top2 + 74, 110, 32);
}

static void layout_sched_panel(int w, int h) {
    int innerW = w - 20;
    int chartTop = 165;
    int chartHeight = h - chartTop - 10;
    if (chartHeight < 180) {
        chartHeight = 180;
    }

    set_bounds(g_sched_group, 10, 10, innerW, 145);
    set_bounds(g_sched_desc, 30, 42, innerW - 60, 22);
    set_bounds(g_sched_fcfs, 30, 78, 100, 34);
    set_bounds(g_sched_sjf, 140, 78, 100, 34);
    set_bounds(g_sched_priority, 250, 78, 120, 34);
    set_bounds(g_sched_compare, max(380, innerW - 140), 78, 130, 34);
    set_bounds(g_sched_lbl_quantum, 30, 117, 60, 22);
    set_bounds(g_sched_quantum, 95, 115, 50, 25);
    set_bounds(g_sched_rr, 155, 112, 130, 34);
    set_bounds(g_sched_show_summary, 295, 112, 110, 34);
    set_bounds(g_sched_show_compute, max(415, innerW - 115), 112, 100, 34);

    set_bounds(g_sched_chart_group, 10, chartTop, innerW, chartHeight);
    set_bounds(g_sched_metrics, 30, chartTop + 25, innerW - 40, 40);
    set_bounds(g_sched_chart, 30, chartTop + 70, innerW - 40, chartHeight - 90);
}

static void layout_resource_panel(int w, int h) {
    int innerW = w - 20;
    (void)h;

    set_bounds(g_res_group, 10, 10, innerW, 225);
    set_bounds(g_res_lbl_pid, 30, 45, 35, 22);
    set_bounds(g_res_pid, 190, 43, 60, 25);
    set_bounds(g_res_lbl_c, 30, 85, 150, 22);
    set_bounds(g_res_reqc, 190, 83, 60, 25);
    set_bounds(g_res_lbl_v, 30, 123, 70, 22);
    set_bounds(g_res_reqv, 190, 121, 60, 25);
    set_bounds(g_res_lbl_s, 30, 161, 70, 22);
    set_bounds(g_res_reqs, 190, 159, 60, 25);
    set_bounds(g_res_request, max(280, innerW - 180), 82, 150, 34);
    set_bounds(g_res_release, max(280, innerW - 180), 126, 150, 34);
    set_bounds(g_res_check, max(280, innerW - 180), 170, 150, 34);
}

static void layout_ipc_panel(int w, int h) {
    int innerW = w - 20;
    int msgW = max(220, innerW - 180);
    (void)h;

    set_bounds(g_ipc_group, 10, 10, innerW, 170);
    set_bounds(g_ipc_lbl_from, 30, 45, 60, 22);
    set_bounds(g_ipc_from, 100, 43, 60, 25);
    set_bounds(g_ipc_lbl_to, 180, 45, 50, 22);
    set_bounds(g_ipc_to, 240, 43, 60, 25);
    set_bounds(g_ipc_lbl_text, 30, 90, 60, 22);
    set_bounds(g_ipc_text, 100, 88, msgW, 25);
    set_bounds(g_ipc_send, 100, 127, 150, 34);
}

static void layout_system_panel(int w, int h) {
    int innerW = w - 20;
    int btnW = 110;
    int gap = 10;
    (void)h;

    set_bounds(g_sys_group, 10, 10, innerW, 170);
    set_bounds(g_sys_desc, 30, 45, innerW - 60, 22);
    set_bounds(g_sys_status, 30, 90, btnW, 34);
    set_bounds(g_sys_logs, 30 + btnW + gap, 90, btnW, 34);
    set_bounds(g_sys_clear, 30 + (btnW + gap) * 2, 90, btnW, 34);
    set_bounds(g_sys_reset, 30 + (btnW + gap) * 3, 90, btnW, 34);
}

static void layout_main(HWND hwnd) {
    RECT rc;
    int cw, ch;
    int margin = 15;
    int topY = 65;
    int sidebarW = 145;
    int gap = 15;
    int outputH;
    int topAreaH;
    int contentX, contentW;
    int summaryW;
    int summaryX;

    GetClientRect(hwnd, &rc);
    cw = rc.right - rc.left;
    ch = rc.bottom - rc.top;

    if (cw < 1040) cw = 1040;
    if (ch < 720) ch = 720;

    outputH = ch / 3;
    if (outputH < 210) outputH = 210;
    if (outputH > 300) outputH = 300;

    topAreaH = ch - topY - outputH - 40;
    if (topAreaH < 330) topAreaH = 330;

    contentX = margin + sidebarW + gap + 10;
    summaryW = cw / 3;
    if (summaryW < 300) summaryW = 300;
    if (summaryW > 390) summaryW = 390;

    contentW = cw - contentX - summaryW - gap - margin;
    if (contentW < 560) {
        contentW = 560;
        summaryW = cw - contentX - contentW - gap - margin;
        if (summaryW < 280) summaryW = 280;
    }

    summaryX = contentX + contentW + gap;

    set_bounds(g_sidebar_group, 15, topY, sidebarW, topAreaH);
    set_bounds(g_nav_tasks, 35, topY + 37, 105, 36);
    set_bounds(g_nav_sched, 35, topY + 81, 105, 36);
    set_bounds(g_nav_resources, 35, topY + 125, 105, 36);
    set_bounds(g_nav_ipc, 35, topY + 169, 105, 36);
    set_bounds(g_nav_system, 35, topY + 213, 105, 36);

    set_bounds(g_section_title, contentX, 22, 500, 28);

    set_bounds(g_panel_tasks, contentX, topY, contentW, topAreaH);
    set_bounds(g_panel_sched, contentX, topY, contentW, topAreaH);
    set_bounds(g_panel_resources, contentX, topY, contentW, topAreaH);
    set_bounds(g_panel_ipc, contentX, topY, contentW, topAreaH);
    set_bounds(g_panel_system, contentX, topY, contentW, topAreaH);

    layout_task_panel(contentW, topAreaH);
    layout_sched_panel(contentW, topAreaH);
    layout_resource_panel(contentW, topAreaH);
    layout_ipc_panel(contentW, topAreaH);
    layout_system_panel(contentW, topAreaH);

    set_bounds(g_summary_group, summaryX, topY, summaryW, topAreaH);
    set_bounds(g_stats_label, summaryX + 15, topY + 25, summaryW - 30, 40);
    set_bounds(g_process_list, summaryX + 15, topY + 70, summaryW - 30, topAreaH - 90);

    set_bounds(g_output_group, 15, topY + topAreaH + 15, cw - 30, outputH);
    set_bounds(g_output, 30, topY + topAreaH + 40, cw - 60, outputH - 45);

    ListView_SetColumnWidth(g_process_list, 0, 40);
    ListView_SetColumnWidth(g_process_list, 1, max(110, summaryW / 3));
    ListView_SetColumnWidth(g_process_list, 2, max(84, summaryW / 4));
    ListView_SetColumnWidth(g_process_list, 3, 38);
    ListView_SetColumnWidth(g_process_list, 4, max(52, summaryW / 6));
}

/* ---------------- Actions ---------------- */

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
    update_process_summary();
    refresh_schedule_visuals();
}

static void do_load_demo(void) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_load_demo_data(msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
    refresh_schedule_visuals();
}

static void do_suspend(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_suspend_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
    refresh_schedule_visuals();
}

static void do_resume(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_resume_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
    refresh_schedule_visuals();
}

static void do_terminate(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_terminate_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
    refresh_schedule_visuals();
}

static void do_run_scheduler(SchedulerType algorithm, int quantum) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_run_scheduler(algorithm, quantum, msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
    refresh_schedule_visuals();
}

static void do_compare_schedulers(void) {
    char msg[32768];
    if (!validate_non_negative_int(g_panel_sched, IDC_SCHED_QUANTUM, "Quantum")) return;
    memset(msg, 0, sizeof(msg));
    serc_compare_schedulers(get_int_value(g_panel_sched, IDC_SCHED_QUANTUM), msg, sizeof(msg));
    replace_output(msg);
    update_process_summary();
    refresh_schedule_visuals();
}

static void do_show_schedule_summary(void) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_get_last_schedule_summary(msg, sizeof(msg));
    replace_output(msg);
    refresh_schedule_visuals();
}

static void do_show_schedule_computation(void) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_get_last_schedule_computation(msg, sizeof(msg));
    replace_output(msg);
    refresh_schedule_visuals();
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
    update_process_summary();
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
    update_process_summary();
}

static void do_check_deadlock_safety(void) {
    char msg[4096];
    memset(msg, 0, sizeof(msg));
    serc_check_deadlock_safety(msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
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
    update_process_summary();
}

static void do_reset_system(void) {
    serc_init();
    append_output("System reset completed.");
    show_status_report();
    update_process_summary();
    refresh_schedule_visuals();
}

static LRESULT handle_process_list_custom_draw(LPARAM lParam) {
    LPNMLVCUSTOMDRAW draw = (LPNMLVCUSTOMDRAW)lParam;
    PCB *table;
    int index;

    if (draw == NULL || draw->nmcd.hdr.idFrom != IDC_PROCESS_LIST) {
        return 0;
    }

    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        return CDRF_NOTIFYITEMDRAW;
    }

    if (draw->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) {
        return CDRF_DODEFAULT;
    }

    index = (int)draw->nmcd.dwItemSpec;
    table = get_processes();
    if (index >= 0 && index < get_process_count()) {
        ProcessState state = table[index].state;
        draw->clrText = state_text_color(state);
        draw->clrTextBk = state_back_color(state);
        return CDRF_NEWFONT;
    }

    return CDRF_DODEFAULT;
}

/* ---------------- Custom drawing ---------------- */

static void draw_nav_button(LPDRAWITEMSTRUCT dis) {
    RECT rc = dis->rcItem;
    COLORREF fill = is_active_nav((int)dis->CtlID) ? COLOR_NAV_ACTIVE : COLOR_NAV;
    COLORREF border = fill;
    char caption[128];
    HBRUSH brush;
    HPEN pen, old_pen;
    HFONT old_font;

    if (dis->itemState & ODS_SELECTED) {
        int r = max(0, (int)GetRValue(fill) - 18);
        int g = max(0, (int)GetGValue(fill) - 18);
        int b = max(0, (int)GetBValue(fill) - 18);
        fill = RGB(r, g, b);
        border = fill;
    }

    brush = CreateSolidBrush(fill);
    FillRect(dis->hDC, &rc, brush);
    DeleteObject(brush);

    pen = CreatePen(PS_SOLID, 1, border);
    old_pen = (HPEN)SelectObject(dis->hDC, pen);
    SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
    Rectangle(dis->hDC, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(dis->hDC, old_pen);
    DeleteObject(pen);

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, RGB(255, 255, 255));

    GetWindowTextA(dis->hwndItem, caption, sizeof(caption));
    old_font = (HFONT)SelectObject(dis->hDC, g_font);
    DrawTextA(dis->hDC, caption, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dis->hDC, old_font);

    if (dis->itemState & ODS_FOCUS) {
        RECT focus = rc;
        InflateRect(&focus, -4, -4);
        DrawFocusRect(dis->hDC, &focus);
    }
}

static COLORREF color_for_pid(int pid) {
    int count = (int)(sizeof(CHART_COLORS) / sizeof(CHART_COLORS[0]));
    if (count == 0) {
        return RGB(52, 152, 219);
    }
    if (pid < 0) {
        pid = -pid;
    }
    if (pid > 0) {
        pid--;
    }
    return CHART_COLORS[pid % count];
}

static void draw_schedule_chart(HWND hwnd, HDC hdc) {
    RECT rc;
    ScheduleResult last;
    HBRUSH bg_brush;
    HPEN axis_pen, grid_pen, old_pen;
    HFONT old_font;
    int i;
    int left_margin = 35;
    int right_margin = 25;
    int top_margin = 52;
    int bottom_margin = 48;
    int chart_h;
    int usable_w;
    int total_time = 0;

    GetClientRect(hwnd, &rc);

    bg_brush = CreateSolidBrush(COLOR_CHART_BG);
    FillRect(hdc, &rc, bg_brush);
    DeleteObject(bg_brush);
    {
        HPEN border_pen = CreatePen(PS_SOLID, 1, COLOR_CHART_BORDER);
        HPEN old_border_pen = (HPEN)SelectObject(hdc, border_pen);
        HBRUSH old_border_brush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, old_border_brush);
        SelectObject(hdc, old_border_pen);
        DeleteObject(border_pen);
    }

    if (!serc_has_last_schedule() || !serc_copy_last_schedule(&last) || last.segment_count <= 0) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_SUBTEXT);
        old_font = (HFONT)SelectObject(hdc, g_small_font);
        DrawTextA(hdc,
                  "Run a scheduling algorithm to display the Gantt chart here.",
                  -1,
                  &rc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old_font);
        return;
    }

    total_time = last.total_time;
    if (total_time <= 0) {
        total_time = last.segments[last.segment_count - 1].end_time;
    }
    if (total_time <= 0) {
        total_time = 1;
    }

    usable_w = (rc.right - rc.left) - left_margin - right_margin;
    chart_h = (rc.bottom - rc.top) - top_margin - bottom_margin;

    if (usable_w < 80 || chart_h < 44) {
        return;
    }

    axis_pen = CreatePen(PS_SOLID, 1, COLOR_CHART_AXIS);
    grid_pen = CreatePen(PS_DOT, 1, COLOR_CHART_GRID);
    old_pen = (HPEN)SelectObject(hdc, axis_pen);

    SelectObject(hdc, grid_pen);
    for (i = 0; i <= 4; i++) {
        int x = left_margin + (usable_w * i) / 4;
        MoveToEx(hdc, x, top_margin + 3, NULL);
        LineTo(hdc, x, top_margin + chart_h);
    }

    SelectObject(hdc, axis_pen);
    MoveToEx(hdc, left_margin, top_margin + chart_h, NULL);
    LineTo(hdc, left_margin + usable_w, top_margin + chart_h);

    old_font = (HFONT)SelectObject(hdc, g_chart_font ? g_chart_font : g_small_font);
    SetBkMode(hdc, TRANSPARENT);

    for (i = 0; i < last.segment_count; i++) {
        ScheduleSegment seg = last.segments[i];
        int x1 = left_margin + (seg.start_time * usable_w) / total_time;
        int x2 = left_margin + (seg.end_time * usable_w) / total_time;
        RECT block;
        RECT shadow;
        HBRUSH block_brush, old_brush, shadow_brush;
        HPEN block_pen;
        HPEN old_block_pen;
        char label[32];
        char time_start[16];
        char time_end[16];
        RECT text_rc;
        SIZE sz_end;

        if (x2 <= x1) {
            x2 = x1 + 8;
        }

        block.left   = x1;
        block.top    = top_margin + 10;
        block.right  = x2;
        block.bottom = top_margin + chart_h - 12;

        shadow = block;
        OffsetRect(&shadow, 2, 3);
        shadow_brush = CreateSolidBrush(RGB(210, 218, 230));
        FillRect(hdc, &shadow, shadow_brush);
        DeleteObject(shadow_brush);

        block_brush = CreateSolidBrush(color_for_pid(seg.pid));
        block_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        old_brush = (HBRUSH)SelectObject(hdc, block_brush);
        old_block_pen = (HPEN)SelectObject(hdc, block_pen);
        RoundRect(hdc, block.left, block.top, block.right, block.bottom, 9, 9);
        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_block_pen);
        DeleteObject(block_brush);
        DeleteObject(block_pen);

        snprintf(label, sizeof(label), "P%d", seg.pid);
        text_rc = block;
        SetTextColor(hdc, RGB(255, 255, 255));
        if (block.right - block.left >= 24) {
            DrawTextA(hdc, label, -1, &text_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        SetTextColor(hdc, COLOR_CHART_AXIS);
        snprintf(time_start, sizeof(time_start), "%d", seg.start_time);
        if (i == 0 || block.right - block.left > 34) {
            TextOutA(hdc, x1 - 4, top_margin + chart_h + 7, time_start, (int)strlen(time_start));
        }

        if (i == last.segment_count - 1) {
            snprintf(time_end, sizeof(time_end), "%d", seg.end_time);
            GetTextExtentPoint32A(hdc, time_end, (int)strlen(time_end), &sz_end);
            TextOutA(hdc, x2 - sz_end.cx / 2, top_margin + chart_h + 7, time_end, (int)strlen(time_end));
        }
    }

    SetTextColor(hdc, COLOR_TITLE);
    {
        RECT title_rc = rc;
        char title[128];
        snprintf(title, sizeof(title), "Gantt Chart - %s | total time %d | %d segment(s)",
                 scheduler_to_string(last.type),
                 total_time,
                 last.segment_count);
        title_rc.left = left_margin;
        title_rc.top = 8;
        title_rc.right = rc.right - 10;
        title_rc.bottom = 32;
        DrawTextA(hdc, title, -1, &title_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    {
        int legend_x = max(left_margin, rc.right - 235);
        int legend_y = 34;
        int seen[MAX_PROCESSES];
        int seen_count = 0;
        memset(seen, 0, sizeof(seen));

        for (i = 0; i < last.segment_count && seen_count < 5; i++) {
            int pid = last.segments[i].pid;
            int found = 0;
            int j;

            for (j = 0; j < seen_count; j++) {
                if (seen[j] == pid) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                RECT swatch;
                char label[16];
                HBRUSH legend_brush;

                seen[seen_count++] = pid;
                swatch.left = legend_x;
                swatch.top = legend_y;
                swatch.right = legend_x + 12;
                swatch.bottom = legend_y + 12;

                legend_brush = CreateSolidBrush(color_for_pid(pid));
                FillRect(hdc, &swatch, legend_brush);
                DeleteObject(legend_brush);

                snprintf(label, sizeof(label), "P%d", pid);
                SetTextColor(hdc, COLOR_TEXT);
                TextOutA(hdc, legend_x + 16, legend_y - 1, label, (int)strlen(label));
                legend_x += 43;
            }
        }
    }

    SelectObject(hdc, old_font);
    SelectObject(hdc, old_pen);
    DeleteObject(grid_pen);
    DeleteObject(axis_pen);
}

/* ---------------- Panel / chart proc ---------------- */

static LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            SendMessageA(GetParent(hwnd), WM_COMMAND, wParam, lParam);
            return 0;

        case WM_NOTIFY:
            SendMessageA(GetParent(hwnd), WM_NOTIFY, wParam, lParam);
            return 0;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, COLOR_TEXT);
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)g_brush_panel;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, COLOR_TEXT);
            SetBkColor(hdc, COLOR_PANEL);
            return (LRESULT)g_brush_panel;
        }

        case WM_ERASEBKGND: {
            RECT rc;
            HDC hdc = (HDC)wParam;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, g_brush_panel);
            return 1;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK ChartProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            draw_schedule_chart(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ---------------- Init / cleanup ---------------- */

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
        28, 0, 0, 0, FW_BOLD,
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

    g_small_font = CreateFontA(
        15, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Segoe UI"
    );

    g_chart_font = CreateFontA(
        15, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Segoe UI"
    );
}

static void init_brushes(void) {
    g_brush_bg = CreateSolidBrush(COLOR_BG);
    g_brush_panel = CreateSolidBrush(COLOR_PANEL);
    g_brush_output = CreateSolidBrush(COLOR_OUTPUT);
    g_brush_sidebar = CreateSolidBrush(COLOR_SIDEBAR);
    g_brush_chart = CreateSolidBrush(COLOR_CHART_BG);
}

static void destroy_fonts(void) {
    if (g_font) DeleteObject(g_font);
    if (g_title_font) DeleteObject(g_title_font);
    if (g_mono_font) DeleteObject(g_mono_font);
    if (g_small_font) DeleteObject(g_small_font);
    if (g_chart_font) DeleteObject(g_chart_font);
    g_font = NULL;
    g_title_font = NULL;
    g_mono_font = NULL;
    g_small_font = NULL;
    g_chart_font = NULL;
}

static void destroy_brushes(void) {
    if (g_brush_bg) DeleteObject(g_brush_bg);
    if (g_brush_panel) DeleteObject(g_brush_panel);
    if (g_brush_output) DeleteObject(g_brush_output);
    if (g_brush_sidebar) DeleteObject(g_brush_sidebar);
    if (g_brush_chart) DeleteObject(g_brush_chart);
    g_brush_bg = NULL;
    g_brush_panel = NULL;
    g_brush_output = NULL;
    g_brush_sidebar = NULL;
    g_brush_chart = NULL;
}

/* ---------------- Main window proc ---------------- */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            init_fonts();
            init_brushes();
            build_main_ui(hwnd);
            serc_init();
            switch_section(SECTION_TASKS);
            replace_output("SERC Mini-OS GUI started.\r\n\r\nResize the window freely. The layout, output area, live process summary, and scheduling chart will adapt.");
            update_process_summary();
            refresh_schedule_visuals();
            SetTimer(hwnd, IDT_REFRESH_TIMER, 1000, NULL);
            layout_main(hwnd);
            return 0;

        case WM_SIZE:
            layout_main(hwnd);
            return 0;

        case WM_TIMER:
            if (wParam == IDT_REFRESH_TIMER) {
                update_process_summary();
                refresh_schedule_visuals();
                return 0;
            }
            break;

        case WM_DRAWITEM:
            if (is_nav_button((int)((LPDRAWITEMSTRUCT)lParam)->CtlID)) {
                draw_nav_button((LPDRAWITEMSTRUCT)lParam);
                return TRUE;
            }
            break;

        case WM_NOTIFY: {
            LPNMHDR hdr = (LPNMHDR)lParam;
            if (hdr != NULL && hdr->idFrom == IDC_PROCESS_LIST && hdr->code == NM_CUSTOMDRAW) {
                return handle_process_list_custom_draw(lParam);
            }
            break;
        }

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
                case IDC_SCHED_COMPARE:
                    do_compare_schedulers();
                    return 0;
                case IDC_SCHED_SUMMARY:
                    do_show_schedule_summary();
                    return 0;
                case IDC_SCHED_COMPUTE:
                    do_show_schedule_computation();
                    return 0;

                case IDC_RES_REQUEST:    do_request_resources(); return 0;
                case IDC_RES_RELEASE:    do_release_resources(); return 0;
                case IDC_RES_CHECK:      do_check_deadlock_safety(); return 0;

                case IDC_IPC_SEND:       do_send_ipc(); return 0;

                case IDC_SYS_STATUS:     show_status_report(); update_process_summary(); refresh_schedule_visuals(); return 0;
                case IDC_SYS_LOGS:       show_logs_report(); update_process_summary(); refresh_schedule_visuals(); return 0;
                case IDC_SYS_CLEAR:      replace_output(""); return 0;
                case IDC_SYS_RESET:      do_reset_system(); return 0;
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND control = (HWND)lParam;
            SetBkMode(hdc, TRANSPARENT);

            if (control == g_section_title) {
                SetTextColor(hdc, COLOR_TITLE);
                return (LRESULT)g_brush_bg;
            }

            if (control == g_stats_label) {
                SetTextColor(hdc, COLOR_SUBTEXT);
                return (LRESULT)g_brush_bg;
            }

            SetTextColor(hdc, COLOR_TEXT);
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
            RECT sidebar;
            HDC hdc = (HDC)wParam;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, g_brush_bg);

            sidebar.left = 15;
            sidebar.top = 65;
            sidebar.right = 160;
            sidebar.bottom = rc.bottom - 15;
            FillRect(hdc, &sidebar, g_brush_sidebar);
            return 1;
        }

        case WM_DESTROY:
            KillTimer(hwnd, IDT_REFRESH_TIMER);
            destroy_fonts();
            destroy_brushes();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ---------------- WinMain ---------------- */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc;
    WNDCLASSA panel_wc;
    WNDCLASSA chart_wc;
    MSG msg;
    HWND hwnd;
    INITCOMMONCONTROLSEX icex;

    (void)hPrevInstance;
    (void)lpCmdLine;

    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Failed to register main window class.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    memset(&panel_wc, 0, sizeof(panel_wc));
    panel_wc.lpfnWndProc = PanelProc;
    panel_wc.hInstance = hInstance;
    panel_wc.lpszClassName = PANEL_CLASS_NAME;
    panel_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    panel_wc.hbrBackground = NULL;

    if (!RegisterClassA(&panel_wc)) {
        MessageBoxA(NULL, "Failed to register panel class.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    memset(&chart_wc, 0, sizeof(chart_wc));
    chart_wc.lpfnWndProc = ChartProc;
    chart_wc.hInstance = hInstance;
    chart_wc.lpszClassName = CHART_CLASS_NAME;
    chart_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    chart_wc.hbrBackground = NULL;

    if (!RegisterClassA(&chart_wc)) {
        MessageBoxA(NULL, "Failed to register chart class.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    hwnd = CreateWindowA(
        APP_CLASS_NAME,
        APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1220, 800,
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
