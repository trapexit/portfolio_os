	;; $Id: DeleteItem.s,v 1.2 1994/02/09 01:27:17 limes Exp $

	AREA	ASMCODE, CODE, READONLY

	EXPORT	DeleteItem
DeleteItem
;	mov	r12,#3
	swi	&10000+3
	mov	pc,r14

	END
