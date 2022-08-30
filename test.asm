bits 64
global _start

_start:
	mov rax, 0
	mov rdi, hello
	int 0x42
	jmp _loop

_loop:
	jmp _loop

section .data
hello: db "hello", 0
