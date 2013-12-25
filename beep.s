	.section .text
	.align 1
	.global __start
__start:
	jsr enablerun
	mov.w #0xf000:16,r2
	mov.w r2, @0x8200

blink:	
	mov.w #0x3020:16,r6
	jsr 0x1b62
	jsr 0x27c8
	jsr wait
	mov.w #0x3020:16,r6
	jsr 0x27ac
	jsr 0x27c8
	jsr wait
	jmp blink
displaynum:
	mov.w #0x3002, r1
	push r1
	push r6
	mov.w #0x301f,r6
	jsr 0x1ff2
	jsr 0x27c8
	pop r6
	pop r1
	rts

enablerun:
	mov.b #0x3,r1l
	mov.b r1l, @0xffc6 	; enable irq sense control register
	mov.b r1l, @0xffc7	; irq enable
	mov.w @0x0, r1    ; store address of reset vector
	mov.w r1, @0xfd96 ; install subroutine for on/off button
	mov.w #runbutton, r1 	; store address of run button handler
	mov.w r1, @0xfd94 	; install handler for run button

	;; mov.w #runbutton, r6
	;; mov.w #0x8000, r1
	;; sub.w r1, r6
	;; jsr displaynum
	;; sleep
	rts
runbutton:
	push r0
	push r1
	push r2
	push r3
	push r4
	push r5
	mov.w #0x1000:16,r2
	mov.w r2, @0x8200
	pop r5
	pop r4
	pop r3
	pop r2
	pop r1
	pop r0
	rts
halt:
	jmp halt

wait:
	mov.w @0x8200,r2
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
