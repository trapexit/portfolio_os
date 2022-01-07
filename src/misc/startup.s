	;; $Id: startup.s,v 1.2 1994/02/09 01:27:17 limes Exp $

	GET	structs.i
	GET	nodes.i
	GET	list.i

	AREA	|ASMCODE|, CODE, READONLY
	EXPORT	self
self
	; space for 3DOheader
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	ENTRY
	EXPORT	|__main|
|__main|
	IMPORT	main
	bl	main

	mov	r15,r8	; return

	END
