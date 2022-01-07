	;; $Id: Video6.s,v 1.13 1994/09/19 22:04:28 vertex Exp $
;--------------------------------------------------
;
; File:			Video.a
; By:			Stephen H. Landrum
; Created:		24-Jun-91
;
; Copyright (c) 1991, New Technologies Group, Inc.
;
;--------------------------------------------------

	GET	registers.i
	GET	structs.i
	GET	macros.a

;--------------------------------------------------

	AREA	|ASMCODE|, CODE, READONLY
;--------------------------------------------------
; Hardware constants and other equates

	GET	inthard.i

;--------------------------------------------------

   IF	:DEF:|_CIO_GCIO|
	GET	Charset.a
   ENDIF

;--------------------------------------------------
; Code
; Do_FIRQ is only used during the cold start before the kernel
; has configured itself and knows how to deal with lists of interrupt
; handlers

	ALIGN	4		; alignment paranoia

	EXPORT	Do_FIRQ
Do_FIRQ	PROC

	MOV	R9,#CLIO	; address of clio
	mov	R8,#2		; V1 interrupt bit
	str	R8,[r9,#&44]	; clear the interrupt

	RTI

	END
