	;; $Id: swap.s,v 1.2 1994/02/09 01:27:17 limes Exp $


	AREA	|ASMCODE|, CODE, READONLY

;	armswap(int *, int v)
	EXPORT	armswap
armswap
	swp	r0,r1,[r0]
	mov	pc,r14

	EXPORT	armswapb
armswapb
	swp	r0,r1,[r0]
	mov	pc,r14

	END
