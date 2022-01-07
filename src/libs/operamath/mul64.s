;------------------------------------------------------
;
; File:		mul64.s
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
; void Mul64 (int64 *p, int64 *m1, int64 *m2)
;
; Multiply one 64 bit integer by another and return a 64 bit result.
; Overflows are not detected.
;

Mul64		PROC

		STMFD	sp!, {r4-r11, lk}

		LDMIA	r1, {r3, r4}		; r3 = a01, (r4 = a23)
		LDMIA	r2, {r1, r2}		; (r1 = b01), r2 = b23

		MOV	r5, r3, LSR #16		; (r5 = a0)
		BIC	r5, r3, r5, LSL #16	; r5 = a1
		MOV	r6, r4, LSR #16		; r6 = a2
		BIC	r4, r4, r6, LSL #16	; r4 = a3
		MOV	r7, r1, LSR #16		; r7 = b0
		BIC	r1, r1, r7, LSL #16	; r1 = b1
		MOV	r8, r2, LSR #16		; r8 = b2
		BIC	r9, r2, r8, LSL #16	; r9 = b3

		MUL	ip, r4, r9		; ip = a3*b3
		MUL	r10, r4, r8		; r10 = a3*b2
		MUL	r11, r4, r1		; r11 = a3*b1
		ADDS	ip, ip, r10, LSL #16
		ADC	r11, r11, r10, LSR #16	; r11,ip = a3*b1 + a3*b2 + a3*b3
		MUL	r10, r6, r9		; r10 = a2*b3
		ADDS	ip, ip, r10, LSL #16
		ADC	r11, r11, r10, LSR #16	; r11,ip = a3*b1 + a3*b2+a2*b3 + a3*b3
		MLA	r11, r6, r8, r11	; r11,ip = a3b1+a2b2 + a3b2+a2b3 + a3b3
		MLA	r11, r3, r2, r11	; r11,ip = a1b2+a0b3 + a3b1+a2b2+a1b3 + a3b2...
		MUL	r10, r4, r7		; r10 = a3*b0
		ADD	r11, r11, r10, LSL #16	; r11,ip = a3b0+a1b2+a0b3 + a3b1+a2b2+a1b3...
		MUL	r10, r6, r1		; r10 = a2*b1
		ADD	r11, r11, r10, LSL #16	; r11,ip = a3b0+a2b1+a1b2+a0b3 + a3b1+a2b2...

		STMIA	r0, {r11, ip}		; save result

		LDMFD	sp!, {r4-r11, pc}	; RTS




;------------------------------------------------------


		END

