global atomic_xchg_u64
global cpu_pause

section .text

atomic_xchg_u64:
    mov rax, rsi
    xchg rax, [rdi]
    ret

cpu_pause:
    pause
    ret