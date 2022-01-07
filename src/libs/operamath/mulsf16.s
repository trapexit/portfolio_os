;------------------------------------------------------
;
; File:		mulsf16.s
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
; frac16 MulSF16 (frac16 m1, frac16 m2);
;
; Multiply two signed 16.16 integers together, and get a 16.16 result.
; Overflows are not detected.  Lower bits are truncated.
;

MulSF16		PROC

		STMFD	sp!, {r4, lk}

		EOR	ip, r0, r1

		TST	r0, #&80000000		; take absolute values of multipliers
		RSBNE	r0, r0, #0		;  and remember product of signs
		TST	r1, #&80000000
		RSBNE	r1, r1, #0

		MOV	r2, r0, LSR #16		; split multiplicands into 16 bit parts
		MOV	r3, r1, LSR #16
		BIC	r4, r0, r2, LSL #16
		BIC	r1, r1, r3, LSL #16

		MUL	r4, r1, r4		; r4 = BD
		MOV	r4, r4, LSR #16		; r4 = BD>>16
		MLA	r0, r3, r0, r4		; r0 = AC<<16 + BC + BD>>16
		MLA	r0, r2, r1, r0		; r0 = AC<<16 + AD+BC + BD>>16

		TST	ip, #&80000000		; correct sign if necessary
		RSBNE	r0, r0, #0

		LDMFD	sp!, {r4, pc}		; RTS


;------------------------------------------------------


		END

