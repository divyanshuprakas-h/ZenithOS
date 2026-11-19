bits 64

global _start

extern main

section .text

_start:
    call main

.hang:
    hlt
    jmp .hang