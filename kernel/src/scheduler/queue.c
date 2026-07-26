#include "queue.h"

void task_queue_push(task_t **head, task_t *task)
{
    if (head == NULL || task == NULL)
    {
        return;
    }

    task->next = NULL;

    if (*head == NULL)
    {
        *head = task;
        return;
    }

    task_t *current = *head;

    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = task;

}

void task_queue_remove(task_t **head, task_t *task)
{
    if (head == NULL || *head == NULL || task == NULL)
    {
        return;
    }

    if (*head == task)
    {
        *head = task->next;
        task->next = NULL;
        return;
    }

    task_t *current = *head;

    while (current->next != NULL)
    {
        if (current->next == task)
        {
            current->next = task->next;
            task->next = NULL;
            return;
        }
        current = current->next;
    }
}