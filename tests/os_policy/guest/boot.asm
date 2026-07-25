; SPDX-License-Identifier: Apache-2.0
; OS-P1 legacy-BIOS bootstrap.
;
; Entry state:
;   CS:IP = 0000:7C00 (normalized below), DL = BIOS boot drive.
; Exit state:
;   64-bit long mode, identity-mapped low 2 MiB, interrupts disabled,
;   RSP = 0x90000, and control transferred to KERNEL_LOAD_ADDRESS.

%ifndef KERNEL_SECTORS
%error "KERNEL_SECTORS must be defined"
%endif

%define KERNEL_LOAD_ADDRESS 0x00010000
%define PAGE_TABLE_ROOT     0x00070000
%define PAGE_TABLE_PDPT     0x00071000
%define PAGE_TABLE_PD       0x00072000
%define LONG_MODE_STACK     0x00090000
%define CODE32_SELECTOR     0x08
%define DATA_SELECTOR       0x10
%define CODE64_SELECTOR     0x18
%define IA32_EFER           0xC0000080
%define EFER_LME            (1 << 8)
%define CR4_PAE             (1 << 5)
%define CR0_PE              (1 << 0)
%define CR0_PG              (1 << 31)

[org 0x7C00]
[bits 16]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    cld
    mov [boot_drive], dl

    mov ah, 0x00
    int 0x13
    jc disk_error

    mov si, disk_address_packet
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error

    cli
    in al, 0x92
    or al, 0x02
    out 0x92, al

    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, CR0_PE
    mov cr0, eax
    jmp CODE32_SELECTOR:protected_entry

disk_error:
    cli
.hang:
    hlt
    jmp .hang

[bits 32]
protected_entry:
    mov ax, DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor ax, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x00007C00

    mov edi, PAGE_TABLE_ROOT
    xor eax, eax
    mov ecx, (3 * 4096) / 4
    rep stosd

    mov dword [PAGE_TABLE_ROOT], PAGE_TABLE_PDPT | 0x03
    mov dword [PAGE_TABLE_PDPT], PAGE_TABLE_PD | 0x03
    mov dword [PAGE_TABLE_PD], 0x00000083

    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax
    mov eax, PAGE_TABLE_ROOT
    mov cr3, eax

    mov ecx, IA32_EFER
    rdmsr
    or eax, EFER_LME
    wrmsr

    mov eax, cr0
    or eax, CR0_PG
    mov cr0, eax
    jmp CODE64_SELECTOR:long_mode_entry

[bits 64]
long_mode_entry:
    mov ax, DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax
    mov rsp, LONG_MODE_STACK
    mov rax, KERNEL_LOAD_ADDRESS
    jmp rax

align 8
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00AF9A000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 4
disk_address_packet:
    db 0x10
    db 0x00
    dw KERNEL_SECTORS
    dw 0x0000
    dw KERNEL_LOAD_ADDRESS >> 4
    dq 0x0000000000000001

boot_drive:
    db 0

times 510 - ($ - $$) db 0
dw 0xAA55
