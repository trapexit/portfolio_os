	;; $Id: allocvectors.s,v 1.2 1995/02/25 00:01:04 vertex Exp $
	GET	structs.i
	GET	nodes.i
	GET	list.i
	GET	folio.i
	GET	kernel.i

ALLOCMEM	EQU	-7
FREEMEM		EQU	-8

	MACRO
$label	GETKERNELBASE
	ldr	r9,adrKB
	ldr	r9,[r9]
	MEND

	IMPORT	KernelBase


	AREA	|STUBKernelBase|, CODE, READONLY

adrKB	DCD	KernelBase

	AREA	|STUBAllocMemFromMemLists|, CODE, READONLY

	EXPORT	AllocMemFromMemLists
AllocMemFromMemLists
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#ALLOCMEM<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBFreeMemToMemLists|, CODE, READONLY

	EXPORT	FreeMemToMemLists
FreeMemToMemLists
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#FREEMEM<<2]
	ldmfd	sp!,{r9,r15}	; return


	END
