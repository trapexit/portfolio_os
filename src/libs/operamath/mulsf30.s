;------------------------------------------------------
;
; File:		mulsf30.s
; By:		Stephen H. Landrum
; Last update:	5-Aug-93
;
; Math routines for the Opera
;
; Copyright (c) 1992, 1993, The 3DO Company
; All rights reserved.
; This document is proprietary and confidential
;
;------------------------------------------------------


		INCLUDE	registers.i
		INCLUDE macros.i
		INCLUDE structs.i


;------------------------------------------------------

		AREA	ASMCODE2, CODE

;------------------------------------------------------
;
; frac30 MulSF30 (frac30 m1, frac30 m2);
;
; Multiply two signed 2.30 fractions together, and get a 2.30 result.
; Overflows are not detected.  Lower bits are truncated.
;

MulSF30		PROC

		STMFD	sp!, {r4, lk}

		EOR	ip, r1, r0

		CMP	r1, #0			; take absolute values of multipliers
		RSBMI	r1, r1, #0		;  and remember product of signs
		CMP	r0, #0
		RSBMI	r0, r0, #0

		MOV	r3, r1, LSR #16		; split multiplicands into 16 bit parts
		MOV	r4, r0, LSR #16
		BIC	r1, r1, r3, LSL #16
		BIC	r0, r0, r4, LSL #16

		MUL	r2, r1, r0		; do four 16x16=>32 Multiplies
		MUL	r1, r4, r1
		MUL	r0, r3, r0
		MUL	r3, r4, r3

		ADDS	r2, r2, r1, LSL #16	; add up the pieces
		ADC	r3, r3, r1, LSR #16
		ADDS	r2, r2, r0, LSL #16
		ADC	r3, r3, r0, LSR #16

		CMP	ip, #&80000000		; negate result if necessary
		MVNCS	r3, r3
		RSBCSS	r2, r2, #0
		ADDEQ	r3, r3, #1

		MOV	r0, r3, LSL #2
		ORR	r0, r0, r2, LSR #30				

		LDMFD	sp!, {r4, pc}	; RTS


;------------------------------------------------------


		END

