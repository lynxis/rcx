	.section .text
	.align 1
	.global __start
__start:
	mov.w #0x3020:16,r6
	jsr 0x1b62
	jsr 0x27c8	
	jsr wait
	mov.w #0x3020:16,r6
	jsr 0x27ac
	jsr 0x27c8
	jsr wait
	jmp __start
halt:
	jmp halt

wait:
	mov.w #0xf000:16,r2
	mov.w #0x0001:16,r3
	mov.w #0x0000:16,r4
waitloop:
	sub.w r3, r2
	cmp.w r4, r2
	beq done
	jmp waitloop
done:	
	rts
	
	
	.section .data
	.string "Do you byte, when I knock?"

	.end
