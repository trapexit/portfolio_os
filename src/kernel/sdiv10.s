	;; $Id: sdiv10.s,v 1.3 1994/04/08 18:16:02 limes Exp $


; Fast signed divide by 10: dividend in a1
; Returns quotient in a1, remainder in a2
; Quotient is truncated (rounded towards zero).

	AREA	|ASMCODE|, CODE, READONLY
	ALIGN	4		; alignment paranoia

	EXPORT	|__rt_sdiv10|
|__rt_sdiv10|
	MOVS	a4, a1
	RSBMI	a1, a1, #0
	MOV	a2, a1
	MOV	a1, a1, LSR #1
	ADD	a1, a1, a1, LSR #1
	ADD	a1, a1, a1, LSR #4
	ADD	a1, a1, a1, LSR #8
	ADD	a1, a1, a1, LSR #16
	MOV	a1, a1, LSR #3
	ADD	a3, a1, a1, ASL #2
	SUB	a2, a2, a3, ASL #1
	CMP	a2, #10
	ADDGE	a1, a1, #1
	SUBGE	a2, a2, #10
	CMP	a4, #0
	RSBMI	a1, a1, #0
	RSBMI	a2, a2, #0
 [	{CONFIG}=26
	MOVS	pc, lr
 |
	MOV	pc, lr
 ]

	END
