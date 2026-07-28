global cpu_cli
global cpu_sti
global cpu_interrupts_enabled

section .text

cpu_cli:
    cli
    ret

cpu_sti:
    sti
    ret

cpu_interrupts_enabled:
    pushfq
    pop rax
    shr rax, 9
    and rax, 1
    ret