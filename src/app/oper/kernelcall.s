	;; $Id: kernelcall.s,v 1.3 1994/07/28 18:26:10 vertex Exp $

;--------------------------------------------------
	GET registers.i
	GET structs.i

	AREA	ASMCODE2, CODE

;--------------------------------------------------
; kernelcall
; supervisor call only
;	kernelcall(r0,r1,r2,r3,func);
;

	IMPORT	|_KernelBase|
adrKB	DCD	|_KernelBase|

	EXPORT	kernelcall
kernelcall
	stmfd	sp!,{r9,r14}
	ldr	r9,adrKB	;
	ldr	r9,[r9]		; get kernelbase in r9
	mov	r14,r15		;get return address
	ldr	r15,[sp,#8]	;jump to routine
	ldmfd	sp!,{r9,r15}

	END

