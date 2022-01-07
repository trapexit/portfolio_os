	;; $Id: getpsr.s,v 1.4 1994/02/09 01:27:17 limes Exp $

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	getPSR		; get PSR of us.
getPSR
;	32bit mode
	mrs	r0,CPSR		; get mode bits
	and	r0,r0,#&1f
	mov	pc,r14		; quick return

	END
