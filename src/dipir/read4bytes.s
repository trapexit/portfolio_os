	;; $Id: read4bytes.s,v 1.3 1994/05/26 18:49:32 markn Exp $

	GET registers.i
	GET structs.i

	AREA	ASMCODE2, CODE


	EXPORT	Read4Bytes
Read4Bytes
	ldmia	r0,{r0,r1,r2,r3}	; get 4 longs as a time

; the following ands are not needed on modern chips
;	and	r1,r1,#255	; clear upper 24 bits
;	and	r2,r2,#255	; clear upper 24 bits
;	and	r3,r3,#255	; clear upper 24 bits

	orr	r0,r3,r0,LSL #24
	orr	r0,r0,r1,LSL #16
	orr	r0,r0,r2,LSL #8
	mov	pc,lk

	EXPORT	ReadNBytes
ReadNBytes
;	ReadNBytes(src,dst,cnt)
;	This routine reads 8 bytes at a time off the expansion bus (src)
;	formats them into 2 words and transfers them to memory (dst).
;	cnt must be a multiple of 8.
;	On an arm this loop runs at 1.37Mbytes/sec

	stmfd	sp!,{r4,r5,r6,r7,r8,r9,lk}	; Need a bunch of registers
	mov	r8,r0			; src
	mov	lk,r1			; dst
	movs	r9,r2,LSR #3		; cnt/8
	ldmeqfd	sp!,{r4,r5,r6,r7,r8,r9,pc}

myloop
	ldmia	r8,{r0,r1,r2,r3,r4,r5,r6,r7}	; get 8 longs at a time
	orr	r0,r3,r0,LSL #24
	orr	r0,r0,r1,LSL #16
	orr	r0,r0,r2,LSL #8

	orr	r4,r7,r4,LSL #24
	orr	r4,r4,r5,LSL #16
	orr	r4,r4,r6,LSL #8

	stmia	lk!,{r0,r4}

	subs	r9,r9,#1
	bne	myloop
	
	ldmfd	sp!,{r4,r5,r6,r7,r8,r9,pc}

	END
