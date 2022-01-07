	;; $Id: setjmp.s,v 1.3 1994/02/09 01:27:17 limes Exp $
;
;	setjmp.s
;

	AREA	|ASMCODE|, CODE, READONLY

	^ 0
sj_v1   #       4
sj_v2   #       4
sj_v3   #       4
sj_v4   #       4
sj_v5   #       4
sj_v6   #       4
sj_sl   #       4
sj_fp   #       4
sj_sp   #       4
sj_pc   #       4
sj_end	#	4

;
;	no support for floating point hardware
;
;	no support for noncontiguous stack frames

	EXPORT	setjmp
setjmp
; save everything that might count as a register variable value.
	STMIA   a1!, {v1-v6, sl, fp, sp, lr}
	MOV     a1, #0
	mov	pc,lr

	EXPORT	longjmp
longjmp
	add	r2,r0,#sj_end	; save ptr to env here
	movs	r0,r1	; put return arg in r0
	moveq	r0,#1	; make sure it is <>0
	ldmdb	r2,{v1-v6,sl,fp,sp,pc}


	END
