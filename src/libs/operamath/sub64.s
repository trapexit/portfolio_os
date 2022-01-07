;------------------------------------------------------
;
; File:		sub64.s
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
; void Sub64 (int64 *r, int64 *s1, int64 *s2);
; void SubF32 (frac32 *r, frac32 *s1, frac32 *s2);
; void SubF60 (frac60 *r, frac60 *s1, frac60 *s2);
;
; Subtract two 64 bit integers and return the 64 bit result
;

Sub64		PROC
SubF32		PROC
SubF60		PROC

		LDMIA	r1, {r3, ip}	; get subtrahends
		LDMIA	r2, {r1, r2}


		SUBS	r2, ip, r2	; subtract
		SBC	r1, r3, r1

		STMIA	r0, {r1, r2}	; store the result

		MOV	pc, lk		; RTS


;------------------------------------------------------


		END

