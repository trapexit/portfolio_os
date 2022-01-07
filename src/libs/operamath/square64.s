;------------------------------------------------------
;
; File:		square64.s
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
; void Square64 (uint64 *p, int64 *m)
;
; Square a 64 bit integer and return a 64 bit result.
; Overflows are not detected.
;

Square64	PROC

		STMFD	sp!, {r4, lk}

		LDMIA	r1, {r1, r2}		; r1 = a01, r2 = a23

		MOV	r3, r1, LSR #16		; (r3 = a0)
		BIC	r3, r1, r3, LSL #16	; r3 = a1
		MOV	r4, r2, LSR #16		; r4 = a2
		BIC	ip, r2, r4, LSL #16	; ip = a3

		MUL	r1, r2, r1		; r1 = a3a0+a2a1 + a3a1
		MUL	r2, r4, r4		; r2 = a2*a2
		ADD	r1, r2, r1, LSL #1	; r1 = a3a0+a2a1+a1a2+a0a3 + a3a1+a2a2+a1a3
		MUL	r2, ip, ip		; r2 = a3*a3
		MUL	r4, ip, r4		; r4 = a2*a3
		ADDS	r2, r2, r4, LSL #17
		ADC	r1, r1, r4, LSR #15	;r1r2 = 2*(a3a0+a2a1) + 2*a3a1+a2a2 + 2*a3a2 + a3a3

		STMIA	r0, {r1,r2}

		LDMFD	sp!, {r4, pc}	; RTS



;------------------------------------------------------


		END

