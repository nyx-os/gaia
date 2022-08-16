;
; Copyright (c) 2022, lg
; 
; SPDX-License-Identifier: BSD-2-Clause
;
bits 64

global gdt_load

gdt_load:
  lgdt [rdi]

  ; Set ss,ds and es to data segment
  mov ax, 0x30
  mov ss, ax
  mov ds, ax
  mov es, ax
  pop rdi

  ; Push the code segment so that the cpu will pop it off through retfq
  mov rax, 0x28
  push rax
  push rdi
  retfq