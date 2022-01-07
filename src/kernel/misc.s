	;; $Id: misc.s,v 1.3 1994/04/08 18:16:02 limes Exp $
;
;	misc.s
;
;	misc asm routine access routines
;	r9 contains ptr to KernelBase on entry
;
	AREA    |ASMCODE|, CODE, READONLY

	ALIGN	4		; alignment paranoia

	EXPORT	__rt_sdiv
	EXPORT	__rt_udiv
	EXPORT	__rt_sdiv10
	EXPORT	__rt_udiv10

;__rt_sdiv
;	ldr	r15,[r9,#-33*4]
;
;__rt_udiv
;	ldr	r15,[r9,#-34*4]
;
;__rt_sdiv10
;	ldr	r15,[r9,#-35*4]
;
;__rt_udiv10
;	ldr	r15,[r9,#-36*4]

	EXPORT	MiscFuncs
MiscFuncs	DCD	0
__rt_sdiv
	ldr	r12,MiscFuncs
	ldr	r15,[r12,#10*4]

__rt_udiv
	ldr	r12,MiscFuncs
	ldr	r15,[r12,#11*4]

__rt_sdiv10
	ldr	r12,MiscFuncs
	ldr	r15,[r12,#12*4]

__rt_udiv10
	ldr	r12,MiscFuncs
	ldr	r15,[r12,#13*4]

	END
