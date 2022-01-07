	;; $Id: superdo.s,v 1.4 1994/02/09 01:27:17 limes Exp $
	GET	structs.i
	GET	nodes.i
	GET	list.i
	GET	folio.i
	GET	kernel.i

	AREA	|ASMCODE|, CODE, READONLY

	MACRO
$label	GETKERNELBASE
	ldr	r9,adrKB
	ldr	r9,[r9]
	MEND

	IMPORT	|_KernelBase|

adrKB	DCD	|_KernelBase|

	EXPORT	SuperDo
SuperDo
	stmfd	sp!,{r9,r14}
	GETKERNELBASE	; returns it in r9
	ldrb	r14,[r9,#fl_MaxUserFunctions]
	add	r12,r12,r14
	add	r12,r12,#1
	mov	r14,r15		; get return address
	ldr	r15,[r9,-r12,lsl #2]	;jmp to routine
	ldmfd	sp!,{r9,r15}

	END
