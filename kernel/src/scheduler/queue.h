#ifndef QUEUE_H
#define QUEUE_H

#include "task.h"

void task_queue_push(task_t **head, task_t *task);
void task_queue_remove(task_t **head, task_t *task);

#endif
