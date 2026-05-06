#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"

void logger_init(void);
void log_event(const char *fmt, ...);
void read_log_file(char *buffer, size_t size);
const char *get_log_file_path(void);

#endif
