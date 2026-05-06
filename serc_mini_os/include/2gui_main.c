#ifdef _WIN32

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system_state.h"
#include "logger.h"

#pragma comment(lib, "comctl32.lib")

#define APP_CLASS_NAME   "SercMiniOSWindow"
#define PANEL_CLASS_NAME "SercMiniOSPanel"
#define APP_TITLE        "SERC Mini-OS - Smart Emergency Response Center"

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
static HFONT g_small_font = NULL;

static HBRUSH g_brush_bg = NULL;
static HBRUSH g_brush_panel = NULL;
static HBRUSH g_brush_output = NULL;
static HBRUSH g_brush_sidebar = NULL;

static HWND g_output = NULL;
static HWND g_section_title = NULL;
static HWND g_stats_label = NULL;
static HWND g_process_list = NULL;

static HWND g_panel_tasks = NULL;
static HWND g_panel_sched = NULL;
static HWND g_panel_resources = NULL;
static HWND g_panel_ipc = NULL;
static HWND g_panel_system = NULL;

static GuiSection g_current_section = SECTION_TASKS;

static COLORREF COLOR_BG           = RGB(242, 246, 252);
static COLORREF COLOR_PANEL        = RGB(255, 255, 255);
static COLORREF COLOR_OUTPUT       = RGB(248, 250, 253);
static COLORREF COLOR_SIDEBAR      = RGB(23, 43, 77);
static COLORREF COLOR_TITLE        = RGB(20, 37, 63);
static COLORREF COLOR_TEXT         = RGB(36, 50, 66);
static COLORREF COLOR_SUBTEXT      = RGB(92, 107, 122);

static COLORREF COLOR_NAV          = RGB(36, 71, 122);
static COLORREF COLOR_NAV_ACTIVE   = RGB(20, 122, 210);
static COLORREF COLOR_ACTION       = RGB(44, 123, 229);
static COLORREF COLOR_ACTION_HOVER = RGB(28, 110, 212);
static COLORREF COLOR_SYSTEM       = RGB(86, 103, 137);
static COLORREF COLOR_BORDER       = RGB(208, 217, 229);

static int is_nav_button(int id) {
    return id >= IDC_NAV_TASKS && id <= IDC_NAV_SYSTEM;
}

static int is_system_button(int id) {
    return id >= IDC_SYS_STATUS && id <= IDC_SYS_RESET;
}

static int is_active_nav(int id) {
    if (g_current_section == SECTION_TASKS && id == IDC_NAV_TASKS) return 1;
    if (g_current_section == SECTION_SCHEDULING && id == IDC_NAV_SCHED) return 1;
    if (g_current_section == SECTION_RESOURCES && id == IDC_NAV_RESOURCES) return 1;
    if (g_current_section == SECTION_IPC && id == IDC_NAV_IPC) return 1;
    if (g_current_section == SECTION_SYSTEM && id == IDC_NAV_SYSTEM) return 1;
    return 0;
}

static void set_control_font(HWND control, HFONT font) {
    if (control && font) {
        SendMessageA(control, WM_SETFONT, (WPARAM)font, TRUE);
    }
}

static void fill_rect_color(HDC hdc, RECT *rc, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, rc, brush);
    DeleteObject(brush);
}

static LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            SendMessageA(GetParent(hwnd), WM_COMMAND, wParam, lParam);
            return 0;

        case WM_NOTIFY: {
            NMHDR *hdr = (NMHDR *)lParam;
            SendMessageA(GetParent(hwnd), WM_NOTIFY, wParam, (LPARAM)hdr);
            return 0;
        }

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

static HWND create_owner_button(HWND parent, int id, const char *text, int x, int y, int w, int h) {
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
             "Processes: %d  |  Active: %d  |  Ready: %d  |  Waiting: %d  |  Suspended: %d  |  Terminated: %d  |  Memory: %d/%d used",
             count, active, ready, waiting, suspended, terminated, get_memory_used(), TOTAL_MEMORY);
    SetWindowTextA(g_stats_label, stats);
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

static void build_task_panel(HWND parent) {
    const char *types[] = {"AMBULANCE", "FIRE", "POLICE"};
    const char *strategies[] = {"First Fit", "Best Fit", "Worst Fit"};

    create_group_box(parent, "Create Emergency Task", 10, 10, 510, 150);
    create_label(parent, "Task Name", 28, 40, 80, 22, NULL);
    create_edit(parent, IDC_TASK_NAME, "Ambulance Dispatch", 118, 38, 160, 25);

    create_label(parent, "Service", 295, 40, 55, 22, NULL);
    create_combo(parent, IDC_TASK_TYPE, 360, 38, 130, 150, types, 3);

    create_label(parent, "Burst", 28, 78, 50, 22, NULL);
    create_edit(parent, IDC_TASK_BURST, "4", 118, 76, 65, 25);

    create_label(parent, "Priority", 205, 78, 55, 22, NULL);
    create_edit(parent, IDC_TASK_PRIORITY, "1", 270, 76, 65, 25);

    create_label(parent, "Memory", 355, 78, 55, 22, NULL);
    create_edit(parent, IDC_TASK_MEMORY, "120", 420, 76, 70, 25);

    create_label(parent, "Max Comm", 28, 115, 75, 22, NULL);
    create_edit(parent, IDC_TASK_MAXC, "2", 118, 113, 50, 25);

    create_label(parent, "Max Vehicles", 185, 115, 85, 22, NULL);
    create_edit(parent, IDC_TASK_MAXV, "1", 280, 113, 50, 25);

    create_label(parent, "Max Staff", 350, 115, 65, 22, NULL);
    create_edit(parent, IDC_TASK_MAXS, "2", 425, 113, 50, 25);

    create_group_box(parent, "Memory Strategy and Process Actions", 10, 175, 510, 110);
    create_label(parent, "Strategy", 28, 205, 60, 22, NULL);
    create_combo(parent, IDC_TASK_STRATEGY, 95, 203, 140, 150, strategies, 3);

    create_owner_button(parent, IDC_TASK_ADD, "Add Task", 255, 202, 100, 32);
    create_owner_button(parent, IDC_TASK_DEMO, "Load Demo", 370, 202, 110, 32);

    create_label(parent, "PID", 28, 248, 30, 22, NULL);
    create_edit(parent, IDC_TASK_PID, "1", 63, 246, 65, 25);
    create_owner_button(parent, IDC_TASK_SUSPEND, "Suspend", 145, 244, 100, 32);
    create_owner_button(parent, IDC_TASK_RESUME, "Resume", 255, 244, 100, 32);
    create_owner_button(parent, IDC_TASK_TERMINATE, "Terminate", 365, 244, 110, 32);
}

static void build_sched_panel(HWND parent) {
    create_group_box(parent, "Run Scheduling Algorithms", 10, 10, 510, 175);
    create_label(parent, "Execute a CPU scheduling algorithm on all READY processes.", 28, 40, 360, 22, NULL);

    create_owner_button(parent, IDC_SCHED_FCFS, "Run FCFS", 28, 80, 120, 36);
    create_owner_button(parent, IDC_SCHED_SJF, "Run SJF", 163, 80, 120, 36);
    create_owner_button(parent, IDC_SCHED_PRIORITY, "Run Priority", 298, 80, 140, 36);

    create_label(parent, "Round Robin Quantum", 28, 135, 130, 22, NULL);
    create_edit(parent, IDC_SCHED_QUANTUM, "2", 165, 133, 65, 25);
    create_owner_button(parent, IDC_SCHED_RR, "Run RR", 245, 130, 100, 36);
}

static void build_resource_panel(HWND parent) {
    create_group_box(parent, "Request or Release Resources", 10, 10, 510, 200);

    create_label(parent, "PID", 28, 45, 30, 22, NULL);
    create_edit(parent, IDC_RES_PID, "1", 70, 43, 65, 25);

    create_label(parent, "Communication Channels", 28, 85, 150, 22, NULL);
    create_edit(parent, IDC_RES_REQC, "1", 205, 83, 65, 25);

    create_label(parent, "Vehicles", 28, 122, 70, 22, NULL);
    create_edit(parent, IDC_RES_REQV, "0", 205, 120, 65, 25);

    create_label(parent, "Staff Units", 28, 159, 70, 22, NULL);
    create_edit(parent, IDC_RES_REQS, "1", 205, 157, 65, 25);

    create_owner_button(parent, IDC_RES_REQUEST, "Request", 330, 82, 130, 36);
    create_owner_button(parent, IDC_RES_RELEASE, "Release", 330, 126, 130, 36);
}

static void build_ipc_panel(HWND parent) {
    create_group_box(parent, "Inter-Process Communication", 10, 10, 510, 165);

    create_label(parent, "From PID", 28, 45, 60, 22, NULL);
    create_edit(parent, IDC_IPC_FROM, "1", 98, 43, 65, 25);

    create_label(parent, "To PID", 180, 45, 50, 22, NULL);
    create_edit(parent, IDC_IPC_TO, "2", 235, 43, 65, 25);

    create_label(parent, "Message", 28, 88, 60, 22, NULL);
    create_edit(parent, IDC_IPC_TEXT, "Coordinate traffic diversion.", 98, 86, 320, 25);

    create_owner_button(parent, IDC_IPC_SEND, "Send IPC Message", 98, 125, 150, 36);
}

static void build_system_panel(HWND parent) {
    create_group_box(parent, "System Utilities", 10, 10, 510, 165);

    create_label(parent, "Inspect status, logs, clear output, or reset the simulator state.", 28, 45, 420, 22, NULL);

    create_owner_button(parent, IDC_SYS_STATUS, "View Status", 28, 90, 110, 36);
    create_owner_button(parent, IDC_SYS_LOGS, "View Logs", 148, 90, 110, 36);
    create_owner_button(parent, IDC_SYS_CLEAR, "Clear Output", 268, 90, 110, 36);
    create_owner_button(parent, IDC_SYS_RESET, "Reset System", 388, 90, 110, 36);
}

static void build_summary_pane(HWND hwnd) {
    RECT rc;
    LVCOLUMNA col;
    GetClientRect(hwnd, &rc);

    create_group_box(hwnd, "Live Process Summary", 710, 70, 255, 300);
    g_stats_label = create_label(hwnd, "No processes loaded.", 725, 95, 225, 40, g_small_font);

    g_process_list = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        725, 140, 225, 210,
        hwnd,
        (HMENU)(INT_PTR)IDC_PROCESS_LIST,
        GetModuleHandle(NULL),
        NULL
    );
    set_control_font(g_process_list, g_small_font);

    ListView_SetExtendedListViewStyle(g_process_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    memset(&col, 0, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.pszText = "PID"; col.cx = 36; col.iSubItem = 0;
    ListView_InsertColumn(g_process_list, 0, &col);

    col.pszText = "Name"; col.cx = 76; col.iSubItem = 1;
    ListView_InsertColumn(g_process_list, 1, &col);

    col.pszText = "State"; col.cx = 62; col.iSubItem = 2;
    ListView_InsertColumn(g_process_list, 2, &col);

    col.pszText = "Pr"; col.cx = 24; col.iSubItem = 3;
    ListView_InsertColumn(g_process_list, 3, &col);

    col.pszText = "Mem"; col.cx = 40; col.iSubItem = 4;
    ListView_InsertColumn(g_process_list, 4, &col);
}

static void build_main_ui(HWND hwnd) {
    create_label(hwnd, "SERC Mini-OS", 18, 18, 180, 30, g_title_font);
    g_section_title = create_label(hwnd, "Task Management", 180, 22, 420, 26, g_title_font);

    create_group_box(hwnd, "Menu", 15, 70, 140, 300);
    create_owner_button(hwnd, IDC_NAV_TASKS, "Task Mgmt", 33, 105, 105, 36);
    create_owner_button(hwnd, IDC_NAV_SCHED, "Scheduling", 33, 149, 105, 36);
    create_owner_button(hwnd, IDC_NAV_RESOURCES, "Resources", 33, 193, 105, 36);
    create_owner_button(hwnd, IDC_NAV_IPC, "IPC", 33, 237, 105, 36);
    create_owner_button(hwnd, IDC_NAV_SYSTEM, "System", 33, 281, 105, 36);

    g_panel_tasks = create_panel(hwnd, 175, 70, 530, 300);
    g_panel_sched = create_panel(hwnd, 175, 70, 530, 300);
    g_panel_resources = create_panel(hwnd, 175, 70, 530, 300);
    g_panel_ipc = create_panel(hwnd, 175, 70, 530, 300);
    g_panel_system = create_panel(hwnd, 175, 70, 530, 300);

    build_task_panel(g_panel_tasks);
    build_sched_panel(g_panel_sched);
    build_resource_panel(g_panel_resources);
    build_ipc_panel(g_panel_ipc);
    build_system_panel(g_panel_system);

    build_summary_pane(hwnd);

    create_group_box(hwnd, "Output / Status", 15, 390, 950, 255);
    g_output = create_output_box(hwnd, IDC_OUTPUT, 30, 415, 920, 210);
}

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
}

static void do_load_demo(void) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_load_demo_data(msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
}

static void do_suspend(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_suspend_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
}

static void do_resume(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_resume_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
}

static void do_terminate(void) {
    char msg[16384];
    if (!validate_non_negative_int(g_panel_tasks, IDC_TASK_PID, "PID")) return;
    memset(msg, 0, sizeof(msg));
    serc_terminate_task(get_int_value(g_panel_tasks, IDC_TASK_PID), msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
}

static void do_run_scheduler(int algorithm, int quantum) {
    char msg[16384];
    memset(msg, 0, sizeof(msg));
    serc_run_scheduler((SchedulerType)algorithm, quantum, msg, sizeof(msg));
    append_output(msg);
    update_process_summary();
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
}

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
}

static void init_brushes(void) {
    g_brush_bg = CreateSolidBrush(COLOR_BG);
    g_brush_panel = CreateSolidBrush(COLOR_PANEL);
    g_brush_output = CreateSolidBrush(COLOR_OUTPUT);
    g_brush_sidebar = CreateSolidBrush(COLOR_SIDEBAR);
}

static void destroy_fonts(void) {
    if (g_font) DeleteObject(g_font);
    if (g_title_font) DeleteObject(g_title_font);
    if (g_mono_font) DeleteObject(g_mono_font);
    if (g_small_font) DeleteObject(g_small_font);
    g_font = NULL;
    g_title_font = NULL;
    g_mono_font = NULL;
    g_small_font = NULL;
}

static void destroy_brushes(void) {
    if (g_brush_bg) DeleteObject(g_brush_bg);
    if (g_brush_panel) DeleteObject(g_brush_panel);
    if (g_brush_output) DeleteObject(g_brush_output);
    if (g_brush_sidebar) DeleteObject(g_brush_sidebar);
    g_brush_bg = NULL;
    g_brush_panel = NULL;
    g_brush_output = NULL;
    g_brush_sidebar = NULL;
}

static void draw_button(LPDRAWITEMSTRUCT dis) {
    RECT rc = dis->rcItem;
    COLORREF fill = COLOR_ACTION;
    COLORREF border = COLOR_ACTION_HOVER;
    COLORREF text = RGB(255, 255, 255);
    UINT state = dis->itemState;
    char caption[128];
    HFONT old_font;
    HBRUSH brush;
    HPEN pen, old_pen;
    int id = (int)dis->CtlID;

    if (is_nav_button(id)) {
        fill = is_active_nav(id) ? COLOR_NAV_ACTIVE : COLOR_NAV;
        border = fill;
    } else if (is_system_button(id)) {
        fill = COLOR_SYSTEM;
        border = COLOR_SYSTEM;
    }

    if (state & ODS_SELECTED) {
        int r = max(0, GetRValue(fill) - 20);
        int g = max(0, GetGValue(fill) - 20);
        int b = max(0, GetBValue(fill) - 20);
        fill = RGB(r, g, b);
        border = fill;
    }

    brush = CreateSolidBrush(fill);
    FillRect(dis->hDC, &rc, brush);
    DeleteObject(brush);

    pen = CreatePen(PS_SOLID, 1, border);
    old_pen = SelectObject(dis->hDC, pen);
    SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
    Rectangle(dis->hDC, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(dis->hDC, old_pen);
    DeleteObject(pen);

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, text);
    GetWindowTextA(dis->hwndItem, caption, sizeof(caption));
    old_font = SelectObject(dis->hDC, g_font);
    DrawTextA(dis->hDC, caption, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dis->hDC, old_font);

    if (state & ODS_FOCUS) {
        RECT focus = rc;
        InflateRect(&focus, -4, -4);
        DrawFocusRect(dis->hDC, &focus);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            init_fonts();
            init_brushes();
            build_main_ui(hwnd);
            serc_init();
            switch_section(SECTION_TASKS);
            replace_output("SERC Mini-OS GUI started.\r\n\r\nUse the menu on the left, observe the live process summary on the right, and view detailed output below.");
            update_process_summary();
            SetTimer(hwnd, IDT_REFRESH_TIMER, 1000, NULL);
            return 0;

        case WM_TIMER:
            if (wParam == IDT_REFRESH_TIMER) {
                update_process_summary();
                return 0;
            }
            break;

        case WM_DRAWITEM:
            draw_button((LPDRAWITEMSTRUCT)lParam);
            return TRUE;

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

                case IDC_SYS_STATUS:     show_status_report(); update_process_summary(); return 0;
                case IDC_SYS_LOGS:       show_logs_report(); update_process_summary(); return 0;
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
            RECT sidebar = {15, 70, 155, 370};
            HDC hdc = (HDC)wParam;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, g_brush_bg);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc;
    WNDCLASSA panel_wc;
    HWND hwnd;
    MSG msg;
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

    hwnd = CreateWindowA(
        APP_CLASS_NAME,
        APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 710,
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