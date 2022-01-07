	;; $Id: sdivtest.s,v 1.2 1994/02/09 01:27:17 limes Exp $


; test for division by zero (used when division is voided)

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	|__rt_divtest|

__rt_divtest
	mov	pc,r14

	END
