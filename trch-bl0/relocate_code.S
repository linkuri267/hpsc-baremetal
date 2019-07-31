
/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  relocate - common relocation function for ARM U-Boot
 *
 *  Copyright (c) 2013  Albert ARIBAUD <albert.u.boot@aribaud.net>
 */
/*
 *   2019
 *   USC/ISI
 *   David Kang (dkang@isi.edu)
 *
 *   Ported for Cortex-M4
 *
*/

#define R_ARM_RELATIVE		23

/*
 * void relocate_code(addr_moni)
 *
 * This function relocates the monitor code.
 *
 * NOTE:
 * To prevent the code below from containing references with an R_ARM_ABS32
 * relocation record type, we never refer to linker-defined symbols directly.
 * Instead, we declare literals which contain their relative location with
 * respect to relocate_code, and at run time, add relocate_code back to them.
 */

.cpu cortex-m4
.thumb

.thumb_func
.global relocate_code
relocate_code:
//	add	lr, lr, r0
	ldr	r1, =__image_copy_start	/* r1 <- SRC &__image_copy_start */
	sub	r4, r0, r1		/* r4 <- relocation offset */
	beq	relocate_done		/* skip relocation */
	ldr	r2, =__image_copy_end	/* r2 <- SRC &__image_copy_end */
copy_loop:
	ldmia	r1!, {r6-r7}		/* copy from source address [r1]    */
	stmia	r0!, {r6-r7}		/* copy to   target address [r0]    */
	cmp	r1, r2			/* until source end address [r2]    */
	blo	copy_loop

	/*
	 * fix .rel.dyn relocations
	 */
	ldr	r2, =__rel_dyn_start	/* r2 <- SRC &__rel_dyn_start */
	ldr	r3, =__rel_dyn_end	/* r3 <- SRC &__rel_dyn_end */
fixloop:
	ldmia	r2!, {r0-r1}		/* (r0,r1) <- (SRC location,fixup) */
	movs	r5, #0xff
	and	r1, r1, r5
	cmp	r1, #R_ARM_RELATIVE
	bne	fixnext

	/* relative fix: increase location by offset */
	add	r0, r0, r4
	ldr	r1, [r0]
	add	r1, r1, r4
	str	r1, [r0]
fixnext:
	cmp	r2, r3
	blo	fixloop

relocate_done:
//	ldr	r1, =__image_copy_start	
	bx	lr

.thumb_func
.global clean_and_jump 
clean_and_jump:
	movs	r2, #0x1
	orr	r0, r0, r2
	mov	lr, r0	/* destination */
	mov	sp, r1	/* new SP */
	bx lr

