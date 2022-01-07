	;; $Id: kprintf.s,v 1.2 1994/02/09 01:27:17 limes Exp $

	AREA	ASMCODE, CODE, READONLY

	EXPORT	kprintf
kprintf
;	mov	r12,#14
;	swi	0
	swi	&10000+14
	mov	pc,r14

	END

