;------------------------------------------------------
;
; File:		neg64.s
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
; void Neg64 (int64 *dest, int64 *src);
; void NegF32 (frac32 *dest, frac32 *src);
; void NegF60 (frac60 *dest, frac60 *src);
;
; return the twos complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction)
;

Neg64		PROC
NegF32		PROC
NegF60		PROC

		LDMIA	r1, {r2, r3}

		RSBS	r2, r2, #0
		RSC	r3, r3, #0

		STMIA	r0, {r2, r3}

		MOV	pc, lk		; RTS




;------------------------------------------------------


		END

