	;; $Id: getpsr.s,v 1.7 1994/09/23 17:30:25 vertex Exp $


	AREA	|ASMCODE|, CODE, READONLY

	ALIGN	4		; alignment paranoia

	EXPORT	getPSR		; get PSR of us.
getPSR
;	32bit mode
	mrs	r0,CPSR		; get mode bits
	and	r0,r0,#&1f
	mov	pc,r14		; quick return

	EXPORT	getSPSR		; get SPSR of us.
getSPSR
;	32bit mode
	mrs	r0,SPSR		; get mode bits
;	and	r0,r0,#&1f
	mov	pc,r14		; quick return

	EXPORT	setSPSR
setSPSR
;	set the saved processor status register
	msr	SPSR_all,r0
	mov	pc,r14

	EXPORT	Disabled
;	interrupts disabled?
Disabled
;	32bit mode
	mrs	r0,CPSR
	and	r0,r0,#&c0
	mov	pc,r14

	EXPORT irqDisabled
irqDisabled
	mov	r2,r14
	bl	Disabled
	and	r0,r0,#&80
	mov	r14,r2
	mov	pc,r14

	END

