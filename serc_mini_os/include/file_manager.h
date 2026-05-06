#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "common.h"

#define DATA_DIR "data"

void file_manager_init(void);
int file_manager_save_text(const char *prefix,
                           const char *extension,
                           const char *content,
                           char *saved_name,
                           size_t saved_name_size);
int file_manager_list_files(char *buffer, size_t size);
int file_manager_read_file(const char *name, char *buffer, size_t size);

#endif
