; $Id: threadstartup.s,v 1.5 1994/11/11 03:35:38 stan Exp $
; Startup code for disk-loaded threads and disk-loaded subroutines

	GET registers.i
	AREA	|ASMCODE|, CODE, READONLY

;-----------------------------------------------------------------------------

	EXPORT |__main|
	EXPORT |_KernelBase|
	EXPORT |__rt_stkovf_split_small|
	EXPORT |__rt_stkovf_split_big|

;-----------------------------------------------------------------------------

	IMPORT	main

;-----------------------------------------------------------------------------

COPYARGSTOSTACK	EQU -(30*4)
STACKOVERFLOW	EQU -(31*4)
EXITTASK	EQU -(37*4)

;-----------------------------------------------------------------------------

	EXPORT	|__my_3DOBinHeader|
|__my_3DOBinHeader|

	EXPORT	|__my_AIFHeader|
|__my_AIFHeader|		EQU	(.-128)

	; _3DOBinHeader structure, filled in by modbin
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0x00180000,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0

;-----------------------------------------------------------------------------

; Beginning of program code. On entry:
;
;	r5   has the value of the tag CREATETASK_TAG_ARGC
;	r6   has the value of the tag CREATETASK_TAG_ARGP
;	r7   has the value of the tag CREATETASK_TAG_BASE (KernelBase)
;
	ENTRY
|__main|
	mov	r0,r5		; get ARGC
	mov	r1,r6		; get ARGP
	mov	r9,r7		; get BASE (KernelBase)

	ldr	r2,adrKB	; get address of pointer
	str	r9,[r2]		; stash KernelBase

	bl	main		; call the main code

	EXPORT	exit
exit
	ldr	r9,adrKB
	ldr	r9,[r9]
	ldr	pc,[r9,#EXITTASK]	; call kernel folio
|__PointOfNoReturn|
	b	|__PointOfNoReturn|


adrKB  DCD |_KernelBase|

;-----------------------------------------------------------------------------

;	dummy for stack overflow problems
	nop
	nop
	nop

|__rt_stkovf_split_small|
	mov	ip,sp
|__rt_stkovf_split_big|
;	b |__rt_stkovf_split_big|

	sub	sp,sp,#4	; make space for new pc
	stmfd	sp!,{r0}
	ldr	r0,adrKB
	ldr	r0,[r0]
	ldr	r0,[r0,#STACKOVERFLOW]	; get ptr to stack overflow kernel function
	str	r0,[sp,#4]		; store it on stack
	ldmfd	sp!,{r0,pc}		; jump to it, restoring r0

|__hangstack|
	b       |__hangstack|

;-----------------------------------------------------------------------------

	AREA	|ASMdata|

	EXPORT	KernelBase
	EXPORT	_KernelBase
KernelBase
_KernelBase	DCD	0


	IMPORT  clib_version
clib_vers	DCD	clib_version

;-----------------------------------------------------------------------------

	END
