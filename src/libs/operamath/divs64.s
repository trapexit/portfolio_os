;------------------------------------------------------
;
; File:		divs64.s
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
; void DivS64 (int64 *q, int64 *r, int64 *d1, int64 *d2);
;
; Divide one 64 bit integer into another, and return the quotient and
; remainder.
;

DivS64		PROC

		STMFD	sp!, {r4-r10, lk}

		LDMIA	r2, {r2, r4}
		LDMIA	r3, {r3, r5}
		MOV	r10, #&80000000

		EOR	ip, r2, r3		; get sign of result in ip MSB
		CMP	r2, #&80000000
		MOV	ip, ip, RRX		; get sign of remainder in ip MSB
		MVNCS	r2, r2			; get absolute value
		RSBCSS	r4, r4, #0
		ADDCS	r2, r2, #1
		CMP	r3, #&80000000
		MVNCS	r3, r3			; get absolute value
		RSBCSS	r5, r5, #0
		ADDCS	r3, r3, #1

		MOV	r7, #0

		CMP	r3, #0		; check for ratio less than 2^32
		MOVNE	r7, r2
		MOVNE	r2, #0
		MOVNE	r8, #0
		BNE	%01

00		  ADDS	r2, r2, r2	; use partially unrolled loops so the code isn't HUGE
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

		  MOVS	r10, r10, ROR #4
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

		  MOVS	r10, r10, ROR #4
		 BPL	%01

		ADDS	ip, ip, ip	; adjust signs of results
		MVNCS	r2, r2
		RSBCSS	r7, r7, #0
		ADDCS	r2, r2, #1

		CMP	ip, #&80000000
		MVNCS	r8, r8
		RSBCSS	r9, r9, #0
		ADDCS	r8, r8, #1

		STMIA	r0, {r8, r9}
		STMIA	r1, {r2, r7}

		LDMFD	sp!, {r4-r10, pc}	; RTS


;------------------------------------------------------


		END

