global context_switch
global context_switch_to_interrupt
global context_resume_from_interrupt
global context_start

extern scheduler_trace_context_switch_entry

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
%define TASK_ENTRY          72
%define TASK_KERNEL_STACK   80
%define TASK_NEXT           104

%macro SAVE_CONTEXT 1
    lea rax, [rsp]
    mov [%1 + CTX_RSP], rax

    mov [%1 + CTX_RBX], rbx
    mov [%1 + CTX_RBP], rbp

    mov [%1 + CTX_R12], r12
    mov [%1 + CTX_R13], r13
    mov [%1 + CTX_R14], r14
    mov [%1 + CTX_R15], r15
%endmacro

%macro RESTORE_CONTEXT 1
    mov rsp, [%1 + CTX_RSP]

    mov rbx, [%1 + CTX_RBX]
    mov rbp, [%1 + CTX_RBP]

    mov r12, [%1 + CTX_R12]
    mov r13, [%1 + CTX_R13]
    mov r14, [%1 + CTX_R14]
    mov r15, [%1 + CTX_R15]
%endmacro

%macro POP_INTERRUPT_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

section .text

; context_switch
;
; RDI = old cpu_context_t
; RSI = new cpu_context_t

context_switch:
    push rdi
    push rsi
    call scheduler_trace_context_switch_entry
    pop rsi
    pop rdi

    SAVE_CONTEXT rdi
    RESTORE_CONTEXT rsi

    sti
    ret


; context_switch_to_interrupt
;
; RDI = old cpu_context_t
; RSI = interrupt stack pointer saved for the next task

context_switch_to_interrupt:
    SAVE_CONTEXT rdi

    mov rsp, rsi
    POP_INTERRUPT_REGS

    add rsp, 16
    iretq


; context_resume_from_interrupt
;
; RDI = cpu_context_t *
;
; Restores a cooperative task state after an interrupt-driven handoff.
; The final STI relies on x86's delayed-interrupt semantics so the RET
; completes before a new IRQ can arrive.

context_resume_from_interrupt:
    RESTORE_CONTEXT rdi
    sti
    ret


; context_start
;
; RDI = task_t *
;
; Starts a task for the first time.

context_start:

    ; Load task stack
    mov rsp, [rdi + TASK_CONTEXT + CTX_RSP]
    add rsp, 8

    ; Jump directly to task entry
    mov rax, [rdi + TASK_ENTRY]

    jmp rax
