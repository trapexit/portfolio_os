;------------------------------------------------------
;
; File:		muluf16.s
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
; UFrac16 MulUF16 (frac16 m1, frac16 m2);
;
; Multiply two unsigned 16.16 integers together, and get a 16.16 result.
; Overflows are not detected.  Lower bits are truncated.
;

MulUF16		PROC

		MOV	r2, r0, LSR #16		; split multiplicands into 16 bit parts
		MOV	r3, r1, LSR #16
		BIC	ip, r0, r2, LSL #16
		BIC	r1, r1, r3, LSL #16

		MUL	ip, r1, ip		; ip = BD
		MOV	ip, ip, LSR #16		; ip = BD>>16
		MLA	r0, r3, r0, ip		; r0 = AC<<16 + BC + BD>>16
		MLA	r0, r2, r1, r0		; r0 = AC<<16 + AD+BC + BD>>16

		MOV	pc, lk			; RTS


;------------------------------------------------------


		END

