*\  :ts=8 bk=0
*
* sportxfer.s:	Inner loops that perform the actual SPORT functions.
*
* Leo L. Schwab						9407.12
****************************************************************************
* Copyright 1994 The 3DO Company.  All Rights Reserved.
*
* 3DO Trade Secrets  -  Confidential and Proprietary
****************************************************************************
*			     --== RCS Log ==--
*
* $Id: sportxfer.s,v 1.2 1994/08/26 18:53:14 limes Exp $
*/
		AREA	sportxfer, CODE
		ALIGN

		INCLUDE	structs.i
		INCLUDE	inthard.i


****************************************************************************
* Data declarations.
*
		IMPORT	sportlines


****************************************************************************
* clonepages
*
* SYNOPSIS
*	nleft = clonepages (dest, src, npages, mask)
*			     r0    r1    r2     r3
*
*	int32	nleft;
*	uint32	*dest, *src;
*	int32	npages;
*	uint32	mask;
*
* DESCRIPTION
*	Performs the clone operation on the VRAM parts.  Repeatedly checks
*	to see if it's safe.  Returns the number of pages yet to be done.
*
		EXPORT	clonepages
clonepages	ROUT

		cmp	r2, #0
		movle	r0, #0
		movle	pc, lr		; Nothing to do

		stmdb	sp, {r4-r12,lr}

	;------	Replicate mask value for towers.
		mov	r4, r3
		mov	r5, r3
		mov	r6, r3
		mov	r7, r3
		mov	r8, r3
		mov	r9, r3
		mov	r10, r3

	;------	Initialize VRAM part with source page.
		ldr	r11, [r1]	; We don't need R1 after this...

	;------	Initialize video line pointers.
		mov	r14, #CLIO
		add	r14, r14, #VCNT-CLIO
		ldr	r1, sportlines

	;------	Compute jump into piecemeal tower.
		ands	r11, r2, #7
		adr	r12, %0
		sub	r12, r12, r11, ASL #2
		mov	pc, r12

	;------	Piecemeal tower; one page per instruction.
		str	r3, [r0], #4
		str	r3, [r0], #4
		str	r3, [r0], #4
		str	r3, [r0], #4
		str	r3, [r0], #4
		str	r3, [r0], #4
		str	r3, [r0], #4
0
	;------	Record pages already done, and compute next jump.
		bics	r2, r2, #7	; Mask off pages already done
		beq	%999		;  Nothing left to do

		ands	r11, r2, #&38	; Compute jump into fat tower
		beq	%1		;  Exact multiple of 64
		adr	r12, %2
		sub	r12, r12, r11, ASR #1
		sub	r2, r2, r11	; Subtract pages about to be done
		add	r2, r2, #64	; Fencepost
		mov	pc, r12

	;------	Fat tower; 8 pages per instruction, 64 pages per loop.
1		stmia	r0!, {r3-r10}	; 8 pages
		stmia	r0!, {r3-r10}	; 8 pages
		stmia	r0!, {r3-r10}	; 8 pages
		stmia	r0!, {r3-r10}	; 8 pages
		stmia	r0!, {r3-r10}	; 8 pages
		stmia	r0!, {r3-r10}	; 8 pages
		stmia	r0!, {r3-r10}	; 8 pages
		stmia	r0!, {r3-r10}	; 8 pages
2
		subs	r2, r2, #64
		beq	%999

		ldr	r11, [r14]	; Get VCNT
		mov	r11, r11, LSL #21  ; Shift off non-VCNT bits
		ldr	r12, [r1, #4]	; Get sportlines[1]
		cmp	r12, r11, LSR #21  ; Shift VCNT back down and compare
		bhi	%1		; Still okay...

	;------	All done.
999		ldr	r11, [r0, #-4]	; Release write lock in VRAM parts
		mov	r0, r2		; Return # of unprocessed pages
		ldmdb	sp, {r4-r12,pc}	; Implied RTS


****************************************************************************
* copypages
*
* SYNOPSIS
*	nleft = copypages (dest, src, npages, mask)
*			    r0    r1    r2     r3
*
*	int32	nleft;
*	uint32	*dest, *src;
*	int32	npages;
*	uint32	mask;
*
* DESCRIPTION
*	Performs the copy operation on the VRAM parts.  Repeatedly checks
*	to see if it's safe.  Returns the number of pages yet to be done.
*
		EXPORT	copypages
copypages	ROUT

		cmp	r2, #0
		movle	r0, #0
		movle	pc, lr		; Nothing to do

		stmdb	sp, {r4-r7}

	;------	Initialize video line pointers.
		mov	r6, #CLIO
		add	r6, r6, #VCNT-CLIO
		ldr	r7, sportlines

	;------	Compute jump into tower.
		ands	r5, r2, #7
		beq	%0		; Exact multiple of 8
		adr	r4, %1
		sub	r4, r4, r5, ASL #3
		sub	r2, r2, r5	; Adjust count of remaining pages
		add	r2, r2, #8	; Fencepost
		mov	pc, r4

	;------	Main copy tower.
0		ldr	r4, [r1], #4	; Load page
		str	r3, [r0], #4	; Write page
		ldr	r4, [r1], #4
		str	r3, [r0], #4
		ldr	r4, [r1], #4
		str	r3, [r0], #4
		ldr	r4, [r1], #4
		str	r3, [r0], #4
		ldr	r4, [r1], #4
		str	r3, [r0], #4
		ldr	r4, [r1], #4
		str	r3, [r0], #4
		ldr	r4, [r1], #4
		str	r3, [r0], #4
		ldr	r4, [r1], #4
		str	r3, [r0], #4
1
		subs	r2, r2, #8	; Decrement page count
		beq	%999		; We done?

		ldr	r4, [r6]	; Get VCNT
		mov	r4, r4, LSL #21	; Shift off non-VCNT bits
		ldr	r5, [r7, #4]	; Get sportlines[1]
		cmp	r5, r4, LSR #21	; Shift VCNT back down and compare
		bhi	%0		; Still okay...

	;------	All done.
999		ldr	r4, [r1, #-4]	; Release write lock in VRAM parts
		mov	r0, r2		; Return # of unprocessed pages
		ldmdb	sp, {r4-r7}
		mov	pc, lr

		END
