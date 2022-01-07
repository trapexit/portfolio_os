	;; $Id: read4bytes.s,v 1.3 1994/02/09 01:27:17 limes Exp $

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

	EXPORT	Write4Bytes
Write4Bytes
	mov	r12,r1	; use r12 as ptr to src
	mov	r3,r0
	mov	r2,r0,lsr #8
	mov	r1,r0,lsr #16
	mov	r0,r0,lsr #24
	stmia	r12,{r0,r1,r2,r3}
	mov	pc,lk

	END
