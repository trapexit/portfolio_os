	;; $Id: softreset.s,v 1.6 1994/02/09 01:27:17 limes Exp $
	
;--------------------------------------------------
	GET registers.i
	GET structs.i
	GET inthard.i

	AREA	ASMCODE2, CODE

;--------------------------------------------------
; int32 SoftReset(void)
; supervisor call only
;

debug	DCD	&e59ff00c
	EXPORT	SoftReset
SoftReset
	mrs	r3,SPSR		; get user psr
	mrs	r2,CPSR		; get current psr
	stmdb	sp!,{r2-r12,r14}
	mov	r3,#CLIO
	mov	r0,#&30		; SOFTRES+CLRDIP
	str	r0,[r3,#CSTATBits-CLIO]	; trigger sr
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
	mov	r0,r0
;waitForSR
	;tst	r0,#0		; back from sr?
	;bne	waitForSR

	ldmia	sp!,{r2-r12,r14}
	msr	SPSR_all,r3	; restore user psr
	msr	CPSR_all,r2
;	ldr	r2,debug
;	mov	r3,#&40
;	str	r2,[r3]		; restore debug vector
	MOV	PC,r14		; return to caller

	END

