	;; $Id: w64copy.s,v 1.3 1994/09/22 17:39:56 limes Exp $

	GET	registers.i

;;; w64copy, w64copy_f, w64copy_r: ultra-fast copy routines for use
;;; when it is known that the blocks are a multiple of 64 bytes long
;;; and are on longword bounds.

	AREA	|ASMCODE|, CODE, READONLY
	ALIGN	4

	EXPORT	w64copy
w64copy
	sub	r3,r1,r0
	cmp	r3,r2
	bcc	w64copy_r

;;; FALLS THRU TO ...

	EXPORT	w64copy_f
w64copy_f
	mov	ip,sp
	stmdb	sp!,{r4-r10,fp,ip,lr,pc}
	sub	fp,ip,#4
	
w64fl
	ldmia	r0!,{r3-r10}
	stmia	r1!,{r3-r10}
	ldmia	r0!,{r3-r10}
	stmia	r1!,{r3-r10}
	subs	r2,r2,#&40
	bne	w64fl
	
	ldmdb	fp,{r4-r10,fp,sp,pc}

	EXPORT	w64copy_r
w64copy_r
	mov	ip,sp
	stmdb	sp!,{r4-r10,fp,ip,lr,pc}
	sub	fp,ip,#4
	
	add	r0,r2,r0
	add	r1,r2,r1
w64rl
	ldmdb	r0!,{r3-r10}
	stmdb	r1!,{r3-r10}
	ldmdb	r0!,{r3-r10}
	stmdb	r1!,{r3-r10}
	subs	r2,r2,#&40
	bne	w64rl
	
	ldmdb	fp,{r4-r10,fp,sp,pc}

	END

