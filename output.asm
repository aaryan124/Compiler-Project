; ================================================

bits 32

extern _printf
extern _scanf

section .data
    fmt_int db "%d", 10, 0
    fmt_scan db "%d", 0

section .text
    global _main

; ---- main ----
_main:
    push ebp
    mov ebp, esp
    sub esp, 16
    ; Stack layout for 'main':
    ;   [ebp-4]  x

    lea eax, [ebp-4]
    push eax
    push dword fmt_scan
    call _scanf
    add esp, 8
    mov eax, dword [ebp-4]
    push eax
    push dword fmt_int
    call _printf
    add esp, 8
    mov eax, 0
    leave
    ret

