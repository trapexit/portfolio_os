	;; $Id: Superbcopy.s,v 1.2 1994/02/09 01:27:17 limes Exp $

	AREA	ASMCODE, CODE, READONLY

	EXPORT	Superbcopy
Superbcopy
;	mov	r12,#27
	swi	&10000+27
	mov	pc,r14

	END

