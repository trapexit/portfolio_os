	;; $Id: cddipir.s,v 1.7 1994/07/19 22:10:00 markn Exp $

	GET	structs.i
	GET	registers.i


	AREA |ASMCODE|, CODE, READONLY


	; The 3DOBinHeader
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0

	; Space where the driver vector table would be if this were a driver.
	DCD	0
	DCD	0
	DCD 	0
	DCD 	0

	DCD	&6475636b	; Magic number ("duck")
	DCD	&00010002	; Version
;	Type 0001, Version XXXX	Normal 
;	Type 0002, Version XXXX Data disc
;	Type 0007, Version XXXX Special disc


	DCD	&d56a972d
	DCD	&a4731669, &77b6cc11, &71cadd19
	DCD	&ce6eca62, &5ba63be9, &1775bf73
	DCD	&8488590f, &ca0645e6, &48ca6bd2
	DCD	&66ec25fa, &dd8212a7, &871661ac

	ENTRY
	IMPORT	cddipir

;	We got here from call from rom dipir code (CallCDROMDipir in cdipir.c)

	stmfd	sp!,{r7}	; save return value on stack, yet I mean rseven
	mov	r0,r6		; get env ptr
	bl	cddipir		; call c code
	ldmfd	sp!,{r15}	; return to rom based dipir code

	END

