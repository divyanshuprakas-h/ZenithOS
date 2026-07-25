global context_switch
global context_start

; cpu_context_t offsets (bytes)

%define CTX_RSP     0

%define CTX_RBX     8
%define CTX_RBP     16

%define CTX_R12     24
%define CTX_R13     32
%define CTX_R14     40
%define CTX_R15     48

; task_t offsets (bytes)

%define TASK_CONTEXT        8
%define TASK_ENTRY          64
%define TASK_STARTED        72
%define TASK_KERNEL_STACK   80
%define TASK_NEXT           104

section .text

; context_switch
;
; RDI = old cpu_context_t
; RSI = new cpu_context_t

context_switch:

    ; Save current task context

    lea rax, [rsp]
    mov [rdi + CTX_RSP], rax

    mov [rdi + CTX_RBX], rbx
    mov [rdi + CTX_RBP], rbp

    mov [rdi + CTX_R12], r12
    mov [rdi + CTX_R13], r13
    mov [rdi + CTX_R14], r14
    mov [rdi + CTX_R15], r15

    ; Restore next task context

    mov rsp, [rsi + CTX_RSP]

    mov rbx, [rsi + CTX_RBX]
    mov rbp, [rsi + CTX_RBP]

    mov r12, [rsi + CTX_R12]
    mov r13, [rsi + CTX_R13]
    mov r14, [rsi + CTX_R14]
    mov r15, [rsi + CTX_R15]

    ret


; context_start
;
; RDI = task_t *
;
; Starts a task for the first time.

context_start:

    ; Load task stack
    mov rsp, [rdi + TASK_CONTEXT + CTX_RSP]

    ; Jump directly to task entry
    mov rax, [rdi + TASK_ENTRY]

    jmp rax