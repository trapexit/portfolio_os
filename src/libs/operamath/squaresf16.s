;------------------------------------------------------
;
; File:		squaresf16.s
; By:		Stephen H. Landrum
; Last update:	23-Oct-92
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


; typedef struct int64 { ulong hi, ulong lo } int64, uint64, frac32, ufrac32;
; typedef long frac16, int32;
; typedef ulong ufrac16, uint32;




;------------------------------------------------------
;
; ufrac16 SquareSF16 (frac16 m);
;
; Return the square of a signed 16.16 integer.
; Overflows are not detected.  Lower bits are truncated.
;

SquareSF16	PROC

		CMP	r0, #0			; take absolute value of multiplier
		RSBMI	r0, r0, #0

		MOV	r2, r0, LSR #16		; split multiplicand into 16 bit parts
		BIC	ip, r0, r2, LSL #16

		MUL	r1, ip, ip		; r1 = BB
		MOV	r1, r1, LSR #16		; r1 = BB>>16
		MLA	r0, r2, r0, r1		; r0 = AA<<16 + AB + BB>>16
		MLA	r0, r2, ip, r0		; r0 = AA<<16 + AB+AB + BB>>16

		MOV	pc, lk			; RTS


;------------------------------------------------------


		END

