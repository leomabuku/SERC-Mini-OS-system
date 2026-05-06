#include "logger.h"

#include <stdarg.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_if_needed(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define mkdir_if_needed(path) mkdir(path, 0777)
#endif

static char log_path[260] = "logs/serc_log.txt";

static FILE *open_log_file(const char *mode) {
    FILE *fp = fopen(log_path, mode);
    if (fp != NULL) {
        return fp;
    }

    mkdir_if_needed("logs");
    fp = fopen(log_path, mode);
    if (fp != NULL) {
        return fp;
    }

    snprintf(log_path, sizeof(log_path), "../logs/serc_log.txt");
    mkdir_if_needed("../logs");
    return fopen(log_path, mode);
}

void logger_init(void) {
    FILE *fp = open_log_file("a");
    if (fp == NULL) {
        return;
    }

    time_t now = time(NULL);
    char time_buffer[64] = "Unknown";
    struct tm *tm_info = localtime(&now);
    if (tm_info != NULL) {
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    fprintf(fp, "\n========== SERC Mini-OS Session Start: %s =========="
                "\n", time_buffer);
    fclose(fp);
}

void log_event(const char *fmt, ...) {
    FILE *fp = open_log_file("a");
    if (fp == NULL) {
        return;
    }

    time_t now = time(NULL);
    char time_buffer[64] = "Unknown";
    struct tm *tm_info = localtime(&now);
    if (tm_info != NULL) {
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    fprintf(fp, "[%s] ", time_buffer);

    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);

    fprintf(fp, "\n");
    fclose(fp);
}

void read_log_file(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    FILE *fp = open_log_file("r");
    if (fp == NULL) {
        snprintf(buffer, size, "Log file not found at %s", log_path);
        return;
    }

    size_t total = fread(buffer, 1, size - 1, fp);
    buffer[total] = '\0';
    fclose(fp);
}

const char *get_log_file_path(void) {
    return log_path;
}
