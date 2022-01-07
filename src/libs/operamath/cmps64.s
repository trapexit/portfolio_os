;------------------------------------------------------
;
; File:		cmps64.s
; By:		Stephen H. Landrum
; Last update:	10-Feb-93
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
; long CompareS64 (int64 *s1, int64 *s2);
; long CompareSF32 (frac32 *s1, frac32 *s2);
; long CompareSF60 (frac60 *s1, frac60 *s2);
;
; Subtract two signed 64 bit integers and return the high word of the result
; (or 1 if the high word is zero, and the low word is non-zero).
; Alternately compare two signed 32.32 fractions.
; The result of the comparison will be positive if s1>s2, zero if s1==s2,
; and negative if s1<s2.
;

CompareS64	PROC
CompareSF32	PROC
CompareSF60	PROC

		LDMIA	r0, {r2, r3}	; get subtrahends
		LDMIA	r1, {r0, r1}

		SUBS	r3, r3, r1	; subtract
		SBCS	r0, r2, r0

		MOVGE	r0, #1		; if greater, return 1
		MOVLT	r0, #-1		; if lesser, return -1
		TEQEQ	r3, #0
		MOVEQ	r0, #0		; if equal, return 0

		MOV	pc, lk		; RTS


;------------------------------------------------------


		END

