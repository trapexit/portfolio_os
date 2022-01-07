	;; $Id: sdiv.s,v 1.3 1994/02/09 01:27:17 limes Exp $


; Fast signed divide by 10: dividend in a1
; Returns quotient in a1, remainder in a2
; Quotient is truncated (rounded towards zero).

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	|__rt_sdiv|

;|x$divide|
|__rt_sdiv|
; Signed divide of a2 by a1: returns quotient in a1, remainder in a2
; Quotient is truncated (rounded towards zero).
; Sign of remainder = sign of dividend.
; Destroys a3, a4 and ip
; Negates dividend and divisor, then does an unsigned divide; signs
; get sorted out again at the end.
; Code mostly as for udiv, except that the justification part is slightly
; simplified by knowledge that the dividend is in the range [0..#x80000000]
; (one register may be gained thereby).

; new code from Steve Landrum
	EOR	ip, r0, r1		; get sign of result in ip MSB
	CMP	r1, #&80000000
	RSBCS	r1, r1, #0		; get absolute value
	MOV	ip, ip, RRX		; get sign of remainder in ip MSB
	CMP	r0, #&80000000
	RSBCS	r0, r0, #0		; get absolute value

	MOV	r3, #0			; initialize quotient
	RSBS	r2, r0, r1, LSR #30
	SBCCS	r1, r1, r0, LSL #30
	ADC	r3, r3, r3		; 1
	RSBS	r2, r0, r1, LSR #29
	SBCCS	r1, r1, r0, LSL #29
	ADC	r3, r3, r3		; 2
	RSBS	r2, r0, r1, LSR #28
	SBCCS	r1, r1, r0, LSL #28
	ADC	r3, r3, r3		; 3
	RSBS	r2, r0, r1, LSR #27
	SBCCS	r1, r1, r0, LSL #27
	ADC	r3, r3, r3		; 4
	RSBS	r2, r0, r1, LSR #26
	SBCCS	r1, r1, r0, LSL #26
	ADC	r3, r3, r3		; 5
	RSBS	r2, r0, r1, LSR #25
	SBCCS	r1, r1, r0, LSL #25
	ADC	r3, r3, r3		; 6
	RSBS	r2, r0, r1, LSR #24
	SBCCS	r1, r1, r0, LSL #24
	ADC	r3, r3, r3		; 7
	RSBS	r2, r0, r1, LSR #23
	SBCCS	r1, r1, r0, LSL #23
	ADC	r3, r3, r3		; 8
	RSBS	r2, r0, r1, LSR #22
	SBCCS	r1, r1, r0, LSL #22
	ADC	r3, r3, r3		; 9
	RSBS	r2, r0, r1, LSR #21
	SBCCS	r1, r1, r0, LSL #21
	ADC	r3, r3, r3		; 10
	RSBS	r2, r0, r1, LSR #20
	SBCCS	r1, r1, r0, LSL #20
	ADC	r3, r3, r3		; 11
	RSBS	r2, r0, r1, LSR #19
	SBCCS	r1, r1, r0, LSL #19
	ADC	r3, r3, r3		; 12
	RSBS	r2, r0, r1, LSR #18
	SBCCS	r1, r1, r0, LSL #18
	ADC	r3, r3, r3		; 13
	RSBS	r2, r0, r1, LSR #17
	SBCCS	r1, r1, r0, LSL #17
	ADC	r3, r3, r3		; 14
	RSBS	r2, r0, r1, LSR #16
	SBCCS	r1, r1, r0, LSL #16
	ADC	r3, r3, r3		; 15
	RSBS	r2, r0, r1, LSR #15
	SBCCS	r1, r1, r0, LSL #15
	ADC	r3, r3, r3		; 16
	RSBS	r2, r0, r1, LSR #14
	SBCCS	r1, r1, r0, LSL #14
	ADC	r3, r3, r3		; 17
	RSBS	r2, r0, r1, LSR #13
	SBCCS	r1, r1, r0, LSL #13
	ADC	r3, r3, r3		; 18
	RSBS	r2, r0, r1, LSR #12
	SBCCS	r1, r1, r0, LSL #12
	ADC	r3, r3, r3		; 19
	RSBS	r2, r0, r1, LSR #11
	SBCCS	r1, r1, r0, LSL #11
	ADC	r3, r3, r3		; 20
	RSBS	r2, r0, r1, LSR #10
	SBCCS	r1, r1, r0, LSL #10
	ADC	r3, r3, r3		; 21
	RSBS	r2, r0, r1, LSR #9
	SBCCS	r1, r1, r0, LSL #9
	ADC	r3, r3, r3		; 22
	RSBS	r2, r0, r1, LSR #8
	SBCCS	r1, r1, r0, LSL #8
	ADC	r3, r3, r3		; 23
	RSBS	r2, r0, r1, LSR #7
	SBCCS	r1, r1, r0, LSL #7
	ADC	r3, r3, r3		; 24
	RSBS	r2, r0, r1, LSR #6
	SBCCS	r1, r1, r0, LSL #6
	ADC	r3, r3, r3		; 25
	RSBS	r2, r0, r1, LSR #5
	SBCCS	r1, r1, r0, LSL #5
	ADC	r3, r3, r3		; 26
	RSBS	r2, r0, r1, LSR #4
	SBCCS	r1, r1, r0, LSL #4
	ADC	r3, r3, r3		; 27
	RSBS	r2, r0, r1, LSR #3
	SBCCS	r1, r1, r0, LSL #3
	ADC	r3, r3, r3		; 28
	RSBS	r2, r0, r1, LSR #2
	SBCCS	r1, r1, r0, LSL #2
	ADC	r3, r3, r3		; 29
	RSBS	r2, r0, r1, LSR #1
	SBCCS	r1, r1, r0, LSL #1
	ADC	r3, r3, r3		; 30
	RSBS	r2, r0, r1
	SBCCS	r1, r1, r0
	ADC	r0, r3, r3		; 31 bits of result, just need the signs

	ADDS	ip, ip, ip	; adjust signs of results
	RSBCS	r1, r1, #0
	RSBMI	r0, r0, #0

	MOV	pc, r14

;	MOVS	ip, a1
;;	BEQ	|__rt_div0|
;	RSBMI	a1, a1, #0		; absolute value of divisor
;	EOR	ip, ip, a2
;	ANDS	a4, a2, #&80000000
;	ORR	ip, a4, ip, LSR #1
;	; ip bit 31  sign of dividend (= sign of remainder)
;	;    bit 30  sign of dividend EOR sign of divisor (= sign of quotient)
;	RSBNE	a2, a2, #0		; absolute value of dividend
;
;	MOV	a3, a1
;	MOV	a4, #0
;s_loop
;	CMP	a2, a3, ASL #0
;	BLS	s_shifted0mod8
;	CMP	a2, a3, ASL #1
;	BLS	s_shifted1mod8
;	CMP	a2, a3, ASL #2
;	BLS	s_shifted2mod8
;	CMP	a2, a3, ASL #3
;	BLS	s_shifted3mod8
;	CMP	a2, a3, ASL #4
;	BLS	s_shifted4mod8
;	CMP	a2, a3, ASL #5
;	BLS	s_shifted5mod8
;	CMP	a2, a3, ASL #6
;	BLS	s_shifted6mod8
;	CMP	a2, a3, ASL #7
;	MOVHI	a3, a3, ASL #8
;	BHI	s_loop
;s_loop2
;	CMP	a2, a3, ASL #7
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #7
;	CMP	a2, a3, ASL #6
;s_shifted6mod8
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #6
;	CMP	a2, a3, ASL #5
;s_shifted5mod8
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #5
;	CMP	a2, a3, ASL #4
;s_shifted4mod8
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #4
;	CMP	a2, a3, ASL #3
;s_shifted3mod8
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #3
;	CMP	a2, a3, ASL #2
;s_shifted2mod8
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #2
;	CMP	a2, a3, ASL #1
;s_shifted1mod8
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #1
;	CMP	a2, a3, ASL #0
;s_shifted0mod8
;	ADC	a4, a4, a4
;	SUBHS	a2, a2, a3, ASL #0
;	CMP	a1, a3, LSR #1
;	MOVLS	a3, a3, LSR #8
;	BLS	s_loop2
;	MOV	a1, a4
;	TST	ip, #&40000000
;	RSBNE	a1, a1, #0
;	TST	ip, #&80000000
;	RSBNE	a2, a2, #0
;	MOV	pc, lr

	END
