

;*****

; $Id: interrupts.i,v 1.8 1994/09/10 01:43:35 vertex Exp $

; $Log: interrupts.i,v $
;Revision 1.8  1994/09/10  01:43:35  vertex
;Prepared for automatic labelling and release numbering.
;
;Revision 1.7  1994/01/28  23:20:16  sdas
;Include Files Self-consistency - includes dependant include files
;
;Revision 1.6  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.5  1992/10/24  01:37:21  dale
;fix id
;

;*****/
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

 IF	:DEF:|_INTERRUPTS_I|
 ELSE

	GBLL	|_INTERRUPTS_I|

	INCLUDE	structs.i
	INCLUDE nodes.i
	INCLUDE item.i

	BEGINSTRUCT	Firq
		STRUCT	ItemNode,firq_firq
		STRUCT	PTR,firq_f
		STRUCT	PTR,firq_Data
		STRUCT	PTR,firq_num
	ENDSTRUCT

CREATEFIRQ_DATA		EQU 1+TAG_ITEM_LAST
CREATEFIRQ_CODE		EQU 1+CREATEFIRQ_DATA
CREATEFIRQ_FLAGS	EQU 1+CREATEFIRQ_CODE
CREATEFIRQ_NUM		EQU 1+CREATEFIRQ_FLAGS ; which interrupt slot

 ENDIF	; |_INTERRUPTS_I|

	END
