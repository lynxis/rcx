	.section .text
	.align 1
	.global __start
__start:
	mov.w #0x3020:16,r6
	jsr 0x1b62
	jmp halt
halt:
	jsr 0x27c8
	;; 	jmp @(#-2:8, PC)
	;; 	jmp __start
	;; 	bra @(2:8,PC)
	jmp halt
	mov.w @0,r0
	jsr @r0

	
	.section .data
	.string "Do you byte, when I knock?"

	.end
