;------------------------------------------------------
;
; File:		divuf16.s
; By:		Stephen H. Landrum
; Last update:	10-Feb-92
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


; typedef struct int64 { ulong hi, ulong lo } int64, uint64, frac32, ufrac32;
; typedef long frac16, int32;
; typedef ulong ufrac16, uint32;




;------------------------------------------------------
;
; ufrac16 DivUF16 (ufrac16 *rem, ufrac16 d1, ufrac16 d2);
;
; Divide an unsigned 16.16 fraction into another and return an unsigned
; 16.16 result.  The remainder will be returned in the location pointed
; to by the first argument (and in r1 as well).  Overflow is signaled by
; all bits set in the return values.  Note that the remainder is not
; really a 16.16 number, but is actually a 0.32 fraction.
;


DivRemUF16	PROC

		STMFD	sp!, {r0, lk}

		MOV	r0, r1
		MOV	r1, r2

		BL	DivUF16

		LDMFD	sp!, {r2, lk}
		STR	r1, [r2]
		MOV	pc, lk			; RTS


;------------------------------------------------------
;
; ufrac16 RecipUF16 (ufrac16 d);
;
; Take the reciprocal of an unsigned 16.16 number and return the 16.16
; result.  The remainder will be returned in r1.
; Overflow is signaled by all bits set in the return values.
;

RecipUF16	PROC

		MOV	r1, r0
		MOV	r0, #&00010000
;		B	DivUF16


;------------------------------------------------------
;
; ufrac16 DivUF16 (ufrac16 d1, ufrac16 d2);
;

DivUF16		PROC

		MOV	r3, #0
		CMP	r1, r0			; if ratio < 1, accelerate answer
		BHI	%01
		CMP	r1, r0, LSR #8		; if ratio < 256, accelerate answer
		BHI	%02

		CMP	r1, r0, LSR #16		; check for overflow
		BLS	%00

		RSBS	ip, r1, r0, LSR #15
		SBCCS	r0, r0, r1, LSL #15
		ADC	r3, r3, r3		; 1
		RSBS	ip, r1, r0, LSR #14
		SBCCS	r0, r0, r1, LSL #14
		ADC	r3, r3, r3		; 2
		RSBS	ip, r1, r0, LSR #13
		SBCCS	r0, r0, r1, LSL #13
		ADC	r3, r3, r3		; 3
		RSBS	ip, r1, r0, LSR #12
		SBCCS	r0, r0, r1, LSL #12
		ADC	r3, r3, r3		; 4
		RSBS	ip, r1, r0, LSR #11
		SBCCS	r0, r0, r1, LSL #11
		ADC	r3, r3, r3		; 5
		RSBS	ip, r1, r0, LSR #10
		SBCCS	r0, r0, r1, LSL #10
		ADC	r3, r3, r3		; 6
		RSBS	ip, r1, r0, LSR #9
		SBCCS	r0, r0, r1, LSL #9
		ADC	r3, r3, r3		; 7
		RSBS	ip, r1, r0, LSR #8
		SBCCS	r0, r0, r1, LSL #8
		ADC	r3, r3, r3		; 8
02		RSBS	ip, r1, r0, LSR #7
		SBCCS	r0, r0, r1, LSL #7
		ADC	r3, r3, r3		; 9
		RSBS	ip, r1, r0, LSR #6
		SBCCS	r0, r0, r1, LSL #6
		ADC	r3, r3, r3		; 10
		RSBS	ip, r1, r0, LSR #5
		SBCCS	r0, r0, r1, LSL #5
		ADC	r3, r3, r3		; 11
		RSBS	ip, r1, r0, LSR #4
		SBCCS	r0, r0, r1, LSL #4
		ADC	r3, r3, r3		; 12
		RSBS	ip, r1, r0, LSR #3
		SBCCS	r0, r0, r1, LSL #3
		ADC	r3, r3, r3		; 13
		RSBS	ip, r1, r0, LSR #2
		SBCCS	r0, r0, r1, LSL #2
		ADC	r3, r3, r3		; 14
		RSBS	ip, r1, r0, LSR #1
		SBCCS	r0, r0, r1, LSL #1
		ADC	r3, r3, r3		; 15
		RSBS	ip, r1, r0
		SBCCS	r0, r0, r1
		ADC	r3, r3, r3		; we have 16 bits of answer

01		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 17
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 18
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 19
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 20
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 21
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 22
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 23
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 24
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 25
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 26
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 27
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 28
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 29
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 30
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1
		ADC	r3, r3, r3		; 31
		ADDS	r0, r0, r0
		CMPCC	r0, r1
		SUBCS	r0, r0, r1

		MOV	r1, r0
		ADC	r0, r3, r3		; 32 bits of answer (finally!)

		MOV	pc, lk			; RTS

00		MOV	r0, #-1			; return an overflow condition
		MOV	r1, #-1
		MOV	pc, lk			; RTS


;------------------------------------------------------


		END

