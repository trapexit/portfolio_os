;------------------------------------------------------
;
; File:		sqrt64_32.s
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
; uint32 Sqrt64_32 (int64 *x);
; ufrac16 sqrtf32_f16 (frac32 *x);
; ufrac30 sqrtf60_f30 (frac60 *x);
;
; Return the positive square root of an unsigned 32 bit number
;


Sqrt64_32	PROC
SqrtF32_F16	PROC
SqrtF60_F30	PROC

		LDMIA	r0, {r2,r3}

		MOV	r0, #0
		MOV	ip, #1

		CMP	r2, #1<<30
		SBCCS	r2, r2, #1<<30
		ADC	r0, r0, r0		; 1

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #28
		SBCCS	r2, r2, r1, LSL #28
		ADC	r0, r0, r0		; 2

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #26
		SBCCS	r2, r2, r1, LSL #26
		ADC	r0, r0, r0		; 3

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #24
		SBCCS	r2, r2, r1, LSL #24
		ADC	r0, r0, r0		; 4

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #22
		SBCCS	r2, r2, r1, LSL #22
		ADC	r0, r0, r0		; 5

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #20
		SBCCS	r2, r2, r1, LSL #20
		ADC	r0, r0, r0		; 6

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #18
		SBCCS	r2, r2, r1, LSL #18
		ADC	r0, r0, r0		; 7

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #16
 		SBCCS	r2, r2, r1, LSL #16
		ADC	r0, r0, r0		; 8

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #14
		SBCCS	r2, r2, r1, LSL #14
		ADC	r0, r0, r0		; 9

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #12
		SBCCS	r2, r2, r1, LSL #12
		ADC	r0, r0, r0		; 10

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #10
		SBCCS	r2, r2, r1, LSL #10
		ADC	r0, r0, r0		; 11

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #8
		SBCCS	r2, r2, r1, LSL #8
		ADC	r0, r0, r0		; 12

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #6
		SBCCS	r2, r2, r1, LSL #6
		ADC	r0, r0, r0		; 13

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #4
		SBCCS	r2, r2, r1, LSL #4
		ADC	r0, r0, r0		; 14

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1, LSL #2
		SBCCS	r2, r2, r1, LSL #2
		ADC	r0, r0, r0		; 15

		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; we have first 16 bits of square root

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 17

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 18

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 19

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 20

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 21

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 22

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 23

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 24

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 25

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 26

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 27

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 28

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 29

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2
		ADD	r1, ip, r0, LSL #2
		CMP	r2, r1
		SBCCS	r2, r2, r1
		ADC	r0, r0, r0		; 30

		CMP	r3, #1<<30
		SBCS	r1, r2, r0
		MOVCS	r2, r1
		SUBCS	r3, r3, #1<<30
		ADC	r0, r0, r0		; 31

		MOV	r2, r2, LSL #2
		ORR	r2, r2, r3, LSR #30
		MOV	r3, r3, LSL #2

		CMP	r3, #1<<30
		SBCS	r1, r2, r0
		ADC	r0, r0, r0		; 32 bits (at last!)

		MOV	pc, lk			; RTS


;------------------------------------------------------


		END

