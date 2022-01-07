	;; $Id: OpenItem.s,v 1.2 1994/02/09 01:27:17 limes Exp $

	AREA	ASMCODE, CODE, READONLY

	EXPORT	OpenItem
OpenItem
;	mov	r12,#5
	swi	&10000+5
	mov	pc,r14

	END

