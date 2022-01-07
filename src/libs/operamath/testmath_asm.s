;------------------------------------------------------
;
; File:		testmath.s
; By:		Stephen H. Landrum
; Last update:	7-Apr-93
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

		INCLUDE	operamath.i


;------------------------------------------------------

		AREA	ASMCODE2, CODE

;------------------------------------------------------

aRecipUF16	PROC

		STMFD	sp!, {lk}
		LIBCALL	RecipUF16
		LDMFD	sp!, {pc}

;------------------------------------------------------

aMulVec3Mat33_F16	PROC

		STMFD	sp!, {lk}
		LIBCALL MulVec3Mat33_F16,AL
		LDMFD	sp!, {pc}

;------------------------------------------------------

		END


