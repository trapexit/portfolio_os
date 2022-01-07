	;; $Id: allocvectors.s,v 1.3 1994/08/05 20:54:28 vertex Exp $
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

	AREA	|MEMDEBUGKernelBase|, DATA, READONLY

adrKB	DCD	KernelBase

	AREA	|MEMDEBUGStubs|, CODE, READONLY

	EXPORT	KernelAllocMemFromMemLists
KernelAllocMemFromMemLists
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#ALLOCMEM<<2]
	ldmfd	sp!,{r9,r15}	; return

	EXPORT	KernelFreeMemToMemLists
KernelFreeMemToMemLists
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#FREEMEM<<2]
	ldmfd	sp!,{r9,r15}	; return

;---------------------------------
;
; These are supervisor-mode vectors, used when memdebug.lib functions are
; called in supervisor mode. We need these in here in order to prevent
; client code from requiring kernel.lib whenever they use memdebug.lib.
; Since we don't give kernel.lib to our 3rd party developers, this would be
; a problem...

	EXPORT	Kernelkprintf
Kernelkprintf
	mov	r12,#14
	b	SuperDo

	EXPORT	KernelLockSemaphore
KernelLockSemaphore
	mov	r12,#7
	b	SuperDo

	EXPORT	KernelUnlockSemaphore
KernelUnlockSemaphore
	mov	r12,#6
;	b	SuperDo

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
