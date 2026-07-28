#include "context.h"

#include "../lib/memory.h"
#include "../stdio/printf.h"

void context_init(
    cpu_context_t *context,
    void (*entry)(void),
    void *stack_top
)
{
    kprintf("[TRACE] context_init context=%p entry=%p stack_top=%p\n",
            context,
            entry,
            stack_top);

    k_memset(context, 0, sizeof(cpu_context_t));

    uint64_t *stack = (uint64_t *)stack_top;

    *(--stack) = 0;
    *(--stack) = (uint64_t)entry;
    context->rsp = (uint64_t)stack;

    kprintf("[TRACE] context_init stored rsp=%p top_qword=%p next_qword=%p\n",
            (void *)(uintptr_t)context->rsp,
            (void *)(uintptr_t)stack[0],
            (void *)(uintptr_t)stack[1]);
}
