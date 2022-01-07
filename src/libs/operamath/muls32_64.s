;------------------------------------------------------
;
; File:		muls32_64.s
; By:		Stephen H. Landrum
; Last update:	11-Feb-93
;
; Math routines for the Opera
;
; Copyright (c) 1992, 1993, The 3DO Company, Inc.
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
; void MulS32_64 (int64 *prod, int32 m1, int32 m2);
; void MulSF16_F32 (frac32 *prod, frac16 m1, frac16 m2);
; void MulSF30_F60 (frac4 *prod, frac2 m1, frac2 m2);
;
; Multiply two signed 32 bit integers together, and get a 64 bit result.
; Alternately multiply two signed 16.16 numbers and get a 32.32 result.
; Alternately multiply two signed 2.30 numbers and get a 4.60 result.
;

MulS32_64	PROC
MulSF16_F32	PROC
MulSF30_F60	PROC

		STMFD	sp!, {r4-r5, lk}

		EOR	ip, r1, r2

		CMP	r1, #0			; take absolute values of multipliers
		RSBMI	r1, r1, #0		;  and remember product of signs
		CMP	r2, #0
		RSBMI	r2, r2, #0

		MOV	r3, r1, LSR #16		; split multiplicands into 16 bit parts
		MOV	r4, r2, LSR #16
		BIC	r1, r1, r3, LSL #16
		BIC	r2, r2, r4, LSL #16

		MUL	r5, r1, r2		; do four 16x16=>32 Multiplies
		MUL	r1, r4, r1
		MUL	r2, r3, r2
		MUL	r3, r4, r3

		ADDS	r5, r5, r1, LSL #16	; add up the pieces
		ADC	r3, r3, r1, LSR #16
		ADDS	r5, r5, r2, LSL #16
		ADC	r3, r3, r2, LSR #16

		CMP	ip, #&80000000		; negate result if necessary
		MVNCS	r3, r3
		RSBCSS	r5, r5, #0
		ADDEQ	r3, r3, #1
		
		STMIA	r0, {r3, r5}		; store the return value

		LDMFD	sp!, {r4-r5, pc}	; RTS
		

;------------------------------------------------------


		END

