; $Id: acs.s,v 1.1 1994/07/07 21:34:50 dale Exp $

	GET	structs.i
	GET	nodes.i
	GET	list.i

	AREA    |ASMCODE|, CODE, READONLY

;;; ------------------------------------------------------------------------
;;; CheckACS: trigger acs execution if needed

	EXPORT	CheckACS
	EXPORT	ACS_StackTop
	EXPORT	ACS_StackLim
	IMPORT	RunACS
CheckACS
	ldr	r0,ACS_Enable
	cmp	r0,#0		; if ACS is not enabled,
	moveq	pc,lr		;    go no further.

	ldr	r0,ACSlistp
	ldr	r1,[r0,#l_Head]
	add	r2,r0,#l_filler
	cmp	r1,r2		; if the list is empty
	cmpne	r1,#0		; or uninitialized (ick),
	moveq	pc,lr		;    just return.

	mov	r2,#0		; prevent ACS from restarting
	str	r2,ACS_Enable

	mov	r0,sp		; 				R0 has FIRQ STACK PTR
	mov	r1,lr		; 				R1 has FIRQ LINK REG
	mrs	r2,spsr		; 				R2 has FIRQ SAVED PSR

	mrs	r3,cpsr
	add	r3,r3,#2
	msr	cpsr,r3		; switch to supv mode
	nop

	mov	r3,sp		; 				R3 has SUPV STACK PTR
	mov	r4,lr		; 				R4 has SUPV LINK REG

	ldr	sp,ACS_StackTop
	stmfd	sp!,{r0-r12}	; save state we will need later
	ldr	sl,ACS_StackLim

	bl	RunACS		; run the ACS list

	mov	r2,#1
	str	r2,ACS_Enable	; OK to run ACS now

	ldmfd	sp!,{r0-r12}	; restore state
	mov	lr,r4		;				SUPV LINK REG from R4
	mov	sp,r3		;				SUPV STACK PTR from R3
	
	mrs	r4,cpsr
	sub	r4,r4,#2
	msr	cpsr,r4		; switch to firq mode
	nop

	msr	spsr,r2		;				FIRQ SAVED PSR from R2
	nop

	mov	lr,r1		;				FIRQ LING REG from R1
	mov	sp,r0		;				FIRQ STACK PTR from R0

	mov	pc,lr		; back out via FIRQ continuation

	EXPORT	ACSlistp
	EXPORT	ACS_Enable
ACS_Enable	DCD	0
ACS_StackTop	DCD	0
ACS_StackLim	DCD	0
ACSlistp	DCD	0
