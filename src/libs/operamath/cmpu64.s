;------------------------------------------------------
;
; File:		cmpu64.s
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
; long CompareU64 (uint64 *s1, uint64 *s2);
; long CompareUF32 (ufrac32 *s1, ufrac32 *s2);
; long CompareUF60 (ufrac60 *s1, ufrac60 *s2);
;
; Subtract two unsigned 64 bit integers and return 1 if s1>s2, 0 if s1==s2,
; and -1 if s1<s2.
; Alternately compare two unsigned 32.32 fractions
;

CompareU64	PROC
CompareUF32	PROC
CompareUF60	PROC

		LDMIA	r0, {r2, r3}	; get subtrahends
		LDMIA	r1, {r0, r1}

		SUBS	r1, r3, r1	; subtract
		SBCS	r2, r2, r0

		ORRS	r0, r1, r2	; if equal, return zero
		MOVCC	r0, #&ffffffff	; if carry clear, result is lower
		MOVHI	r0, #1		; if carry set and not equal, result is higher

		MOV	pc, lk		; RTS


;------------------------------------------------------


		END

