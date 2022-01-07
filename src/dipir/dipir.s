	;; $Id: dipir.s,v 1.27 1994/08/29 18:33:26 sdas Exp $

	GET	structs.i
	GET	registers.i

	AREA |ASMCODE|, CODE, READONLY

;	Dipir's stack is set up immediately after the end of dipir code.
stacksize	EQU	2*1024

	ENTRY

; We got here either from reset processing at power on
; or from the dipir interrupt.

;	lk pts back to interrupted code
;	interrupts are turned off
;	write dmas are turned off
;	memory at dipir_start... is secure
;	memory saved is 0x200-0xc000 (about 48k)


	IMPORT	_end
	IMPORT	dipir
	IMPORT	RSACheck
	IMPORT	GenRSACheck
	IMPORT	QueryROMSysInfo
	IMPORT	SetROMSysInfo
	IMPORT	SectorECC
	IMPORT	OpenRomFile
	IMPORT	ReadRomFile
	IMPORT	CloseRomFile
	IMPORT	GetDiscOsVersion

;	Dipir vector table starts at 0x200
;	The vector table provides a way for external callers (the kernel)
;	to get access to some useful routines in dipir.
;	The number of entries in this table is in slot 6 (don't ask why it's 6).
	b	dipir_start	;  0  200
	b	RSACheck	;  1  204
	b	GenRSACheck	;  2  208
	b	QueryROMSysInfo	;  3  20c
	b	SetROMSysInfo	;  4  210
	b	SectorECC	;  5  214
	DCD	11		;  6  218 = count of entries in table
	b	OpenRomFile	;  7  21c
	b	ReadRomFile	;  8  220
	b	CloseRomFile	;  9  224
	b	GetDiscOsVersion ; 10  228
;	Add additional ones here (and increment the count in slot 6).

dipir_start
	mov	r0,sp		; save the old stack ptr
	; Save the old abort stack ptr.
	; Note: we don't initialize the abort sp here because the abort
	; handler doesn't push anything on the stack; it just uses
	; sp (r13_abt) as a scratch register.
	mrs	r1,CPSR		; get current mode
	bic	r2,r1,#&0f
	orr	r2,r2,#&07
	msr	CPSR,r2		; switch to abort mode 
	nop
	str	sp,oldabort_sp	; save abort stack ptr (r13_abt)
	msr	CPSR,r1		; restore mode

	mov	r1,#stacksize	; Set up our own stack immediately after dipir
	ldr	r2,DipirCodeEnd
	add	r2,r1,r2
	str	r2,DipirEnd	; now DipirEnd points after the stack
	mov	sp,r2

;	initialize dipir stack for testing (r1 = stacksize)
	ldr	r2,StackInitValue	; constant value
	mov	r3,r2
	mov	r4,r2
	mov	r5,r2
initstack
	stmfd	sp!,{r2,r3,r4,r5}
	stmfd	sp!,{r2,r3,r4,r5}
	subs	r1,r1,#32
	bgt	initstack
	add	sp,sp,#stacksize	; restore stack to top

	stmdb	sp!,{r0,lk}	; save old-sp,lk on stack to get back
	mrs	r1,SPSR		; get old cpu mode
	stmdb	sp!,{r1}	; save on stack
	str	sp,my_stack	; incase of abort
	mov	r1,#&10
	ldr	r0,[r1]
	str	r0,olddata_abort	; save abort vector
	mov	r0,#&10
	adr	r1,dataabort
	bl	InstallArmVector


	bl	dipir
;	mov	r0,#0		; normal return

dipir_ret
	ldr	r2,olddata_abort ; restore data abort vector
	mov	r1,#&10
	str	r2,[r1]		

	mrs	r1,CPSR		; restore data abort stack ptr
	bic	r2,r1,#&0f
	orr	r2,r2,#&07
	msr	CPSR,r2		; switch to abort mode
	nop
	ldr	sp,oldabort_sp	; restore stack ptrt (r13_abt)
	msr	CPSR,r1		; restore mode

	ldmia	sp!,{r1}	; get old SPSR
	msr	SPSR_all,r1	; make sure we go back to same place
	ldmia	sp!,{sp,pc}^	; jump back
	nop

; NOTE:
; The SPSR needs to be saved in case we get an abort, which wipes
; out the SPSR.
StackInitValue	DCD &d111111d

	EXPORT	DipirEnd
DipirEnd	DCD 0
DipirCodeEnd	DCD _end


	nop
	nop
	EXPORT EnableIrq
EnableIrq
	mrs	r1,CPSR		; copy cpsr->r1
	bic	r1,r1,#&80	; clear interrupt bits first
	msr	CPSR_ctl,r1	; copy r1->cpsr_ctl
	mov	pc,r14

	EXPORT	ret_code
olddata_abort	DCD	0
oldabort_sp	DCD	0
ret_code	DCD	0
my_stack	DCD	0
	EXPORT	jmp_back
jmp_back	DCD	0	; data abort
	EXPORT	ignoreabort
ignoreabort	DCD	0
	EXPORT	abortignored
abortignored	DCD	0

	nop
	nop
	IMPORT	longjmp
	EXPORT	dataabort
dataabort
	; Note: can't push onto stack here because r13_abt has not 
	; been initialized.  Just use r13_abt as a scratch register.
	ldr	r13,ignoreabort		
	movs	r13,r13			; should we just ignore this abort?
	beq	notignoreabort
	str	r13,abortignored	; yup, set flag
	subs	pc,r14,#8		; return to aborted instruction

;	read abort reason
notignoreabort
	mov	r0,#&3300000
	orr	r0,r0,#&0000020
	ldr	r1,[r0]		; read abort register
	mov	r2,#0
	str	r2,[r0]	; clear abort
	; go back to super mode
	; and start again
	mrs	r0,CPSR
	bic	r0,r0,#7
	orr	r0,r0,#3
	msr	CPSR_ctl,r0	; back to supervisor mode
	nop
	ldr	r0,jmp_back
	mov	r1,#1
	b	longjmp

	ldr	sp,my_stack	; get back return stack
	mov	r0,#1		; tell operator to try again
	b	dipir_ret


	EXPORT InstallArmVector
InstallArmVector
;	routine to modify lowlevel arm vector table
;	to branch to code
;	enter with r0 = vector
;	           r1 = code
	sub	r2,r1,r0	; offset by vector #
	sub	r2,r2,#8	; and 8 for branch prefetch
	mov	r2,r2,LSR #2	; shift it down by 2
	add	r2,r2,#&ea000000 ; make a branch out of it
	str	r2,[r0]
	mov	pc,r14		; return

;	to get around the god damn casting problem in longjmp()
	EXPORT	myfucking_longjmp
	IMPORT	longjmp
myfucking_longjmp
	b	longjmp

	EXPORT	CallAIF
CallAIF	
	mov	r6,r1		; transfer env ptr
	stmfd	sp!,{r7,lk}	; pass r7 in as return
	mov	r7,pc		; get return address
	mov	pc,r0		; jump to remote code
	ldmia	sp!,{r7,pc}	; return to caller


	EXPORT	callaif2
	; callaif(arga,argb,funcp);
callaif2
	stmfd	sp!,{r4-r11,r14}
	mov	r5,r0
	mov	r6,r1
	mov	r7,r9
	mov	r8,r15		;get return address
	mov	r15,r2		;jump to routine
	ldmfd	sp!,{r4-r11,r15}

	EXPORT	RealBoot
RealBoot
;takes	r0 as function ptr
;takes	r1-r3 -> r6->r8
	stmfd	sp!,{r6-r8,r14}
	mov	r6,r1
	mov	r7,r2
	mov	r8,r3
	mov	lk,pc
	mov	pc,r0
	ldmfd	sp!,{r6-r8,pc}

	EXPORT	discOsVersion
discOsVersion	DCD	&FFFFFFFF

	END

