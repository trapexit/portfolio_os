*\  :ts=8 bk=0
*
* supergrafdo.s:	Supervisor mode SWI caller thingie.
*			Based on kernellib's SuperDo() routine (I just can't
*			leave anything alone...).
*
* Leo L. Schwab						9408.02
*
* $Id: supergrafdo.s,v 1.1 1994/08/25 23:03:12 ewhac Exp $
*/
		INCLUDE	structs.i
		INCLUDE	nodes.i
		INCLUDE	list.i
		INCLUDE	folio.i
		INCLUDE	kernel.i

****************************************************************************
* SuperGrafDo
*
* SYNOPSIS
*	SuperGrafDo (offset)
*		      r12
*
*	int32	offset;
*
		AREA	|Shaddup|, CODE, READONLY

		EXPORT	SuperGrafDo
SuperGrafDo	ROUT

		stmfd	sp!, {r9,lr}

		ldr	r9, adrGB	; Fetch GrafBase
		ldr	r9, [r9]

		ldrb	r14, [r9, #fl_MaxUserFunctions]
		add	r12, r12, r14
		add	r12, r12, #1		; Fencepost
		mov	lr, pc			; get return address
		ldr	pc, [r9, -r12, lsl #2]	; jmp to routine

		ldmfd	sp!, {r9,pc}		; RTS

	;------	Local copy of pointer pointer (gah).
		IMPORT	|GrafBase|
adrGB		DCD	|GrafBase|

		END
