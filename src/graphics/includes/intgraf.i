; $Id: intgraf.i,v 1.5 1994/02/09 02:04:35 limes Exp $
;  *************************************************************************
; 
;  Internal Graphics Include File
; 
;  Copyright (C) 1992, New Technologies Group, Inc.
;  NTG Trade Secrets  -  Confidential and Proprietary
; 
;  The contents of this file were designed with tab stops of 4 in mind
; 
;  DATE   NAME             DESCRIPTION
;  ------ ---------------- -------------------------------------------------
;  920724 -RJ Mical        Start overhaul
;  920717 Stephen Landrum  Last edits before July handoff
; 
;  **********************************************************************   



	IF :LNOT::DEF:	|__INTGRAF_I|
	GBLL	|__INTGRAF_I|



	INCLUDE	nodes.i
	INCLUDE	folio.i
	INCLUDE	item.i
	INCLUDE	list.i
	INCLUDE	device.i
	INCLUDE	io.i


	INCLUDE	graphics.i

	INCLUDE	inthard.i


SC384EN		EQU	&00100000	; This should be defined in inthard.i


	BEGINSTRUCT	 CreateScreenArgs
		INT32	st_DisplayHeight
		INT32	st_ScreenCount
		INT32	st_ScreenHeight
		INT32	st_BitmapCount
		PTR	st_BitmapWidthArray
		PTR	st_BitmapHeightArray
		PTR	st_BitmapBufArray
		INT32	st_VDLType
		PTR	st_VDLPtrArray
		INT32	st_SPORTBankBits
		INT32	st_buffarrayallocatedflag
	ENDSTRUCT	


BLANKVDL_SIZE EQU 8	 ; number of words in the system VDL entry   
VDL_DMACTRLLAST EQU ((2:SHL:VDL_LEN_SHIFT)+0)

DISPLAY_WIDTH      EQU 320
DISPLAY_CLIPWIDTH  EQU 320
DISPLAY_HEIGHT     EQU 240

DISPLAY_RAMSIZE      EQU (DISPLAY_WIDTH*DISPLAY_HEIGHT*2)
;??? #define FB_OFFSET	(32*4)  



;  routine numbers for folio calls   
WRITEPIXEL       EQU -5
GETPIXELADDRESS  EQU -4
READVDLCOLOR     EQU -3
READPIXEL        EQU -2
MAPSPRITE        EQU -1



MINSPORTVCOUNT	EQU 10
MAXSPORTVCOUNT	EQU 16




PIXELSIZE   EQU 1
PIXELSHIFT  EQU 0

	ENDIF	  ; of #ifndef __INTGRAF_I

	END

