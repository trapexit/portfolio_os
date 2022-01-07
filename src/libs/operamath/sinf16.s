;------------------------------------------------------
;
; File:		sinf16.s
; By:		Stephen H. Landrum
; Last update:	14-Dec-92
;
; Math routines for the Opera
;
; Copyright (c) 1992, The 3DO Company, Inc.
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

		IMPORT	_sintable

CIRCLEUNITS	EQU	16384
CIRCLEBITS	EQU	14

CIRCLESHIFT	EQU	(24-CIRCLEBITS)

;------------------------------------------------------
;
; frac16 SinF16 (frac16 x)
; frac16 CosF16 (frac16 x)
;
; Compute the 16.16 sine or cosine of a 16.16 fraction (assuming that there are 256.0 units in a
; circle.  Alternatively, a 32 bit integer could be passed in assuming 16,777,216 units in a
; circle.
;

CosF16		PROC

		ADD	r0, r0, #&00400000	; in case of cosine, take sine of next quadrant

SinF16		PROC

		TST	r0, #&00400000		; check for second quadrant
		RSBNE	r0, r0, #&00800000	; complement value
		BIC	r0, r0, #&ff000000	; mask off unused bits
		CMP	r0, #&00800000		; check for second half of circle
		SUBCS	r0, r0, #&00800000	; move to first half

		MOV	r2, r0, LSR #CIRCLESHIFT
		BIC	r0, r0, r2, LSL #CIRCLESHIFT
		LDR	r3, PTR_sintable
		LDR	r1, [r3, r2, LSL #2]	; get value from table
		MOV	r1, r1, LSR #CIRCLESHIFT
		ADD	r2, r2, #1
		LDR	r2, [r3, r2, LSL #2]	; get next value from table
		MOV	r2, r2, LSR #CIRCLESHIFT
		MUL	r2, r0, r2
		RSB	r0, r0, #1<<CIRCLESHIFT
		MLA	r0, r1, r0, r2		; interpolate between points
		MOV	r0, r0, LSR #15
		RSBCS	r0, r0, #0		; negate if necessary (carry from compare above)

		MOV	pc, lk

		

;------------------------------------------------------

PTR_sintable	DCD	_sintable

;------------------------------------------------------

		END


