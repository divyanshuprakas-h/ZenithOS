#ifndef PROCESS_H
#define PROCESS_H

#include "../fs/fd.h"
#include "../elf/elf_loader.h"
#include "../vmm/page_table.h"

#include <stdint.h>

struct task;

typedef enum
{
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_EXITED
} process_state_t;

typedef struct process
{
    uint64_t pid;

    process_state_t state;

    int exit_code;

    char name[64];

    struct process *parent;

    struct task *main_thread;

    struct task *thread_list;

    struct process *next;

    elf_image_t image;

    fd_table_t fd_table;

    page_table_t *page_table;

    uint64_t page_table_physical;

}process_t;

process_t *process_create(
    const char *name,
    void(*entry)(void)
);

process_t *process_lookup(uint64_t pid);

process_t *process_current(void);

void process_destroy(process_t *process);

void process_test(void);

void process_exit(int exit_code);

int process_load_image(process_t *process, const elf_image_t *image);

#endif