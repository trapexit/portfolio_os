;------------------------------------------------------
;
; File:		divu64.s
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
; void DivU64 (int64 *q, int64 *r, int64 *d1, int64 *d2);
;
; Divide one 64 bit integer into another, and return the quotient and
; remainder.
;

DivU64		PROC

		STMFD	sp!, {r4-r9, lk}

		LDMIA	r2, {r2, r4}
		LDMIA	r3, {r3, r5}
		MOV	ip, #&80000000

		MOV	r7, #0

		CMP	r3, #0		; check for ratio less than 2^32
		MOVNE	r7, r2
		MOVNE	r2, #0
		MOVNE	r8, #0
		BNE	%01

00		  ADDS	r2, r2, r2	; use a partially unrolled loop so the code's not HUGE
		  ADC	r7, r7, r7
		  CMP	r7, r5
		  SUBCS	r7, r7, r5
		  ADCS	r8, r8, r8

		  ADDS	r2, r2, r2
		  ADC	r7, r7, r7
		  CMP	r7, r5
		  SUBCS	r7, r7, r5
		  ADCS	r8, r8, r8

		  ADDS	r2, r2, r2
		  ADC	r7, r7, r7
		  CMP	r7, r5
		  SUBCS	r7, r7, r5
		  ADCS	r8, r8, r8

		  ADDS	r2, r2, r2
		  ADC	r7, r7, r7
		  CMP	r7, r5
		  SUBCS	r7, r7, r5
		  ADCS	r8, r8, r8

		  MOVS	ip, ip, ROR #4
		 BPL	%00

01		  ADDS	r4, r4, r4
		  ADCS	r7, r7, r7
		  ADC	r2, r2, r2
		  CMP	r7, r5
		  SBCS	r6, r2, r3
		  SUBCS	r7, r7, r5
		  MOVCS	r2, r6
		  ADCS	r9, r9, r9

		  ADDS	r4, r4, r4
		  ADCS	r7, r7, r7
		  ADC	r2, r2, r2
		  CMP	r7, r5
		  SBCS	r6, r2, r3
		  SUBCS	r7, r7, r5
		  MOVCS	r2, r6
		  ADCS	r9, r9, r9

		  ADDS	r4, r4, r4
		  ADCS	r7, r7, r7
		  ADC	r2, r2, r2
		  CMP	r7, r5
		  SBCS	r6, r2, r3
		  SUBCS	r7, r7, r5
		  MOVCS	r2, r6
		  ADCS	r9, r9, r9

		  ADDS	r4, r4, r4
		  ADCS	r7, r7, r7
		  ADC	r2, r2, r2
		  CMP	r7, r5
		  SBCS	r6, r2, r3
		  SUBCS	r7, r7, r5
		  MOVCS	r2, r6
		  ADCS	r9, r9, r9

		  MOVS	ip, ip, ROR #4
		 BPL	%01

		STMIA	r0, {r8, r9}
		STMIA	r1, {r2, r7}

		LDMFD	sp!, {r4-r9, pc}	; RTS


;------------------------------------------------------


		END

