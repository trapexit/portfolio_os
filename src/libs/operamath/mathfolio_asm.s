	;; $Id: mathfolio_asm.s,v 1.3 1994/02/09 01:27:17 limes Exp $
;------------------------------------------------------
;
; File:		mathfolio.s
; By:		Stephen H. Landrum
; Last update:	26-Mar-93
;
; Math folio routines for the Opera
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

getPSR		PROC

		mrs	r0, CPSR
		mov	pc, lk		; RTS

;------------------------------------------------------

		END
