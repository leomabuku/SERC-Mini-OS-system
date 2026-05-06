#include "raylib.h"

#include "system_state.h"
#include "deadlock.h"
#include "ipc.h"
#include "logger.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_MIN_W 1180
#define APP_MIN_H 740
#define OUTPUT_SIZE 32768
#define FILE_LIST_MAX 64

typedef enum {
    SCREEN_DASHBOARD = 0,
    SCREEN_SCHEDULING,
    SCREEN_MEMORY,
    SCREEN_IPC,
    SCREEN_RESOURCES,
    SCREEN_FILES,
    SCREEN_COUNT
} AppScreen;

typedef struct {
    Rectangle bounds;
    char text[128];
    int active;
    int max_len;
} TextBox;

typedef struct {
    AppScreen screen;
    AppScreen previous_screen;
    float transition;
    float time;
    float gantt_time;
    int gantt_playing;
    float pulse;
    float memory_flash;
    float ipc_anim;
    int ipc_from;
    int ipc_to;
    int service_index;
    MemoryStrategy memory_strategy;
    char output[OUTPUT_SIZE];
    char file_preview[OUTPUT_SIZE];
    char files[FILE_LIST_MAX][128];
    int file_count;
    int selected_file;
    TextBox task_name;
    TextBox task_burst;
    TextBox task_priority;
    TextBox task_memory;
    TextBox task_c;
    TextBox task_v;
    TextBox task_s;
    TextBox quantum;
    TextBox ipc_from_box;
    TextBox ipc_to_box;
    TextBox ipc_message;
    TextBox res_pid;
    TextBox res_c;
    TextBox res_v;
    TextBox res_s;
} AppState;

static const Color COL_BG_TOP = {10, 17, 31, 255};
static const Color COL_BG_BOTTOM = {18, 32, 56, 255};
static const Color COL_PANEL = {24, 36, 60, 235};
static const Color COL_PANEL_DARK = {15, 23, 42, 240};
static const Color COL_BORDER = {59, 130, 246, 90};
static const Color COL_TEXT = {226, 232, 240, 255};
static const Color COL_MUTED = {148, 163, 184, 255};
static const Color COL_BLUE = {56, 189, 248, 255};
static const Color COL_GREEN = {34, 197, 94, 255};
static const Color COL_YELLOW = {250, 204, 21, 255};
static const Color COL_ORANGE = {251, 146, 60, 255};
static const Color COL_RED = {248, 113, 113, 255};
static const Color COL_PURPLE = {168, 85, 247, 255};
static const Color COL_PINK = {244, 114, 182, 255};

static const char *screen_names[SCREEN_COUNT] = {
    "Dashboard", "Scheduling", "Memory", "IPC", "Resources", "Files"
};

static int maxi(int a, int b) { return a > b ? a : b; }
static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static Color with_alpha(Color color, unsigned char alpha) {
    color.a = alpha;
    return color;
}

static Color color_lerp(Color a, Color b, float t) {
    t = clampf(t, 0.0f, 1.0f);
    return (Color) {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static Color state_color(ProcessState state) {
    switch (state) {
        case STATE_NEW: return COL_MUTED;
        case STATE_READY: return COL_GREEN;
        case STATE_RUNNING: return COL_BLUE;
        case STATE_WAITING: return COL_YELLOW;
        case STATE_SUSPENDED: return COL_PURPLE;
        case STATE_TERMINATED: return COL_RED;
        default: return COL_MUTED;
    }
}

static Color pid_color(int pid) {
    Color colors[] = {
        {56, 189, 248, 255}, {34, 197, 94, 255}, {168, 85, 247, 255},
        {250, 204, 21, 255}, {244, 114, 182, 255}, {248, 113, 113, 255},
        {45, 212, 191, 255}, {129, 140, 248, 255}, {251, 146, 60, 255}
    };
    int count = (int)(sizeof(colors) / sizeof(colors[0]));
    if (pid < 1) pid = 1;
    return colors[(pid - 1) % count];
}

static void draw_wrapped_text(const char *text, Rectangle bounds, int font_size, Color color) {
    int len;
    int start = 0;
    int line_start = 0;
    int last_space = -1;
    float y = bounds.y;
    int line_height = font_size + 6;

    if (text == NULL) return;
    len = (int)strlen(text);

    for (int i = 0; i <= len; i++) {
        char c = text[i];
        char line[512];
        int end_line = 0;

        if (c == ' ') last_space = i;

        if (c == '\n' || c == '\0') {
            end_line = i;
        } else {
            int chars = i - line_start + 1;
            int copy = chars < (int)sizeof(line) - 1 ? chars : (int)sizeof(line) - 1;
            memcpy(line, text + line_start, (size_t)copy);
            line[copy] = '\0';
            if (MeasureText(line, font_size) > bounds.width && last_space >= line_start) {
                end_line = last_space;
                i = last_space;
            }
        }

        if (end_line || c == '\0') {
            int copy = end_line - line_start;
            if (copy > (int)sizeof(line) - 1) copy = (int)sizeof(line) - 1;
            if (copy > 0 && y + line_height <= bounds.y + bounds.height) {
                memcpy(line, text + line_start, (size_t)copy);
                line[copy] = '\0';
                DrawText(line, (int)bounds.x, (int)y, font_size, color);
                y += line_height;
            }
            if (c == '\0') break;
            line_start = i + 1;
            start = line_start;
            last_space = -1;
            (void)start;
        }
    }
}

static void draw_background(AppState *app) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float t = app->time;

    DrawRectangleGradientV(0, 0, w, h, COL_BG_TOP, COL_BG_BOTTOM);

    for (int i = 0; i < 24; i++) {
        float x = fmodf((float)(i * 97) + t * (12 + i % 5), (float)w);
        float y = fmodf((float)(i * 53) + sinf(t * 0.7f + i) * 24.0f + t * 3.0f, (float)h);
        Color c = with_alpha(i % 3 == 0 ? COL_BLUE : (i % 3 == 1 ? COL_PURPLE : COL_GREEN), 28);
        DrawCircleGradient((int)x, (int)y, 28.0f + (float)(i % 4) * 8.0f, c, with_alpha(c, 0));
    }
}

static void panel(Rectangle r, const char *title) {
    DrawRectangleRounded(r, 0.08f, 10, COL_PANEL);
    DrawRectangleRoundedLinesEx(r, 0.08f, 10, 1.0f, COL_BORDER);
    if (title != NULL && title[0] != '\0') {
        DrawText(title, (int)r.x + 18, (int)r.y + 14, 20, COL_TEXT);
    }
}

static int button(Rectangle r, const char *label, Color accent) {
    Vector2 mouse = GetMousePosition();
    int hover = CheckCollisionPointRec(mouse, r);
    int pressed = hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    Color fill = hover ? color_lerp(COL_PANEL, accent, 0.28f) : color_lerp(COL_PANEL_DARK, accent, 0.12f);

    DrawRectangleRounded(r, 0.22f, 12, fill);
    DrawRectangleRoundedLinesEx(r, 0.22f, 12, 1.2f, hover ? accent : with_alpha(accent, 140));
    DrawText(label, (int)(r.x + r.width / 2 - MeasureText(label, 18) / 2), (int)(r.y + r.height / 2 - 9), 18, COL_TEXT);
    return pressed;
}

static void text_box_update(TextBox *box) {
    int key;
    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        box->active = CheckCollisionPointRec(mouse, box->bounds);
    }

    if (!box->active) return;

    key = GetCharPressed();
    while (key > 0) {
        int len = (int)strlen(box->text);
        if (key >= 32 && key <= 126 && len < box->max_len - 1) {
            box->text[len] = (char)key;
            box->text[len + 1] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(box->text);
        if (len > 0) box->text[len - 1] = '\0';
    }
}

static void text_box_draw(TextBox *box, const char *label) {
    Rectangle r = box->bounds;
    DrawText(label, (int)r.x, (int)r.y - 22, 15, COL_MUTED);
    DrawRectangleRounded(r, 0.16f, 8, box->active ? (Color){30, 58, 95, 255} : COL_PANEL_DARK);
    DrawRectangleRoundedLinesEx(r, 0.16f, 8, 1.0f, box->active ? COL_BLUE : COL_BORDER);
    DrawText(box->text, (int)r.x + 10, (int)r.y + 9, 18, COL_TEXT);
}

static int to_int(const char *s, int fallback) {
    int value;
    if (s == NULL || s[0] == '\0') return fallback;
    value = atoi(s);
    return value;
}

static void set_output(AppState *app, const char *message) {
    snprintf(app->output, sizeof(app->output), "%s", message ? message : "");
}

static void append_output(AppState *app, const char *message) {
    size_t used = strlen(app->output);
    if (used + 4 >= sizeof(app->output)) return;
    snprintf(app->output + used, sizeof(app->output) - used, "\n\n%s", message ? message : "");
}

static void init_text_box(TextBox *box, const char *text, int max_len) {
    memset(box, 0, sizeof(*box));
    snprintf(box->text, sizeof(box->text), "%s", text ? text : "");
    box->max_len = max_len;
}

static void init_app(AppState *app) {
    char msg[OUTPUT_SIZE];
    memset(app, 0, sizeof(*app));
    app->screen = SCREEN_DASHBOARD;
    app->previous_screen = SCREEN_DASHBOARD;
    app->memory_strategy = MEM_FIRST_FIT;
    app->selected_file = -1;

    init_text_box(&app->task_name, "Ambulance Dispatch", 64);
    init_text_box(&app->task_burst, "4", 16);
    init_text_box(&app->task_priority, "1", 16);
    init_text_box(&app->task_memory, "120", 16);
    init_text_box(&app->task_c, "2", 16);
    init_text_box(&app->task_v, "1", 16);
    init_text_box(&app->task_s, "2", 16);
    init_text_box(&app->quantum, "2", 16);
    init_text_box(&app->ipc_from_box, "1", 16);
    init_text_box(&app->ipc_to_box, "2", 16);
    init_text_box(&app->ipc_message, "Coordinate traffic diversion.", 120);
    init_text_box(&app->res_pid, "1", 16);
    init_text_box(&app->res_c, "1", 16);
    init_text_box(&app->res_v, "0", 16);
    init_text_box(&app->res_s, "1", 16);

    serc_init();
    serc_load_demo_data(msg, sizeof(msg));
    set_output(app, msg);
}

static void switch_screen(AppState *app, AppScreen screen) {
    if (screen == app->screen) return;
    app->previous_screen = app->screen;
    app->screen = screen;
    app->transition = 0.0f;
}

static void draw_sidebar(AppState *app, Rectangle side) {
    panel(side, "");
    DrawText("SERC", (int)side.x + 22, (int)side.y + 20, 34, COL_TEXT);
    DrawText("Mini-OS", (int)side.x + 22, (int)side.y + 56, 20, COL_BLUE);
    DrawText("Emergency Control", (int)side.x + 22, (int)side.y + 84, 14, COL_MUTED);

    for (int i = 0; i < SCREEN_COUNT; i++) {
        Rectangle r = {side.x + 16, side.y + 135 + i * 54, side.width - 32, 42};
        Color accent = i == SCREEN_DASHBOARD ? COL_BLUE :
                       i == SCREEN_SCHEDULING ? COL_GREEN :
                       i == SCREEN_MEMORY ? COL_YELLOW :
                       i == SCREEN_IPC ? COL_PURPLE :
                       i == SCREEN_RESOURCES ? COL_ORANGE : COL_PINK;
        if (app->screen == (AppScreen)i) {
            DrawRectangleRounded(r, 0.22f, 12, color_lerp(COL_PANEL_DARK, accent, 0.38f));
            DrawRectangleRoundedLinesEx(r, 0.22f, 12, 1.3f, accent);
        }
        if (button(r, screen_names[i], accent)) {
            switch_screen(app, (AppScreen)i);
        }
    }
}

static void count_process_states(int *total, int counts[6]) {
    PCB *table = get_processes();
    int count = get_process_count();
    memset(counts, 0, sizeof(int) * 6);
    *total = count;
    for (int i = 0; i < count; i++) {
        if (table[i].state >= STATE_NEW && table[i].state <= STATE_TERMINATED) {
            counts[table[i].state]++;
        }
    }
}

static void metric_card(Rectangle r, const char *label, const char *value, Color accent) {
    DrawRectangleRounded(r, 0.12f, 12, COL_PANEL);
    DrawRectangleRoundedLinesEx(r, 0.12f, 12, 1.0f, with_alpha(accent, 150));
    DrawCircleGradient((int)(r.x + r.width - 28), (int)(r.y + 28), 32, with_alpha(accent, 90), with_alpha(accent, 0));
    DrawText(label, (int)r.x + 16, (int)r.y + 14, 15, COL_MUTED);
    DrawText(value, (int)r.x + 16, (int)r.y + 42, 28, COL_TEXT);
}

static void progress_bar(Rectangle r, float value, Color color, const char *label) {
    value = clampf(value, 0.0f, 1.0f);
    DrawText(label, (int)r.x, (int)r.y - 22, 16, COL_MUTED);
    DrawRectangleRounded(r, 0.35f, 12, COL_PANEL_DARK);
    DrawRectangleRounded((Rectangle){r.x, r.y, r.width * value, r.height}, 0.35f, 12, color);
    DrawRectangleRoundedLinesEx(r, 0.35f, 12, 1.0f, with_alpha(color, 130));
}

static void draw_dashboard(AppState *app, Rectangle area) {
    int total;
    int counts[6];
    int res_total[RESOURCE_TYPES], res_available[RESOURCE_TYPES], res_allocated[RESOURCE_TYPES];
    char value[64];
    Rectangle grid;
    float card_w = (area.width - 36) / 4.0f;

    count_process_states(&total, counts);
    copy_resource_snapshot(res_total, res_available, res_allocated);

    DrawText("Emergency Response Dashboard", (int)area.x, (int)area.y, 30, COL_TEXT);
    DrawText("Live Mini-OS process, memory, resources, and scheduling state.", (int)area.x, (int)area.y + 38, 17, COL_MUTED);

    snprintf(value, sizeof(value), "%d", total);
    metric_card((Rectangle){area.x, area.y + 80, card_w, 94}, "Total Tasks", value, COL_BLUE);
    snprintf(value, sizeof(value), "%d / %d", get_memory_used(), TOTAL_MEMORY);
    metric_card((Rectangle){area.x + card_w + 12, area.y + 80, card_w, 94}, "Memory Used", value, COL_GREEN);
    snprintf(value, sizeof(value), "%d", counts[STATE_READY]);
    metric_card((Rectangle){area.x + (card_w + 12) * 2, area.y + 80, card_w, 94}, "Ready Queue", value, COL_YELLOW);
    snprintf(value, sizeof(value), "%s", serc_has_last_schedule() ? "Active" : "None");
    metric_card((Rectangle){area.x + (card_w + 12) * 3, area.y + 80, card_w, 94}, "Scheduler", value, COL_PURPLE);

    grid = (Rectangle){area.x, area.y + 200, area.width * 0.55f, 260};
    panel(grid, "Process State Mix");
    {
        const ProcessState states[] = {STATE_NEW, STATE_READY, STATE_RUNNING, STATE_WAITING, STATE_SUSPENDED, STATE_TERMINATED};
        const char *labels[] = {"New", "Ready", "Running", "Waiting", "Suspended", "Terminated"};
        for (int i = 0; i < 6; i++) {
            Rectangle row = {grid.x + 24, grid.y + 54 + i * 31, grid.width - 48, 20};
            float pct = total > 0 ? (float)counts[states[i]] / (float)total : 0.0f;
            char label[96];
            snprintf(label, sizeof(label), "%s  %d", labels[i], counts[states[i]]);
            progress_bar(row, pct, state_color(states[i]), label);
        }
    }

    {
        Rectangle res = {area.x + area.width * 0.58f, area.y + 200, area.width * 0.42f, 260};
        panel(res, "Resources");
        const char *names[] = {"Comm Channels", "Vehicles", "Staff Units"};
        Color colors[] = {COL_BLUE, COL_ORANGE, COL_GREEN};
        for (int i = 0; i < RESOURCE_TYPES; i++) {
            char label[96];
            float pct = res_total[i] > 0 ? (float)res_allocated[i] / (float)res_total[i] : 0.0f;
            snprintf(label, sizeof(label), "%s  allocated %d / %d", names[i], res_allocated[i], res_total[i]);
            progress_bar((Rectangle){res.x + 26, res.y + 72 + i * 58, res.width - 52, 22}, pct, colors[i], label);
        }
    }

    {
        Rectangle form = {area.x, area.y + 485, area.width, 178};
        const char *services[] = {"AMBULANCE", "FIRE", "POLICE"};
        int max_need[RESOURCE_TYPES];
        char msg[OUTPUT_SIZE];

        panel(form, "Create Emergency Task");
        app->task_name.bounds = (Rectangle){form.x + 24, form.y + 56, 210, 38};
        app->task_burst.bounds = (Rectangle){form.x + 250, form.y + 56, 70, 38};
        app->task_priority.bounds = (Rectangle){form.x + 338, form.y + 56, 70, 38};
        app->task_memory.bounds = (Rectangle){form.x + 426, form.y + 56, 86, 38};
        app->task_c.bounds = (Rectangle){form.x + 530, form.y + 56, 58, 38};
        app->task_v.bounds = (Rectangle){form.x + 606, form.y + 56, 58, 38};
        app->task_s.bounds = (Rectangle){form.x + 682, form.y + 56, 58, 38};

        text_box_update(&app->task_name); text_box_update(&app->task_burst);
        text_box_update(&app->task_priority); text_box_update(&app->task_memory);
        text_box_update(&app->task_c); text_box_update(&app->task_v); text_box_update(&app->task_s);

        text_box_draw(&app->task_name, "Task Name");
        text_box_draw(&app->task_burst, "Burst");
        text_box_draw(&app->task_priority, "Priority");
        text_box_draw(&app->task_memory, "Memory");
        text_box_draw(&app->task_c, "Max C");
        text_box_draw(&app->task_v, "Max V");
        text_box_draw(&app->task_s, "Max S");

        DrawText(TextFormat("Service: %s", services[app->service_index]), (int)form.x + 760, (int)form.y + 46, 18, COL_TEXT);
        if (button((Rectangle){form.x + 760, form.y + 76, 96, 34}, "Change", COL_PURPLE)) {
            app->service_index = (app->service_index + 1) % 3;
        }
        if (button((Rectangle){form.x + form.width - 150, form.y + 60, 120, 42}, "Create", COL_GREEN)) {
            max_need[0] = to_int(app->task_c.text, 0);
            max_need[1] = to_int(app->task_v.text, 0);
            max_need[2] = to_int(app->task_s.text, 0);
            memset(msg, 0, sizeof(msg));
            serc_add_task(app->task_name.text, services[app->service_index],
                          to_int(app->task_burst.text, 1),
                          to_int(app->task_priority.text, 1),
                          to_int(app->task_memory.text, 1),
                          max_need, app->memory_strategy, msg, sizeof(msg));
            append_output(app, msg);
            app->memory_flash = 1.0f;
        }
    }
}

static void draw_gantt(Rectangle r, AppState *app, const ScheduleResult *result) {
    int total_time;
    float max_time;

    panel(r, "Persistent Animated Gantt Chart");
    if (result == NULL || result->segment_count <= 0) {
        DrawText("Run or compare a scheduling algorithm to draw the Gantt chart.", (int)r.x + 28, (int)r.y + 78, 18, COL_MUTED);
        return;
    }

    total_time = result->total_time > 0 ? result->total_time : result->segments[result->segment_count - 1].end_time;
    max_time = app->gantt_playing ? app->gantt_time : (float)total_time;
    if (max_time >= (float)total_time) {
        app->gantt_playing = 0;
        max_time = (float)total_time;
    }

    DrawText(TextFormat("%s | total time %d | %d segment(s)",
                        scheduler_to_string(result->type), total_time, result->segment_count),
             (int)r.x + 28, (int)r.y + 48, 17, COL_MUTED);

    {
        Rectangle chart = {r.x + 28, r.y + 86, r.width - 56, r.height - 126};
        DrawRectangleRounded(chart, 0.06f, 8, COL_PANEL_DARK);
        for (int i = 0; i <= 4; i++) {
            float x = chart.x + chart.width * (float)i / 4.0f;
            DrawLineEx((Vector2){x, chart.y}, (Vector2){x, chart.y + chart.height}, 1.0f, with_alpha(COL_MUTED, 45));
        }

        for (int i = 0; i < result->segment_count; i++) {
            ScheduleSegment seg = result->segments[i];
            float visible_end = fminf((float)seg.end_time, max_time);
            float x1, x2;
            Rectangle block;
            if ((float)seg.start_time >= max_time) continue;

            x1 = chart.x + ((float)seg.start_time / (float)total_time) * chart.width;
            x2 = chart.x + (visible_end / (float)total_time) * chart.width;
            if (x2 < x1 + 4) x2 = x1 + 4;
            block = (Rectangle){x1, chart.y + 20, x2 - x1, chart.height - 40};

            DrawRectangleRounded((Rectangle){block.x + 2, block.y + 3, block.width, block.height}, 0.18f, 8, with_alpha(BLACK, 70));
            DrawRectangleRounded(block, 0.18f, 8, pid_color(seg.pid));
            if (block.width > 28) {
                DrawText(TextFormat("P%d", seg.pid), (int)(block.x + block.width / 2 - MeasureText(TextFormat("P%d", seg.pid), 18) / 2), (int)(block.y + block.height / 2 - 9), 18, WHITE);
            }
            if (i == 0 || block.width > 32) {
                DrawText(TextFormat("%d", seg.start_time), (int)x1 - 2, (int)(chart.y + chart.height + 8), 14, COL_MUTED);
            }
        }
        DrawText(TextFormat("%d", total_time), (int)(chart.x + chart.width - 10), (int)(chart.y + chart.height + 8), 14, COL_MUTED);
    }
}

static void draw_schedule_tables(Rectangle r, AppState *app, const ScheduleResult *last) {
    (void)app;
    panel(r, "Scheduling Metrics and Computation");

    if (last == NULL || last->metric_count == 0) {
        DrawText("Metrics appear here after scheduling.", (int)r.x + 22, (int)r.y + 56, 18, COL_MUTED);
        return;
    }

    metric_card((Rectangle){r.x + 22, r.y + 50, 150, 78}, "Avg Wait", TextFormat("%.2f", last->average_waiting_time), COL_BLUE);
    metric_card((Rectangle){r.x + 184, r.y + 50, 170, 78}, "Avg Turnaround", TextFormat("%.2f", last->average_turnaround_time), COL_GREEN);
    metric_card((Rectangle){r.x + 366, r.y + 50, 150, 78}, "CPU Used", TextFormat("%.2f%%", last->cpu_utilization), COL_YELLOW);

    {
        float y = r.y + 150;
        const char *headers[] = {"PID", "Burst", "Start", "Completion", "Turnaround", "Waiting"};
        int widths[] = {55, 70, 70, 112, 112, 90};
        float x = r.x + 22;

        DrawRectangleRounded((Rectangle){x - 6, y - 8, r.width - 44, 34}, 0.08f, 8, with_alpha(COL_BLUE, 55));
        for (int h = 0; h < 6; h++) {
            DrawText(headers[h], (int)x, (int)y, 15, COL_TEXT);
            x += widths[h];
        }

        for (int i = 0; i < last->metric_count && i < 10; i++) {
            const ProcessScheduleMetric *m = &last->metrics[i];
            y += 30;
            x = r.x + 22;
            DrawText(TextFormat("%d", m->pid), (int)x, (int)y, 15, COL_MUTED); x += widths[0];
            DrawText(TextFormat("%d", m->burst_time), (int)x, (int)y, 15, COL_MUTED); x += widths[1];
            DrawText(TextFormat("%d", m->start_time), (int)x, (int)y, 15, COL_MUTED); x += widths[2];
            DrawText(TextFormat("%d", m->completion_time), (int)x, (int)y, 15, COL_MUTED); x += widths[3];
            DrawText(TextFormat("%d", m->turnaround_time), (int)x, (int)y, 15, COL_MUTED); x += widths[4];
            DrawText(TextFormat("%d", m->waiting_time), (int)x, (int)y, 15, COL_MUTED);
        }
    }
}

static void draw_comparison_table(Rectangle r, int quantum) {
    SchedulerType types[] = {SCHED_FCFS, SCHED_SJF, SCHED_PRIORITY, SCHED_RR};
    ScheduleResult rows[4];
    int best = -1;
    const char *headers[] = {"Algorithm", "Tasks", "Avg Waiting", "Avg Turnaround", "CPU %", "Time", "Segments", "Best"};
    int widths[] = {150, 56, 105, 126, 70, 60, 82, 60};
    float y = r.y + 52;
    float x;

    panel(r, "Algorithm Comparison");
    for (int i = 0; i < 4; i++) {
        memset(&rows[i], 0, sizeof(rows[i]));
        run_scheduler_preview(types[i], quantum, &rows[i]);
        if (rows[i].process_count > 0 &&
            (best < 0 || rows[i].average_waiting_time < rows[best].average_waiting_time)) {
            best = i;
        }
    }

    DrawRectangleRounded((Rectangle){r.x + 16, y - 8, r.width - 32, 34}, 0.08f, 8, with_alpha(COL_PURPLE, 55));
    x = r.x + 24;
    for (int h = 0; h < 8; h++) {
        DrawText(headers[h], (int)x, (int)y, 14, COL_TEXT);
        x += widths[h];
    }

    for (int i = 0; i < 4; i++) {
        y += 32;
        x = r.x + 24;
        DrawText(scheduler_to_string(rows[i].type), (int)x, (int)y, 14, i == best ? COL_GREEN : COL_MUTED); x += widths[0];
        DrawText(TextFormat("%d", rows[i].process_count), (int)x, (int)y, 14, COL_MUTED); x += widths[1];
        DrawText(TextFormat("%.2f", rows[i].average_waiting_time), (int)x, (int)y, 14, COL_MUTED); x += widths[2];
        DrawText(TextFormat("%.2f", rows[i].average_turnaround_time), (int)x, (int)y, 14, COL_MUTED); x += widths[3];
        DrawText(TextFormat("%.1f", rows[i].cpu_utilization), (int)x, (int)y, 14, COL_MUTED); x += widths[4];
        DrawText(TextFormat("%d", rows[i].total_time), (int)x, (int)y, 14, COL_MUTED); x += widths[5];
        DrawText(TextFormat("%d", rows[i].segment_count), (int)x, (int)y, 14, COL_MUTED); x += widths[6];
        DrawText(i == best ? "YES" : "", (int)x, (int)y, 14, COL_GREEN);
    }
}

static void draw_scheduling(AppState *app, Rectangle area) {
    ScheduleResult last;
    int has_last = serc_copy_last_schedule(&last);
    char msg[OUTPUT_SIZE];
    int quantum = maxi(1, to_int(app->quantum.text, RR_QUANTUM_DEFAULT));

    DrawText("Scheduling Control", (int)area.x, (int)area.y, 30, COL_TEXT);
    DrawText("Run algorithms, compare metrics, and replay the persistent Gantt chart.", (int)area.x, (int)area.y + 38, 17, COL_MUTED);

    app->quantum.bounds = (Rectangle){area.x, area.y + 96, 80, 38};
    text_box_update(&app->quantum);
    text_box_draw(&app->quantum, "RR Quantum");

    if (button((Rectangle){area.x + 100, area.y + 96, 88, 38}, "FCFS", COL_BLUE)) {
        serc_run_scheduler(SCHED_FCFS, quantum, msg, sizeof(msg));
        set_output(app, msg);
        app->gantt_time = 0; app->gantt_playing = 1;
    }
    if (button((Rectangle){area.x + 198, area.y + 96, 88, 38}, "SJF", COL_GREEN)) {
        serc_run_scheduler(SCHED_SJF, quantum, msg, sizeof(msg));
        set_output(app, msg);
        app->gantt_time = 0; app->gantt_playing = 1;
    }
    if (button((Rectangle){area.x + 296, area.y + 96, 104, 38}, "Priority", COL_YELLOW)) {
        serc_run_scheduler(SCHED_PRIORITY, quantum, msg, sizeof(msg));
        set_output(app, msg);
        app->gantt_time = 0; app->gantt_playing = 1;
    }
    if (button((Rectangle){area.x + 410, area.y + 96, 128, 38}, "Round Robin", COL_PURPLE)) {
        serc_run_scheduler(SCHED_RR, quantum, msg, sizeof(msg));
        set_output(app, msg);
        app->gantt_time = 0; app->gantt_playing = 1;
    }
    if (button((Rectangle){area.x + 548, area.y + 96, 120, 38}, "Compare", COL_PINK)) {
        serc_compare_schedulers(quantum, msg, sizeof(msg));
        set_output(app, msg);
        app->gantt_time = 0; app->gantt_playing = 1;
    }
    if (button((Rectangle){area.x + 678, area.y + 96, 92, 38}, "Replay", COL_GREEN)) {
        app->gantt_time = 0; app->gantt_playing = 1;
    }
    if (button((Rectangle){area.x + 780, area.y + 96, 104, 38}, "Summary", COL_BLUE)) {
        serc_get_last_schedule_summary(msg, sizeof(msg));
        set_output(app, msg);
    }
    if (button((Rectangle){area.x + 894, area.y + 96, 120, 38}, "Compute", COL_ORANGE)) {
        serc_get_last_schedule_computation(msg, sizeof(msg));
        set_output(app, msg);
    }

    draw_gantt((Rectangle){area.x, area.y + 160, area.width, 255}, app, has_last ? &last : NULL);
    draw_schedule_tables((Rectangle){area.x, area.y + 435, area.width * 0.52f, area.height - 445}, app, has_last ? &last : NULL);
    draw_comparison_table((Rectangle){area.x + area.width * 0.54f, area.y + 435, area.width * 0.46f, area.height - 445}, quantum);
}

static void draw_memory(AppState *app, Rectangle area) {
    MemorySegment segments[MAX_SEGMENTS];
    MemoryFrame frames[TOTAL_FRAMES];
    int segment_count = copy_memory_segments(segments, MAX_SEGMENTS);
    int frame_count = copy_memory_frames(frames, TOTAL_FRAMES);
    const char *strategies[] = {"First Fit", "Best Fit", "Worst Fit", "Paging"};
    Color strategy_colors[] = {COL_BLUE, COL_GREEN, COL_YELLOW, COL_PURPLE};

    DrawText("Memory and Paging", (int)area.x, (int)area.y, 30, COL_TEXT);
    DrawText("Graphical contiguous map plus paging frame grid.", (int)area.x, (int)area.y + 38, 17, COL_MUTED);

    for (int i = 0; i < 4; i++) {
        Rectangle b = {area.x + i * 128, area.y + 84, 116, 38};
        if (button(b, strategies[i], strategy_colors[i])) {
            app->memory_strategy = (MemoryStrategy)i;
        }
        if (app->memory_strategy == (MemoryStrategy)i) {
            DrawRectangleRoundedLinesEx(b, 0.22f, 12, 2.0f, WHITE);
        }
    }

    metric_card((Rectangle){area.x + area.width - 360, area.y + 76, 160, 76}, "External Frags", TextFormat("%d", get_fragment_count()), COL_YELLOW);
    metric_card((Rectangle){area.x + area.width - 180, area.y + 76, 160, 76}, "Paging Waste", TextFormat("%d", get_paging_internal_fragmentation()), COL_PURPLE);

    {
        Rectangle map = {area.x, area.y + 170, area.width, 180};
        panel(map, "Contiguous Allocation Map");
        for (int i = 0; i < segment_count; i++) {
            float x = map.x + 24 + ((float)segments[i].start / TOTAL_MEMORY) * (map.width - 48);
            float w = ((float)segments[i].size / TOTAL_MEMORY) * (map.width - 48);
            Rectangle block = {x, map.y + 70, w < 3 ? 3 : w, 58};
            Color c = segments[i].is_free ? with_alpha(COL_MUTED, 120) : pid_color(segments[i].pid);
            if (app->memory_flash > 0.0f && !segments[i].is_free) c = color_lerp(c, WHITE, app->memory_flash * 0.45f);
            DrawRectangleRounded(block, 0.12f, 8, c);
            if (block.width > 42) {
                DrawText(segments[i].is_free ? "FREE" : TextFormat("P%d", segments[i].pid), (int)block.x + 6, (int)block.y + 19, 15, WHITE);
            }
        }
        progress_bar((Rectangle){map.x + 24, map.y + 142, map.width - 48, 18}, (float)get_memory_used() / TOTAL_MEMORY, COL_GREEN, TextFormat("Total Memory Used %d / %d", get_memory_used(), TOTAL_MEMORY));
    }

    {
        Rectangle grid = {area.x, area.y + 375, area.width, area.height - 385};
        int cols = 8;
        float cell_w = (grid.width - 64) / cols;
        float cell_h = 46;
        panel(grid, "Paging Frame Grid");
        for (int i = 0; i < frame_count; i++) {
            int row = i / cols;
            int col = i % cols;
            Rectangle cell = {grid.x + 24 + col * cell_w, grid.y + 62 + row * (cell_h + 12), cell_w - 10, cell_h};
            Color c = frames[i].is_free ? with_alpha(COL_MUTED, 80) : (frames[i].page_number >= 0 ? pid_color(frames[i].pid) : with_alpha(COL_ORANGE, 180));
            DrawRectangleRounded(cell, 0.14f, 8, c);
            DrawRectangleRoundedLinesEx(cell, 0.14f, 8, 1.0f, with_alpha(WHITE, 70));
            DrawText(TextFormat("F%d", frames[i].frame_number), (int)cell.x + 8, (int)cell.y + 7, 14, WHITE);
            DrawText(frames[i].is_free ? "free" : TextFormat("P%d pg%d", frames[i].pid, frames[i].page_number), (int)cell.x + 8, (int)cell.y + 25, 12, WHITE);
        }
    }
}

static Vector2 node_position_for_index(Rectangle area, int index, int count) {
    float cx = area.x + area.width / 2.0f;
    float cy = area.y + area.height / 2.0f;
    float radius = fminf(area.width, area.height) * 0.36f;
    float angle = ((float)index / (float)maxi(count, 1)) * 6.2831853f - 1.5708f;
    return (Vector2){cx + cosf(angle) * radius, cy + sinf(angle) * radius};
}

static int process_index_by_pid(int pid) {
    PCB *table = get_processes();
    int count = get_process_count();
    for (int i = 0; i < count; i++) {
        if (table[i].pid == pid) return i;
    }
    return -1;
}

static void draw_ipc(AppState *app, Rectangle area) {
    PCB *table = get_processes();
    int process_count = get_process_count();
    IPCMessage messages[IPC_QUEUE_SIZE];
    int msg_count = ipc_copy_messages(messages, IPC_QUEUE_SIZE);
    char msg[OUTPUT_SIZE];

    DrawText("Inter-Process Communication", (int)area.x, (int)area.y, 30, COL_TEXT);
    DrawText("Message queue and animated process-to-process coordination.", (int)area.x, (int)area.y + 38, 17, COL_MUTED);

    app->ipc_from_box.bounds = (Rectangle){area.x, area.y + 88, 70, 38};
    app->ipc_to_box.bounds = (Rectangle){area.x + 86, area.y + 88, 70, 38};
    app->ipc_message.bounds = (Rectangle){area.x + 172, area.y + 88, 340, 38};
    text_box_update(&app->ipc_from_box); text_box_update(&app->ipc_to_box); text_box_update(&app->ipc_message);
    text_box_draw(&app->ipc_from_box, "From");
    text_box_draw(&app->ipc_to_box, "To");
    text_box_draw(&app->ipc_message, "Message");
    if (button((Rectangle){area.x + 532, area.y + 88, 118, 38}, "Send IPC", COL_PURPLE)) {
        app->ipc_from = to_int(app->ipc_from_box.text, 1);
        app->ipc_to = to_int(app->ipc_to_box.text, 2);
        serc_send_message(app->ipc_from, app->ipc_to, app->ipc_message.text, msg, sizeof(msg));
        append_output(app, msg);
        app->ipc_anim = 1.0f;
    }

    {
        Rectangle graph = {area.x, area.y + 155, area.width * 0.58f, area.height - 165};
        panel(graph, "Process Nodes");
        for (int i = 0; i < process_count; i++) {
            Vector2 p = node_position_for_index(graph, i, process_count);
            Color c = state_color(table[i].state);
            DrawCircleGradient((int)p.x, (int)p.y, 30, with_alpha(c, 110), with_alpha(c, 10));
            DrawCircleV(p, 22, c);
            DrawText(TextFormat("P%d", table[i].pid), (int)p.x - 12, (int)p.y - 9, 18, WHITE);
            DrawText(table[i].service_type, (int)p.x - 34, (int)p.y + 28, 12, COL_MUTED);
        }
        if (app->ipc_anim > 0.0f) {
            int from_idx = process_index_by_pid(app->ipc_from);
            int to_idx = process_index_by_pid(app->ipc_to);
            if (from_idx >= 0 && to_idx >= 0) {
                Vector2 a = node_position_for_index(graph, from_idx, process_count);
                Vector2 b = node_position_for_index(graph, to_idx, process_count);
                float t = 1.0f - app->ipc_anim;
                Vector2 p = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
                DrawLineEx(a, b, 2.0f, with_alpha(COL_BLUE, 80));
                DrawCircleV(p, 9.0f, COL_PINK);
            }
        }
    }

    {
        Rectangle q = {area.x + area.width * 0.60f, area.y + 155, area.width * 0.40f, area.height - 165};
        panel(q, "IPC Queue");
        DrawText("FROM   TO      MESSAGE", (int)q.x + 20, (int)q.y + 54, 15, COL_TEXT);
        for (int i = 0; i < msg_count && i < 12; i++) {
            float y = q.y + 86 + i * 32;
            DrawText(TextFormat("%-6d %-7d", messages[i].from_pid, messages[i].to_pid), (int)q.x + 20, (int)y, 14, COL_MUTED);
            DrawText(messages[i].message, (int)q.x + 115, (int)y, 14, COL_MUTED);
        }
        if (msg_count == 0) DrawText("No messages yet.", (int)q.x + 20, (int)q.y + 86, 16, COL_MUTED);
    }
}

static void draw_resources(AppState *app, Rectangle area) {
    int total[RESOURCE_TYPES], available[RESOURCE_TYPES], allocated[RESOURCE_TYPES];
    char msg[OUTPUT_SIZE];
    const char *names[] = {"Communication Channels", "Vehicles", "Staff Units"};
    Color colors[] = {COL_BLUE, COL_ORANGE, COL_GREEN};

    copy_resource_snapshot(total, available, allocated);
    DrawText("Resources and Deadlock Control", (int)area.x, (int)area.y, 30, COL_TEXT);
    DrawText("Visual resource allocation plus Banker's Algorithm safety status.", (int)area.x, (int)area.y + 38, 17, COL_MUTED);

    app->res_pid.bounds = (Rectangle){area.x, area.y + 90, 70, 38};
    app->res_c.bounds = (Rectangle){area.x + 88, area.y + 90, 70, 38};
    app->res_v.bounds = (Rectangle){area.x + 176, area.y + 90, 70, 38};
    app->res_s.bounds = (Rectangle){area.x + 264, area.y + 90, 70, 38};
    text_box_update(&app->res_pid); text_box_update(&app->res_c); text_box_update(&app->res_v); text_box_update(&app->res_s);
    text_box_draw(&app->res_pid, "PID");
    text_box_draw(&app->res_c, "Comm");
    text_box_draw(&app->res_v, "Vehicles");
    text_box_draw(&app->res_s, "Staff");

    if (button((Rectangle){area.x + 360, area.y + 90, 110, 38}, "Request", COL_GREEN)) {
        int req[RESOURCE_TYPES] = {to_int(app->res_c.text, 0), to_int(app->res_v.text, 0), to_int(app->res_s.text, 0)};
        serc_request_resources(to_int(app->res_pid.text, 1), req, msg, sizeof(msg));
        append_output(app, msg);
    }
    if (button((Rectangle){area.x + 482, area.y + 90, 110, 38}, "Release", COL_ORANGE)) {
        int rel[RESOURCE_TYPES] = {to_int(app->res_c.text, 0), to_int(app->res_v.text, 0), to_int(app->res_s.text, 0)};
        serc_release_resources(to_int(app->res_pid.text, 1), rel, msg, sizeof(msg));
        append_output(app, msg);
    }
    if (button((Rectangle){area.x + 604, area.y + 90, 130, 38}, "Check Safety", COL_BLUE)) {
        serc_check_deadlock_safety(msg, sizeof(msg));
        append_output(app, msg);
    }

    {
        Rectangle bars = {area.x, area.y + 165, area.width * 0.55f, 305};
        panel(bars, "Resource Usage");
        for (int i = 0; i < RESOURCE_TYPES; i++) {
            char label[128];
            float pct = total[i] > 0 ? (float)allocated[i] / (float)total[i] : 0;
            snprintf(label, sizeof(label), "%s  allocated %d / %d  available %d", names[i], allocated[i], total[i], available[i]);
            progress_bar((Rectangle){bars.x + 28, bars.y + 82 + i * 70, bars.width - 56, 26}, pct, colors[i], label);
        }
    }

    {
        Rectangle safe = {area.x + area.width * 0.58f, area.y + 165, area.width * 0.42f, 305};
        int is_safe = serc_check_deadlock_safety(msg, sizeof(msg));
        panel(safe, "Deadlock Status");
        DrawCircleGradient((int)safe.x + 75, (int)safe.y + 92, 44, with_alpha(is_safe ? COL_GREEN : COL_RED, 150), with_alpha(is_safe ? COL_GREEN : COL_RED, 0));
        DrawText(is_safe ? "SAFE" : "UNSAFE", (int)safe.x + 132, (int)safe.y + 73, 34, is_safe ? COL_GREEN : COL_RED);
        draw_wrapped_text(msg, (Rectangle){safe.x + 28, safe.y + 138, safe.width - 56, safe.height - 160}, 16, COL_MUTED);
    }
}

static void parse_file_listing(AppState *app, const char *listing) {
    int count = 0;
    const char *start = listing;
    const char *p = listing;
    memset(app->files, 0, sizeof(app->files));

    while (p != NULL && *p != '\0' && count < FILE_LIST_MAX) {
        if (*p == '\n') {
            int len = (int)(p - start);
            if (len > 0 && len < 128) {
                memcpy(app->files[count], start, (size_t)len);
                app->files[count][len] = '\0';
                count++;
            }
            start = p + 1;
        }
        p++;
    }
    if (start != NULL && *start != '\0' && count < FILE_LIST_MAX) {
        snprintf(app->files[count++], 128, "%s", start);
    }
    app->file_count = count;
    if (app->selected_file >= count) app->selected_file = count - 1;
}

static void refresh_files(AppState *app) {
    char listing[OUTPUT_SIZE];
    serc_list_data_files(listing, sizeof(listing));
    parse_file_listing(app, listing);
}

static void draw_files(AppState *app, Rectangle area) {
    char msg[OUTPUT_SIZE];
    DrawText("System Files and Logs", (int)area.x, (int)area.y, 30, COL_TEXT);
    DrawText("Read logs, export status/schedule reports, and preview saved files.", (int)area.x, (int)area.y + 38, 17, COL_MUTED);

    if (button((Rectangle){area.x, area.y + 88, 110, 38}, "View Logs", COL_BLUE)) {
        serc_logs_to_string(msg, sizeof(msg));
        set_output(app, msg);
    }
    if (button((Rectangle){area.x + 122, area.y + 88, 126, 38}, "Save Status", COL_GREEN)) {
        char name[260];
        serc_save_status_snapshot(name, sizeof(name), msg, sizeof(msg));
        append_output(app, msg);
        refresh_files(app);
    }
    if (button((Rectangle){area.x + 260, area.y + 88, 145, 38}, "Save Schedule", COL_PURPLE)) {
        char name[260];
        serc_save_schedule_report(name, sizeof(name), msg, sizeof(msg));
        append_output(app, msg);
        refresh_files(app);
    }
    if (button((Rectangle){area.x + 417, area.y + 88, 110, 38}, "List Files", COL_YELLOW)) {
        refresh_files(app);
    }
    if (button((Rectangle){area.x + 539, area.y + 88, 120, 38}, "Load Demo", COL_ORANGE)) {
        serc_load_demo_data(msg, sizeof(msg));
        set_output(app, msg);
    }
    if (button((Rectangle){area.x + 671, area.y + 88, 92, 38}, "Reset", COL_RED)) {
        serc_init();
        set_output(app, "System reset completed.");
        refresh_files(app);
    }

    {
        Rectangle list = {area.x, area.y + 158, area.width * 0.34f, area.height - 168};
        panel(list, "data/ Files");
        if (app->file_count == 0) refresh_files(app);
        for (int i = 0; i < app->file_count && i < 15; i++) {
            Rectangle row = {list.x + 18, list.y + 58 + i * 31, list.width - 36, 26};
            int hover = CheckCollisionPointRec(GetMousePosition(), row);
            if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                app->selected_file = i;
                serc_read_data_file(app->files[i], app->file_preview, sizeof(app->file_preview));
            }
            DrawRectangleRounded(row, 0.12f, 8, i == app->selected_file ? with_alpha(COL_BLUE, 90) : (hover ? with_alpha(COL_BLUE, 45) : with_alpha(COL_PANEL_DARK, 120)));
            DrawText(app->files[i], (int)row.x + 8, (int)row.y + 6, 14, COL_TEXT);
        }
    }

    {
        Rectangle preview = {area.x + area.width * 0.37f, area.y + 158, area.width * 0.63f, area.height - 168};
        panel(preview, "Preview");
        draw_wrapped_text(app->file_preview[0] ? app->file_preview : "Select a file from the list to preview it here.",
                          (Rectangle){preview.x + 22, preview.y + 58, preview.width - 44, preview.height - 76},
                          15, COL_MUTED);
    }
}

static void draw_output_panel(AppState *app, Rectangle r) {
    panel(r, "Output Status");
    draw_wrapped_text(app->output, (Rectangle){r.x + 22, r.y + 54, r.width - 44, r.height - 72}, 15, COL_MUTED);
}

static void update_app(AppState *app) {
    float dt = GetFrameTime();
    app->time += dt;
    app->pulse = (sinf(app->time * 2.0f) + 1.0f) * 0.5f;
    if (app->transition < 1.0f) app->transition = clampf(app->transition + dt * 4.0f, 0.0f, 1.0f);
    if (app->gantt_playing) app->gantt_time += dt * 12.0f;
    if (app->memory_flash > 0.0f) app->memory_flash = clampf(app->memory_flash - dt, 0.0f, 1.0f);
    if (app->ipc_anim > 0.0f) app->ipc_anim = clampf(app->ipc_anim - dt * 0.9f, 0.0f, 1.0f);
}

int main(void) {
    AppState app;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(APP_MIN_W, APP_MIN_H, "SERC Mini-OS - Raylib Emergency Dashboard");
    SetTargetFPS(60);
    init_app(&app);

    while (!WindowShouldClose()) {
        int w = maxi(GetScreenWidth(), APP_MIN_W);
        int h = maxi(GetScreenHeight(), APP_MIN_H);
        Rectangle side = {20, 20, 210, h - 40};
        Rectangle content = {250, 24, w - 280, h - 220};
        Rectangle output = {250, h - 176, w - 280, 152};

        update_app(&app);

        BeginDrawing();
        ClearBackground(COL_BG_TOP);
        draw_background(&app);
        draw_sidebar(&app, side);

        switch (app.screen) {
            case SCREEN_DASHBOARD: draw_dashboard(&app, content); break;
            case SCREEN_SCHEDULING: draw_scheduling(&app, content); break;
            case SCREEN_MEMORY: draw_memory(&app, content); break;
            case SCREEN_IPC: draw_ipc(&app, content); break;
            case SCREEN_RESOURCES: draw_resources(&app, content); break;
            case SCREEN_FILES: draw_files(&app, content); break;
            default: break;
        }

        if (app.transition < 1.0f) {
            unsigned char alpha = (unsigned char)((1.0f - app.transition) * 120.0f);
            DrawRectangleRec(content, with_alpha(COL_BG_TOP, alpha));
            DrawRectangleLinesEx((Rectangle){content.x - 4.0f + (1.0f - app.transition) * 24.0f,
                                             content.y - 4.0f,
                                             content.width + 8.0f,
                                             content.height + 8.0f},
                                 2.0f,
                                 with_alpha(COL_BLUE, alpha));
        }

        draw_output_panel(&app, output);
        DrawText(TextFormat("FPS %d", GetFPS()), w - 82, 8, 14, with_alpha(COL_MUTED, 180));
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
