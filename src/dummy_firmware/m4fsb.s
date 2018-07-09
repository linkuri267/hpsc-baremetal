.cpu cortex-m4
.thumb

.thumb_func
.global _start
_start:
stacktop: .word 0x20001000
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset
.word reset


	.fpu softvfp
	.thumb_func
reset:
        ldr r0, .L2
	bx	r0
.L3:
	.align 2
.L2:
	.word 1073741824 // 0x40000000
#	.word 	-3145728	// ffd00000
#	.word 	-2359296	// fffdc000
#	.word 	2048 // 0x800
