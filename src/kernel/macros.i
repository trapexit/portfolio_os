; $Id: macros.i,v 1.2 1994/02/09 02:04:35 limes Exp $
;--------------------------------------------------
;
; File:		Macros.i
; By:		Stephen H. Landrum
; Last update:	6-Feb-92
;
; Copyright (c) 1992, New Technologies Group, Inc.
;
; This document is proprietary and confidential
;
;--------------------------------------------------

 IF :DEF:|__macros_i|
 ELSE
	GBLL	|__macros_i|

; Macros

		MACRO
$label		WORD	$value
$label		DCD	$value
		MEND


		MACRO
$label		NOP
$label		MOV	R0, R0
		MEND
		

		MACRO
$label		RTS
$label		MOV	PC, LK
		MEND
		

		MACRO
$label		RTI
$label		SUBS	PC, LK, #4
		MEND
		

		MACRO
$label		POPRTS
$label		LDMFD	SP!, {PC}
		MEND
		

		MACRO
$label		PROC
		EXPORT	$label
		ALIGN
$label		ROUT
		MEND


 ENDIF

		END

