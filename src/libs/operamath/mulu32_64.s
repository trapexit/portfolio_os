;------------------------------------------------------
;
; File:		mulu32_64.s
; By:		Stephen H. Landrum
; Last update:	32-Oct-92
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
; void MulU32_64 (int64 *prod, uint32 m1, uint32 m2);
; void MulUF16_F32 (frac32 *prod, frac16 m1, frac16 m2);
; void MulUF30_F60 (frac60 *prod, frac30 m1, frac30 m2);
;
; Multiply two unsigned 32 bit integers together, and get a 64 bit result.
; Alternately multiply two unsigned 16.16 numbers and get a 32.32 result.
; Alternately multiply two unsigned 2.30 numbers and get a 4.60 result.
;

MulU32_64	PROC
MulUF16_F32	PROC
MulUF30_F60	PROC

		STMFD	sp!, {r4, lk}

		MOV	r3, r1, LSR #16		; split multiplicands into 16 bit parts
		MOV	r4, r2, LSR #16
		BIC	r1, r1, r3, LSL #16
		BIC	r2, r2, r4, LSL #16

		MUL	ip, r1, r2		; do four 16x16=>32 multiplies
		MUL	r1, r4, r1
		MUL	r2, r3, r2
		MUL	r3, r4, r3

		ADDS	ip, ip, r1, LSL #16	; add up the pieces
		ADC	r3, r3, r1, LSR #16
		ADDS	r4, ip, r2, LSL #16
		ADC	r3, r3, r2, LSR #16

		STMIA	r0, {r3, r4}		; store the return value

		LDMFD	sp!, {r4, pc}	; RTS


;------------------------------------------------------


		END

