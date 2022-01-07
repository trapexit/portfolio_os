	;; $Id: udiv10.s,v 1.2 1994/02/09 01:27:17 limes Exp $

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	|__rt_udiv10|
|__rt_udiv10|                                 ; udiv10 ;
;
; Fast unsigned divide by 10: dividend in a1, divisor in a2.
; Returns quotient in a1, remainder in a2.
; Also destroys a3.
;
; Calculate x / 10 as (x * 2**32/10) / 2**32.
; That is, we calculate the most significant word of the double-length
; product. In fact, we calculate an approximation which may be 1 off
; because we've ignored a carry from the least significant word we didn't
; calculate. We correct for this by insisting that the remainder < 10
; and by incrementing the quotient if it isn't.

        MOV     a2, a1
        MOV     a1, a1, LSR #1
        ADD     a1, a1, a1, LSR #1
        ADD     a1, a1, a1, LSR #4
        ADD     a1, a1, a1, LSR #8
        ADD     a1, a1, a1, LSR #16
        MOV     a1, a1, LSR #3
        ADD     a3, a1, a1, ASL #2
        SUB     a2, a2, a3, ASL #1
        CMP     a2, #10
        ADDGE   a1, a1, #1
        SUBGE   a2, a2, #10
 [	{CONFIG}=26
        MOVS    pc, r14
 |
        MOV    pc, r14
 ]

	END
