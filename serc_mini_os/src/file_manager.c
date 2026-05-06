#include "file_manager.h"
#include "logger.h"

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define mkdir_if_needed(path) _mkdir(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#define mkdir_if_needed(path) mkdir(path, 0777)
#endif

static void make_data_dir(void) {
    mkdir_if_needed(DATA_DIR);
}

static void timestamp_slug(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    if (buffer == NULL || size == 0) {
        return;
    }

    if (tm_info == NULL) {
        snprintf(buffer, size, "unknown_time");
        return;
    }

    strftime(buffer, size, "%Y%m%d_%H%M%S", tm_info);
}

static int is_safe_file_name(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    for (int i = 0; name[i] != '\0'; i++) {
        char c = name[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            return 0;
        }
    }

    return 1;
}

void file_manager_init(void) {
    make_data_dir();
}

int file_manager_save_text(const char *prefix,
                           const char *extension,
                           const char *content,
                           char *saved_name,
                           size_t saved_name_size) {
    char stamp[32];
    char name[128];
    char path[320];
    FILE *fp;

    if (prefix == NULL || prefix[0] == '\0' ||
        extension == NULL || extension[0] == '\0' ||
        content == NULL || saved_name == NULL || saved_name_size == 0) {
        return 0;
    }

    make_data_dir();
    timestamp_slug(stamp, sizeof(stamp));
    snprintf(name, sizeof(name), "%s_%s.%s", prefix, stamp, extension);
    snprintf(path, sizeof(path), "%s/%s", DATA_DIR, name);

    fp = fopen(path, "w");
    if (fp == NULL) {
        snprintf(saved_name, saved_name_size, "Could not open %s for writing.", path);
        log_event("File save failed: %s", path);
        return 0;
    }

    fputs(content, fp);
    fclose(fp);

    snprintf(saved_name, saved_name_size, "%s", name);
    log_event("File saved: %s", path);
    return 1;
}

int file_manager_list_files(char *buffer, size_t size) {
    size_t used = 0;
    int count = 0;

    if (buffer == NULL || size == 0) {
        return 0;
    }

    make_data_dir();
    buffer[0] = '\0';

#ifdef _WIN32
    {
        WIN32_FIND_DATAA data;
        HANDLE find;
        char pattern[320];

        snprintf(pattern, sizeof(pattern), "%s/*", DATA_DIR);
        find = FindFirstFileA(pattern, &data);
        if (find == INVALID_HANDLE_VALUE) {
            snprintf(buffer, size, "No files found in %s.\n", DATA_DIR);
            return 0;
        }

        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                int written = snprintf(buffer + used, size - used, "%s\n", data.cFileName);
                if (written > 0) {
                    used += (size_t)written < size - used ? (size_t)written : size - used - 1;
                }
                count++;
            }
        } while (FindNextFileA(find, &data) && used < size - 1);

        FindClose(find);
    }
#else
    {
        DIR *dir = opendir(DATA_DIR);
        struct dirent *entry;

        if (dir == NULL) {
            snprintf(buffer, size, "No files found in %s.\n", DATA_DIR);
            return 0;
        }

        while ((entry = readdir(dir)) != NULL && used < size - 1) {
            if (entry->d_name[0] == '.') {
                continue;
            }

            int written = snprintf(buffer + used, size - used, "%s\n", entry->d_name);
            if (written > 0) {
                used += (size_t)written < size - used ? (size_t)written : size - used - 1;
            }
            count++;
        }

        closedir(dir);
    }
#endif

    if (count == 0) {
        snprintf(buffer, size, "No files found in %s.\n", DATA_DIR);
    }

    return count;
}

int file_manager_read_file(const char *name, char *buffer, size_t size) {
    char path[320];
    FILE *fp;
    size_t read_count;

    if (!is_safe_file_name(name) || buffer == NULL || size == 0) {
        if (buffer != NULL && size > 0) {
            snprintf(buffer, size, "Invalid file name.");
        }
        return 0;
    }

    snprintf(path, sizeof(path), "%s/%s", DATA_DIR, name);
    fp = fopen(path, "r");
    if (fp == NULL) {
        snprintf(buffer, size, "Could not open %s.", path);
        return 0;
    }

    read_count = fread(buffer, 1, size - 1, fp);
    buffer[read_count] = '\0';
    fclose(fp);
    return 1;
}
