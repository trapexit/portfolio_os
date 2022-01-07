;------------------------------------------------------
;
; File:		sqrtf16.s
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
; ufrac16 SqrtF16 (ufrac16 x);
;
; Return the positive square root of an unsigned 16.16 number
;


SqrtF16		PROC

		MOV	r2, #0
		MOV	ip, #1

		CMP	r0, #1<<30
		SBCCS	r0, r0, #1<<30
		ADC	r2, r2, r2		; 1

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #28
		SBCCS	r0, r0, r1, LSL #28
		ADC	r2, r2, r2		; 2

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #26
		SBCCS	r0, r0, r1, LSL #26
		ADC	r2, r2, r2		; 3

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #24
		SBCCS	r0, r0, r1, LSL #24
		ADC	r2, r2, r2		; 4

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #22
		SBCCS	r0, r0, r1, LSL #22
		ADC	r2, r2, r2		; 5

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #20
		SBCCS	r0, r0, r1, LSL #20
		ADC	r2, r2, r2		; 6

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #18
		SBCCS	r0, r0, r1, LSL #18
		ADC	r2, r2, r2		; 7

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #16
 		SBCCS	r0, r0, r1, LSL #16
		ADC	r2, r2, r2		; 8

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #14
		SBCCS	r0, r0, r1, LSL #14
		ADC	r2, r2, r2		; 9

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #12
		SBCCS	r0, r0, r1, LSL #12
		ADC	r2, r2, r2		; 10

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #10
		SBCCS	r0, r0, r1, LSL #10
		ADC	r2, r2, r2		; 11

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #8
		SBCCS	r0, r0, r1, LSL #8
		ADC	r2, r2, r2		; 12

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #6
		SBCCS	r0, r0, r1, LSL #6
		ADC	r2, r2, r2		; 13

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #4
		SBCCS	r0, r0, r1, LSL #4
		ADC	r2, r2, r2		; 14

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1, LSL #2
		SBCCS	r0, r0, r1, LSL #2
		ADC	r2, r2, r2		; 15

		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; we have the first 16 bits

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; 17

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; 18

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; 19

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; 20

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; 21

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; 22

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		SBCCS	r0, r0, r1
		ADC	r2, r2, r2		; 23

		MOV	r0, r0, LSL #2
		ADD	r1, ip, r2, LSL #2
		CMP	r0, r1
		ADC	r0, r2, r2		; 24 bits of answer (at last!)

		MOV	pc, lk			; RTS


;------------------------------------------------------


		END

