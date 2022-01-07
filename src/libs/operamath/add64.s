;------------------------------------------------------
;
; File:		add64.s
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
; void Add64 (int64 *r, int64 *a1, int64 *a2);
; void AddF32 (frac32 *r, frac32 *a1, frac32 *a2);
; void AddF60 (frac60 *r, frac60 *a1, frac60 *a2);
;
; Add two 64 bit integers together and return the 64 bit result
; Alternately add two 32.32 fractions or two 4.60 fractions
;

Add64		PROC
AddF32		PROC
AddF60		PROC

		LDMIA	r1, {r3, ip}	; get addends
		LDMIA	r2, {r1, r2}

		ADDS	r2, r2, ip	; add
		ADC	r1, r1, r3

		STMIA	r0, {r1, r2}	; store the result

		MOV	pc, lk		; RTS


;------------------------------------------------------


		END

