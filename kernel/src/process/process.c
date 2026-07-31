#include "process.h"

#include "../scheduler/task.h"
#include "../scheduler/scheduler.h"
#include "../mm/heap.h"
#include "../lib/string.h"
#include "../lib/memory.h"
#include "../stdio/printf.h"
#include "../fs/fd.h"
#include  "../kernel_err/errno.h"

static uint64_t next_pid = 1;

static process_t *process_list = NULL;

static uint64_t process_allocate_pid(void)
{
    return next_pid++;
}

process_t *process_create(
    const char *name,
    void(*entry)(void)
)
{
    if (entry == NULL)
    {
        return NULL;
    }

    process_t *process = kmalloc(sizeof(process_t));

    if (process == NULL)
    {
        return NULL;
    }

    k_memset(process, 0, sizeof(process_t));
    fd_init(&process->fd_table);

    process->pid = process_allocate_pid();

    process->state = PROCESS_READY;

    process->exit_code = 0;

    process->parent = NULL;

    process->page_table = page_table_create(&process->page_table_physical);

    if (process->page_table == NULL)
    {
        return NULL;
    }

    kprintf("[PT] process->page_table_physical = %p\n", (void *)process->page_table_physical);
    kprintf("[PROCESS] PID %u PML4 VA=%p PA=%p\n", process->pid, process->page_table, (void *)process->page_table_physical);

    process->next = process_list;
    process_list = process;

    k_memset(process->name, 0, sizeof(process->name));
    k_strcpy(process->name, name);

    task_t *task = task_create(entry);

    if (task == NULL)
    {
        kfree(process);
        return NULL;
    }

    task->process = process;

    process->main_thread = task;
    process->thread_list = task;

    scheduler_add_task(task);

    return process;

}

process_t *process_lookup(uint64_t pid)
{
    process_t *process = process_list;

    while (process != NULL)
    {
        if (process->pid == pid)
        {
            return process;
        }
        process = process->next;
    }
    return NULL;
}

process_t *process_current(void)
{
    task_t *task = scheduler_current_task();

    if (task == NULL)
    {
        return NULL;
    }

    return task->process;
}

void process_destroy(process_t *process)
{
    if (process == NULL)
    {
        return;
    }

    if (process_list == process)
    {
        process_list = process->next;
    }
    else
    {
        process_t *current = process_list;

        while (current != NULL && current->next != process)
        {
            current = current->next;
        }

        if (current != NULL)
        {
            current->next = process->next;
        }
    }
    kfree(process);
}

void process_exit(int exit_code)
{
    process_t *process = process_current();

    if (process == NULL)
    {
        return;
    }

    process->state = PROCESS_EXITED;
    process->exit_code = exit_code;

    kprintf("[PROCESS] PID %llu exited with status %d\n", (unsigned long long)process->pid, exit_code);

    task_exit();

}


int process_load_image(process_t *process, const elf_image_t *image)
{
    if (process == NULL || image == NULL)
        return KERNEL_EINVAL;

    process->image = *image;

    return KERNEL_SUCCESS;
}

