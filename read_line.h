#ifndef TASK5_6_READ_LINE_H
#define TASK5_6_READ_LINE_H
#include "sys/types.h"
#include <sys/select.h>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define FAIL     -1
#define MC_EOF      -2
#define TIMEOUT  -3

struct file;

struct file *make_buffered_file(int fd, size_t buf_size);

void free_buffered_file(struct file *f);

size_t read_line(struct file *f, char *res, size_t size, time_t timeout);

#endif
