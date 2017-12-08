
.cpu cortex-m4
.thumb

.thumb_func
.global _start
_start:
stacktop: .word 0x20001000
.word reset
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang
.word hang

.word irq0
.word irq1
.word irq2
.word irq3
.word irq4
.word irq5
.word irq6
.word irq7
.word irq8
.word irq9
.word irq10
.word irq11
.word irq12
.word irq13
.word irq14
.word irq15
.word irq16
.word irq17
.word irq18
.word irq19

.thumb_func
reset:
    bl notmain
    b hang
    b hang

.thumb_func
hang:   b .
    b hang 

.thumb_func
irq0:    
    bl myirq0
    b hang
    b hang

.thumb_func
irq1:    
    bl myirq1
    b hang
    b hang

.thumb_func
irq2:    
    bl myirq2
    b hang
    b hang

.thumb_func
irq3:    
    bl myirq3
    b hang
    b hang

.thumb_func
irq4:    
    bl myirq4
    b hang
    b hang

.thumb_func
irq5:    
    bl myirq5
    b hang
    b hang

.thumb_func
irq6:    
    bl myirq6
    b hang
    b hang

.thumb_func
irq7:    
    bl myirq7
    b hang
    b hang

.thumb_func
irq8:    
    bl myirq8
    b hang
    b hang

.thumb_func
irq9:    
    bl myirq9
    b hang
    b hang

.thumb_func
irq10:   
    bl myirq10
    b hang
    b hang

.thumb_func
irq11:   
    bl myirq11
    b hang
    b hang

.thumb_func
irq12:   
    bl myirq12
    b hang
    b hang

.thumb_func
irq13:   
    bl myirq13
    b hang
    b hang

.thumb_func
irq14:   
    bl myirq14
    b hang
    b hang

.thumb_func
irq15:   
    bl myirq15
    b hang
    b hang

.thumb_func
irq16:   
    bl myirq16
    b hang
    b hang

.thumb_func
irq17:   
    bl myirq17
    b hang
    b hang

.thumb_func
irq18:   
    bl myirq18
    b hang
    b hang

.thumb_func
irq19:   
    bl myirq19
    b hang
    b hang


.thumb_func
.globl PUT32
PUT32:
    str r1,[r0]
    bx lr
