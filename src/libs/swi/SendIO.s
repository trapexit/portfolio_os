	;; $Id: SendIO.s,v 1.2 1994/02/09 01:27:17 limes Exp $

	AREA	ASMCODE, CODE, READONLY

	EXPORT	SendIO
SendIO
;	mov	r12,#24
	swi	&10000+24
	mov	pc,r14

	END

