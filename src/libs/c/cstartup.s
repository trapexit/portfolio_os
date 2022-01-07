; $Id: cstartup.s,v 1.13 1994/11/09 23:58:39 stan Exp $
;	c initial startup code for tasks

	GET registers.i

	AREA	|ASMCODE|, CODE, READONLY

;	extended header for 3do binaries
;	filled in by 3do bin tool

	EXPORT	|__my_3DOBinHeader|
|__my_3DOBinHeader|

	EXPORT	|__my_AIFHeader|
|__my_AIFHeader|		EQU	(.-128)

	DCD	0,0,0,0,0,0,0,0
	DCD	0,0x00180000,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0

	EXPORT	|__main|
	IMPORT	main

COPYARGSTOSTACK	EQU	-(30*4)
STACKOVERFLOW	EQU	-(31*4)
EXITTASK	EQU	-(37*4)

	EXPORT |__main|
;	on entry r9 = |_KernelBase|
;	our stack, sl, r0, r1 have all been set up already.
;	At present there is no env passed in.
	ENTRY
|__main|
	mov	r0,r5
	mov	r1,r6
	mov	r9,r7

	ldr	r2,adrKB
	str	r9,[r2]

	mov	r8,lk		; save link register
	mov	lk,pc		; get return address
	ldr	pc,[r9,#COPYARGSTOSTACK]	; call kernel folio
	mov	lk,r8		; restore lk register
	bl	main

	EXPORT	exit
exit
	ldr	r9,adrKB
	ldr	r9,[r9]
	ldr	pc,[r9,#EXITTASK]	; call kernel folio
|__PointOfNoReturn|
	b	|__PointOfNoReturn|

adrKB	DCD	|_KernelBase|

;	dummy for stack overflow problems
	EXPORT |__rt_stkovf_split_small|
	EXPORT |__rt_stkovf_split_big|
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
	b  |__hangstack|

	AREA	|ASMdata|

	EXPORT	KernelBase
	EXPORT	_KernelBase
KernelBase
_KernelBase	DCD	0


	IMPORT  clib_version
clib_vers	DCD	clib_version

	END
