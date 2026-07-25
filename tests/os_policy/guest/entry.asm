; SPDX-License-Identifier: Apache-2.0
; Shared OS-P1 guest runtime. Architecture and device operations remain
; assembly-owned; the linked policy object is hardware-blind C.

[bits 64]

%define COM1_BASE            0x03F8
%define COM1_LINE_STATUS     (COM1_BASE + 5)
%define COM1_TRANSMIT_EMPTY  0x20
%define QEMU_DEBUG_EXIT_PORT 0x00F4
%define GUEST_STACK_TOP      0x00090000
%define GUEST_FAILURE_CODE   0x0000003F

section .text.entry
global _start
global osp_guest_serial_write
global osp_guest_exit
extern osp_guest_main

_start:
    cli
    mov rsp, GUEST_STACK_TOP
    and rsp, -16
%ifdef OSP_GUEST_ENABLE_SSE2
    ; Case-scoped scalar-double support. Architecture enablement remains
    ; assembly-owned; policy C only consumes the declared SysV XMM ABI.
    mov rax, cr0
    and rax, ~(1 << 2)
    or rax, (1 << 1)
    mov cr0, rax
    mov rax, cr4
    or rax, (1 << 9) | (1 << 10)
    mov cr4, rax
%endif
    call serial_init
    call osp_guest_main
    mov edi, GUEST_FAILURE_CODE
    jmp osp_guest_exit

; serial_init()
; Clobbers: RAX, RDX.
serial_init:
    mov dx, COM1_BASE + 1
    xor al, al
    out dx, al
    mov dx, COM1_BASE + 3
    mov al, 0x80
    out dx, al
    mov dx, COM1_BASE
    mov al, 0x03
    out dx, al
    mov dx, COM1_BASE + 1
    xor al, al
    out dx, al
    mov dx, COM1_BASE + 3
    mov al, 0x03
    out dx, al
    mov dx, COM1_BASE + 2
    mov al, 0xC7
    out dx, al
    mov dx, COM1_BASE + 4
    mov al, 0x0B
    out dx, al
    ret

; serial_putc(DIL=byte)
; Clobbers: RAX, RDX, R8.
serial_putc:
    mov r8b, dil
.wait:
    mov dx, COM1_LINE_STATUS
    in al, dx
    test al, COM1_TRANSMIT_EMPTY
    jz .wait
    mov dx, COM1_BASE
    mov al, r8b
    out dx, al
    ret

; osp_guest_serial_write(RDI=nul-terminated bytes)
; Clobbers: RAX, RDX, RSI, RDI, R8.
osp_guest_serial_write:
    mov rsi, rdi
.next:
    lodsb
    test al, al
    jz .done
    mov dil, al
    call serial_putc
    jmp .next
.done:
    ret

; osp_guest_exit(EDI=debug-exit value)
; Does not return. QEMU's process exit status is (value << 1) | 1.
osp_guest_exit:
    mov eax, edi
    mov dx, QEMU_DEBUG_EXIT_PORT
    out dx, eax
    cli
.halt:
    hlt
    jmp .halt
