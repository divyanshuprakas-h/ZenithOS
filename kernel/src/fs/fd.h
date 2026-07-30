#ifndef FD_H
#define FD_H

#include "file.h"

#define FD_MAX 64

typedef struct fd_table
{

    file_t *files[FD_MAX];

} fd_table_t;

void fd_init(fd_table_t *table);

int fd_allocate(fd_table_t *table, file_t *file);

file_t *fd_get(fd_table_t *table, int fd);

int fd_close(fd_table_t *table, int fd);

#endif




