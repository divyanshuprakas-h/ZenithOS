#ifndef SCHEDULER_ERROR_H
#define SCHEDULER_ERROR_H

typedef enum
{
    SCHED_OK = 0,
    SCHED_ERROR_OUT_OF_MEMORY,
    SCHED_ERROR_INVALID_ARGUMENT,
    SCHED_ERROR_QUEUE_FULL
} scheduler_error_t;

#endif

