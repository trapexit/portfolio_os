	;; $Id: w64zero.s,v 1.2 1994/09/22 18:40:59 limes Exp $

	GET	registers.i

;;; w64zero
;;; ultra-fast block clear.
;;; block must start on a longword bound and
;;; must be a multiple of 64 bytes long.

	AREA	|ASMCODE|, CODE, READONLY
	ALIGN	4

	EXPORT	w64zero
w64zero
	mov	ip,sp
	stmdb	sp!,{r4-r9,fp,ip,lr,pc}
	sub	fp,ip,#4
	
	mov	r2,#0
	mov	r3,#0
	mov	r4,#0
	mov	r5,#0
	mov	r6,#0
	mov	r7,#0
	mov	r8,#0
	mov	r9,#0
w64zl
	stmia	r0!,{r2-r9}
	stmia	r0!,{r2-r9}
	subs	r1,r1,#&40
	bne	w64zl
	
	ldmdb	fp,{r4-r9,fp,sp,pc}

	END

