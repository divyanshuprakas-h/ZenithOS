global context_start_user

section .text

context_start_user:
    cli

.hang:
    hlt
    jmp .hang