#include "context.h"

#include "../lib/memory.h"

void context_init(
    cpu_context_t *context,
    void (*entry)(void),
    void *stack_top
)
{
    k_memset(context, 0, sizeof(cpu_context_t));

    uint64_t *stack = (uint64_t *)stack_top;

    *(--stack) = 0;
    *(--stack) = (uint64_t)entry;
    context->rsp = (uint64_t)stack;
}
