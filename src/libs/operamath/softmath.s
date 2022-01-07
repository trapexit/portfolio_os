;------------------------------------------------------
;
; File:		softmath.s
; By:		Stephen H. Landrum
; Last update:	27-Mar-93
;
; Math routines for the Opera
;
; User mode routines
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
; void Transpose33_F16 (mat33f16 dest, mat33f16 src);
;
; /* Return the transpose of a 3x3 matrix of 16.16 values */
;

Transpose33_F16	PROC

		STMFD	sp!, {r4-r8, lk}

		LDMIA	r1, {r1-r8,ip}
		STMIA	r0!, {r1,r4,r7}
		STMIA	r0!, {r2,r5,r8}
		STMIA	r0!, {r3,r6,ip}

		LDMFD	sp!, {r4-r8, pc}	; RTS


;------------------------------------------------------
;
; void Transpose44_F16 (mat44f16 dest, mat44f16 src);
;
; /* Return the transpose of a 4x4 matrix of 16.16 values */
;

Transpose44_F16	PROC

		STMFD	sp!, {r4-r8, lk}

		LDMIA	r1!, {r2-r8,ip}
		STMIA	r0!, {r2,r6}
		ADD	r0, r0, #8
		STMIA	r0!, {r3,r7}
		ADD	r0, r0, #8
		STMIA	r0!, {r4,r8}
		ADD	r0, r0, #8
		STMIA	r0!, {r5,ip}
		ADD	r0, r0, #-48
		LDMIA	r1!, {r2-r8,ip}
		STMIA	r0!, {r2,r6}
		ADD	r0, r0, #8
		STMIA	r0!, {r3,r7}
		ADD	r0, r0, #8
		STMIA	r0!, {r4,r8}
		ADD	r0, r0, #8
		STMIA	r0!, {r5,ip}

		LDMFD	sp!, {r4-r8, pc}	; RTS


;------------------------------------------------------


		END
