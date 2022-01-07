;------------------------------------------------------
;
; File:		divu32.s
; By:		Stephen H. Landrum
; Last update:	3-Feb-93
;
; Math routines for the Opera
;
; Copyright (c) 1992,1993, The 3DO Company, Inc.
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




 IF {FALSE}
;------------------------------------------------------
;
; uint32 DivU32 (uint32 d1, uint32 d2)
;
; Divide a 32 bit integer d2 by 32 bit integer d1 and return a 32 bit quotient.
; The remainder is also returned in r1.  This routine takes arguments and returns
; values in the same way that the compiler expects the internally supplied
; library routines to.
;

DivU32		PROC

		RSBS	r2, r0, r1, LSR #31
		SBCCS	r1, r1, r0, LSL #31
		ADC	r3, r3, r3		; 1
		RSBS	r2, r0, r1, LSR #30
		SBCCS	r1, r1, r0, LSL #30
		ADC	r3, r3, r3		; 2
		RSBS	r2, r0, r1, LSR #29
		SBCCS	r1, r1, r0, LSL #29
		ADC	r3, r3, r3		; 3
		RSBS	r2, r0, r1, LSR #28
		SBCCS	r1, r1, r0, LSL #28
		ADC	r3, r3, r3		; 4
		RSBS	r2, r0, r1, LSR #27
		SBCCS	r1, r1, r0, LSL #27
		ADC	r3, r3, r3		; 5
		RSBS	r2, r0, r1, LSR #26
		SBCCS	r1, r1, r0, LSL #26
		ADC	r3, r3, r3		; 6
		RSBS	r2, r0, r1, LSR #25
		SBCCS	r1, r1, r0, LSL #25
		ADC	r3, r3, r3		; 7
		RSBS	r2, r0, r1, LSR #24
		SBCCS	r1, r1, r0, LSL #24
		ADC	r3, r3, r3		; 8
		RSBS	r2, r0, r1, LSR #23
		SBCCS	r1, r1, r0, LSL #23
		ADC	r3, r3, r3		; 9
		RSBS	r2, r0, r1, LSR #22
		SBCCS	r1, r1, r0, LSL #22
		ADC	r3, r3, r3		; 10
		RSBS	r2, r0, r1, LSR #21
		SBCCS	r1, r1, r0, LSL #21
		ADC	r3, r3, r3		; 11
		RSBS	r2, r0, r1, LSR #20
		SBCCS	r1, r1, r0, LSL #20
		ADC	r3, r3, r3		; 12
		RSBS	r2, r0, r1, LSR #19
		SBCCS	r1, r1, r0, LSL #19
		ADC	r3, r3, r3		; 13
		RSBS	r2, r0, r1, LSR #18
		SBCCS	r1, r1, r0, LSL #18
		ADC	r3, r3, r3		; 14
		RSBS	r2, r0, r1, LSR #17
		SBCCS	r1, r1, r0, LSL #17
		ADC	r3, r3, r3		; 15
		RSBS	r2, r0, r1, LSR #16
		SBCCS	r1, r1, r0, LSL #16
		ADC	r3, r3, r3		; 16
		RSBS	r2, r0, r1, LSR #15
		SBCCS	r1, r1, r0, LSL #15
		ADC	r3, r3, r3		; 17
		RSBS	r2, r0, r1, LSR #14
		SBCCS	r1, r1, r0, LSL #14
		ADC	r3, r3, r3		; 18
		RSBS	r2, r0, r1, LSR #13
		SBCCS	r1, r1, r0, LSL #13
		ADC	r3, r3, r3		; 19
		RSBS	r2, r0, r1, LSR #12
		SBCCS	r1, r1, r0, LSL #12
		ADC	r3, r3, r3		; 20
		RSBS	r2, r0, r1, LSR #11
		SBCCS	r1, r1, r0, LSL #11
		ADC	r3, r3, r3		; 21
		RSBS	r2, r0, r1, LSR #10
		SBCCS	r1, r1, r0, LSL #10
		ADC	r3, r3, r3		; 22
		RSBS	r2, r0, r1, LSR #9
		SBCCS	r1, r1, r0, LSL #9
		ADC	r3, r3, r3		; 23
		RSBS	r2, r0, r1, LSR #8
		SBCCS	r1, r1, r0, LSL #8
		ADC	r3, r3, r3		; 24
		RSBS	r2, r0, r1, LSR #7
		SBCCS	r1, r1, r0, LSL #7
		ADC	r3, r3, r3		; 25
		RSBS	r2, r0, r1, LSR #6
		SBCCS	r1, r1, r0, LSL #6
		ADC	r3, r3, r3		; 26
		RSBS	r2, r0, r1, LSR #5
		SBCCS	r1, r1, r0, LSL #5
		ADC	r3, r3, r3		; 27
		RSBS	r2, r0, r1, LSR #4
		SBCCS	r1, r1, r0, LSL #4
		ADC	r3, r3, r3		; 28
		RSBS	r2, r0, r1, LSR #3
		SBCCS	r1, r1, r0, LSL #3
		ADC	r3, r3, r3		; 29
		RSBS	r2, r0, r1, LSR #2
		SBCCS	r1, r1, r0, LSL #2
		ADC	r3, r3, r3		; 30
		RSBS	r2, r0, r1, LSR #1
		SBCCS	r1, r1, r0, LSL #1
		ADC	r3, r3, r3		; 31
		RSBS	r2, r0, r1
		SBCCS	r1, r1, r0
		ADC	r0, r3, r3		; 32 bits of result (Whew!)

		MOV	pc, lk

 ELSE
		IMPORT	__rt_udiv
 ENDIF

;------------------------------------------------------
;
; int32 DivRemU32 (uint32 *r, uint32 *d1, uint32 *d2)
;
; Calculate the quotient and remainder from a 32 bit integer division.
;

DivRemU32	PROC

		STMFD	sp!, {r0, lk}
		MOV	r0, r2
		BL	__rt_udiv
		LDMFD	sp!, {r2}
		STR	r1, [r2]

		LDMFD	sp!, {pc}



;------------------------------------------------------


		END

