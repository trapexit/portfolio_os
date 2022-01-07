; $Id: startup.s,v 1.59.1.1 1994/12/13 04:51:18 markn Exp $

	GET	structs.i
	GET	nodes.i
	GET	list.i
	GET	folio.i
	GET	kernel.i
	GET	task.i
	GET	interrupts.i
	GET	kernelmacros.i
	GET	device.i
	GET	io.i
	GET	kernelnodes.i
	GET	mem.i
	GET	semaphore.i
	GET	time.i
	GET	timer.i
	GET	item.i
	GET	msgport.i

;SUPERSTACKSIZE	*	4096

ARM_FIRQVec	EQU	&01c
ARM_SWIVec	EQU	&08
ARM_PREABTVec	EQU	&0c
ARM_DATABTVec	EQU	&10
ARM_UNDINSVec	EQU	&04
ARM_IRQVec	EQU	&18

MADAM		EQU	&03300000
CLIO		EQU	&03400000

;	args to the kernel
r6_ROM		EQU	&100
r6_DEBUG	EQU	&200
r6_ROMOVERCD	EQU	&400
r6_BOOTDATA	EQU	&001

;	EXPORT	|ptr_firq|
;	EXPORT	SuperStackBottom
	EXPORT	SuperStack
	EXPORT	SuperLimit
	EXPORT |__rt_stkovf_split_small|
	EXPORT |__rt_stkovf_split_big|
	IMPORT	main
	EXPORT |__main|
	EXPORT	KernelBase
	EXPORT	KernelItem
;	EXPORT	|_exit|
	EXPORT	FirqHandler

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	|__my_AIFHeader|
|__my_AIFHeader|		EQU	(.-128)

	EXPORT	|__my_3DOBinHeader|
|__my_3DOBinHeader|

	; space for 3DOheader
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0

KernelBase	DCD	0	; romsysinfo depends on KernelBase
				;  being defined here

	ALIGN	4		; alignment paranoia

	ENTRY
|__main|
; on entry all nonuser stacks have been set up the the
; launch rom. These will have been set to the end of
; the memory space.
; Also interrupts are disabled.

;	if launched from rom, r6= 0x100
;	if launched from rom in a ROM-over-CD environment, r6= 0x400
;	otherwise launched from debugger
;	but if we set r6 to 0x200, then
;	pretend we were launched from rom,
;	but still talk to the debugger.

 [	ROMBUILD=1			; we will gather CPUFlags in r0.
	mov	r0,#KB_ROMAPP		; this is ROM app kernel
 |
	mov	r0,#0			; this is not ROM app kernel
 ]
	mov	r5,r6			; save r6 in r5
	and	r6,r6,#:NOT:r6_BOOTDATA	; clear the BOOTDATA bit in r6
	cmp	r6,#r6_ROM		; if really from rom,
	orreq	r0,r0,#KB_NODBGR	; turn on the "no debugger" bit.
	cmp	r6,#r6_ROMOVERCD	; if really from rom but ROMOverCD,
	orreq   r0,r0,#(KB_NODBGR+KB_ROMOVERCD) ; turn on the "no debugger" bit.
	cmp	r6,#r6_DEBUG		; if really from debug but faking rom,
	orreq	r0,r0,#(KB_NODBGR+KB_NODBGRPOOF) ; turn on "no debugger" but also turn on "no debugger poof".
	str	r0,CPUFlags		; store updated CPU Flags

; If running from rom, or faking rom under debugger,
; store r7 and r8 in the volume label and the misc
; code address.

	tst	r0,#KB_NODBGR
	beq	no_r7r8
	tst	r5,#r6_BOOTDATA		; what is in r7?
	strne	r7,DipirBootData	; new: r7 = ptr to BootData
	streq	r7,DipirBootVolumeLabel	; old: r7 = ptr to DiscLabel
 [	EXT_MISC=1
	str	r8,MiscCode
 ]
no_r7r8

; determine the size of memory.

;	Hard assign all nonuser stacks.
;	put stacks below kernel, above dipir again

	adr	r0,|__my_AIFHeader|

;;; SYSTEM STACKS
;;; 	fiq	2k
;;; 	irq	1k
;;; 	svc	2k
;;; 	abt	2k
;;; 	und	1k
;;; 
;;; an additional 2k may someday be needed for the ACS stacks,
;;; if and when we start using ACS.

ALL_SUPER_STACKS	EQU	&2000	; 8k all super stacks

; initialize stack contents
	mov	r1,#ALL_SUPER_STACKS	; 8k total of system stacks
	ldr	r2,stackinitvalue	; recognizable value
	mov	r3,r2
	mov	r4,r2
	mov	r5,r2

stackloop
	stmfd	r0!,{r2,r3,r4,r5}
	stmfd	r0!,{r2,r3,r4,r5}
	subs	r1,r1,#32
	bgt	stackloop
	add	r0,r0,#ALL_SUPER_STACKS ; get stack top back

;foo	b foo

;	Now set up all other nonuser stacks
	mrs	r1,CPSR		; get current psr bits
	bic	r1,r1,#&0f

;	set up firq stack
	add	r1,r1,#&1	; firq mode 1
	msr	CPSR_ctl,r1
	nop
	mov	sp,r0		; set top of firq stack
	mov	sl,#0		; set firq stack limit

	sub	r0,r0,#&800	; save 2k

;	set up irq stack
	add	r1,r1,#&1	; irq mode 2
	msr	CPSR_ctl,r1
	nop
	mov	sp,r0		; set top of irq stack
;	mov	sl,#0		; set firq stack limit


	sub	r0,r0,#&0400	; save 1k

;	set up svc stack
	add	r1,r1,#&1	; svc mode 3
	msr	CPSR_ctl,r1
	nop
	mov	sp,r0		; set top of swi/svc stack
;	mov	sl,#0		; set firq stack limit


	sub	r0,r0,#&0800	; save 2k

;	set up abt stack
	add	r1,r1,#&4	; abt mode 7
	msr	CPSR_ctl,r1
	nop
	mov	sp,r0		; set top of abt stack
;	mov	sl,#0		; set firq stack limit


	sub	r0,r0,#&0800	; save 2k

;	set up und stack
	add	r1,r1,#&4	; und mode 11
	msr	CPSR_ctl,r1
	nop
	mov	sp,r0		; set top of und stack
;	mov	sl,#0		; set firq stack limit


	sub	r0,r0,#&0400	; save 1k

;;;;	set up acs stack
;;;	IMPORT	ACS_StackTop
;;;	IMPORT	ACS_StackLim
;;;	str	r0,ACS_StackTop
;;;	sub	r0,r0,#&0800	; save 2k
;;;	str	r0,ACS_StackLim

; go back to supervisor mode
	sub	r1,r1,#8	; get mode = 3
	msr	CPSR_ctl,r1	; go to supervisor mode

	ldr	r0,SuperStackSize
	sub	sl,sp,r0		; compute stack limit
	add	sl,sl,#64	; add special stack slop

; set up SWI vector
;	get swi vector to debugger
	mov	r1, #ARM_SWIVec
	ldr	r0,[r1]
	str	r0,olddbgswi
	ldr	r0,CPUFlags
	tst	r0,#KB_NODBGR	; Is mac debugger there?
	bleq	get_target
	ldrne	r0,addr_swi_ignore	; nope just ignore debugger SWIs
	str	r0,oldswi	; save here

;	Now put in paranoid SWI handler
	mov	r0,#ARM_SWIVec
	adr	r1,newswi
	bl	InstallArmVector

;	We do not touch the IRQ interrupts at this time
;	Now get debuggers irq entry and redirect to ourselves
	ldr	r0,CPUFlags
	and	r0,r0,#(KB_NODBGR+KB_NODBGRPOOF)
	cmp	r0,#KB_NODBGR	; is mac debugger there?

;	set up irq handler
	mov	r0,#ARM_IRQVec
	ldr	r1,=irqhandler
	bleq	InstallArmVector

;	set up prefetch-abort handler
	mov	r0,#ARM_PREABTVec
	adr	r1,prefetchabort
	bl	InstallArmVector

;	set up data-abort handler
	mov	r0,#ARM_DATABTVec
	adr	r1,dataabort
	bl	InstallArmVector

; The mac debugger uses the illegal instruction handler
; for breakpoints; if we are using the debugger,
; do not stomp on the vector.

	ldr	r0,CPUFlags
	and	r0,r0,#(KB_NODBGR+KB_NODBGRPOOF)
	cmp	r0,#KB_NODBGR	; is mac debugger there?

;	set up illegal-instruction handler
	mov	r0,#ARM_UNDINSVec
	adr	r1,illins
	bleq	InstallArmVector

; we need to install an undefined instruction handler here
;	Check for Arm600/MMU
	mov	r0,#0		; initialize r0

; Before using this, we need to check out how we
; interact with the debugger's use of ILLINS
; for breakpoints.
;	mrc	p15,0,r0,c0,c0
;	EXPORT	illins_check
;illins_check

	str	r0,Arm600

	str	r10,SuperLimit
	str	r13,SuperStack	; needed for tasks commiting suicide

	b	main

;------------------------------------------------------------

	EXPORT	DipirBootVolumeLabel
DipirBootVolumeLabel	DCD	0
	EXPORT	DipirBootData
DipirBootData		DCD	0
 [	EXT_MISC=1
	EXPORT	MiscCode
MiscCode		DCD	0
 ]
stackinitvalue	DCD	&11111111
;
;------------------------------------------------------------

olddbgswi DCD 0

;	EXPORT PrepareToReboot
;PrepareToReboot
;	ldr	r0,olddbgswi
;	mov	r1, #ARM_SWIVec
;	str	r0,[r1]
;	mov	pc,r14	; done

;------------------------------------------------------------
;
;------------------------------------------------------------

get_target
;	compute the debugger address we want to branch to
	ldr	r0,[r1]		; get the ldr instruction
	mov	r0,r0,LSL #20	; trim off all but
	mov	r0,r0,LSR #20	;   the low 12 bits
	add	r0,r0,r1	; offset by pc at that time
	add	r0,r0,#8	; add another 8 for prefetch
	ldr	r0,[r0]		; get the actual new pc
	mov	pc,r14	; done

;------------------------------------------------------------
; General Purpose Vector Installation Routine
;------------------------------------------------------------

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


;-------------------------------------------------------------
; Illegal Instruction Handler
;-------------------------------------------------------------

	EXPORT	illins
	IMPORT	cillins
illins
	stmfd	sp!,{r0-r12,r14}	; save working registers
	ldr	r9,KernelBase
;	32bit
;	reenable I interrupts for debugger

	mrs	r0,CPSR
	mrs	r1,SPSR
	and	r1,r1,#&80	; get old I bit
	bic	r0,r0,#&80	; clear current I bit
	orr	r0,r0,r1	; bring I bit from SPSR into CPSR
	msr	CPSR_ctl,r0
	mov	r0,sp		; send it current sp as frame ptr

	sub	sl,sp,#&0800-16	; set stack limit ************

	adr	r14,aborts_ret
	b	cillins

;-------------------------------------------------------------
; Prefetch Abort Handler
;-------------------------------------------------------------

	EXPORT	prefetchabort
	IMPORT	cprefetchabort
prefetchabort
	stmfd	sp!,{r0-r12,r14}	; save working registers
	ldr	r9,KernelBase
;	32bit
;	reenable I interrupts for debugger

	mrs	r0,CPSR
	mrs	r1,SPSR
	and	r1,r1,#&80	; get old I bit
	bic	r0,r0,#&80	; clear current I bit
	orr	r0,r0,r1	; bring I bit from SPSR into CPSR
	msr	CPSR_ctl,r0
	mov	r0,sp

	sub	sl,sp,#&0800-16	; set stack limit ************

	adr	r14,aborts_ret
	b	cprefetchabort

;-------------------------------------------------------------
; Illegal Data Abort Handler
;-------------------------------------------------------------

;	Hacks to support slowbus ignoreabort
	EXPORT	ignoreabort
	EXPORT	abortignored
ignoreabort	DCD	0
abortignored	DCD	0

	EXPORT	dataabort
	IMPORT	cdataabort
dataabort
	stmfd	sp!,{r0-r12,r14}	; save working registers
	ldr	r9,ignoreabort
	movs	r9,r9			; should we just ignore this?
	beq	notignoreabort
	str	r9,abortignored		; yup, set flag
	ldmfd	sp!,{r0-r12,r14}
	subs	pc,r14,#8		; jump back now

notignoreabort
	ldr	r9,KernelBase
;	reenable I interrupts for debugger
	mrs	r0,CPSR
	mrs	r1,SPSR
	and	r1,r1,#&80	; get old I bit
	bic	r0,r0,#&80	; clear current I bit
	orr	r0,r0,r1	; bring I bit from SPSR into CPSR
	msr	CPSR_ctl,r0

	mov	r0,sp

	sub	sl,sp,#&0800-16	; set stack limit ************

	bl	cdataabort

aborts_ret
	teq	r0,#0
	ldmnefd	sp!,{r0-r12,r15}^	; restore working registers, go back
	nop
	ldmfd	sp!,{r0-r12,r14}	; restore working registers

;	set up like a swi
;	go into supervisor mode
	mov	r1,#&13		; firq and irq enabled
	msr	CPSR_ctl,r1	; goto to super, no interrupts
	nop

	stmfd	sp!,{r8,r9,r10,r11,r14}	; save working registers
	ldr	r8,KernelBase

	b	swi_ret

;------------------------------------------------------------
;
;------------------------------------------------------------

;	IMPORT	cswi
;addr_cswi DCD	cswi


	EXPORT	Arm600
Arm600	DCD	0
	EXPORT	CPUFlags
CPUFlags	DCD	0
	EXPORT	SaveKB
	EXPORT	SuperStackSize
SaveKB		DCD	0
KernelItem	DCD	0
SuperLimit	DCD	0	; startup stack limit from debugger
SuperStack	DCD	0	; startup stack from debugger
SuperStackSize	DCD	0x800	; 2k supervisor super stack
	EXPORT	SwiSemaphore
SwiSemaphore	DCD	0
	EXPORT	DevSemaphore
DevSemaphore	DCD	0
;|SuperStackBottom| DCD	0


;------------------------------------------------------------
;	Set Kernel Base Address
;------------------------------------------------------------
	EXPORT setKernelBase
; presetting the firq_r9 register does not seem to work.
; we must continue to load the proper KernelBase in r9?
setKernelBase
	str	r0,KernelBase
	str	r0,SaveKB
	; config=32
;	mrs	r1,CPSR		; copy cpsr->r1
;	mov	r2,r1		; save here
;	bic	r1,r1,#&01f	; clear all mode bits
;	orr	r1,r1,#&11	; firq mode
;	msr	CPSR_ctl,r1	; copy r1->cpsr_ctl
;	nop	; enter firq mode
;	mov	r9,r0		; to preset r9 of firq
;	msr	CPSR_ctl,r2	; return to previous mode
;	nop
	mov	pc,r14		; return to caller

;	EXPORT	getr9
;getr9
;	mov	r0,r9
;	mov	pc,r14


;------------------------------------------------------------
;	Get Kernel Base Address
;------------------------------------------------------------

	EXPORT	getKernelBase
getKernelBase
	ldr	r0,KernelBase
	mov	pc,r14		; return to caller

;------------------------------------------------------------
;
;------------------------------------------------------------

oldswi	DCD 0

	IMPORT	CLand
	IMPORT	TailInsertNode
	IMPORT	RemHead
	IMPORT	CLaunch
	IMPORT	swiabort
	IMPORT	ExitTask
	IMPORT	KillSelf

;------------------------------------------------------------
;	Force a task to exit -	never returns; sends status in r0
;				runs in usermode and destroys user's r9
;------------------------------------------------------------
	EXPORT	ForceExit
ForceExit
	ldr	r9,KernelBase	; get KernelBase
	b	ExitTask

;------------------------------------------------------------
;	Force a task to kill itself -	never returns
;					runs in usermode and destroys user's r9
;------------------------------------------------------------
	EXPORT	ForceKill
ForceKill
	ldr	r9,KernelBase	; get KernelBase
	b	KillSelf

;------------------------------------------------------------

taskexit
	ldr	r9,KernelBase	; get KernelBase
	ldr	r0,[r9,#kb_CurrentTask]	; get current Task
	ldr	r0,[r0,#N_Item]	; what is the item#

	mov	r14,#3		; we will kill it
	orr	r14,r14,#&10000	; kernel item#
	b	reenter_to_kill

aborttask
	ldr	r9,KernelBase	; get KernelBase
	ldr	r0,[r9,#kb_CurrentTask]	; get current Task
	ldr	r0,[r0,#N_Item]	; what is the item#

	stmfd   sp!,{r0,r1,r2,r3,r12}
	mov	r1,r13		; pass in stack ptr
	bl	swiabort
	ldmfd	sp!,{r0,r1,r2,r3,r12}	; restore regs

	mov	r14,#3		; we will kill it
	orr	r14,r14,#&10000	; kernel item#
	b	reenter_to_kill

	EXPORT newswi

maybedodebug
; we will start to handle a few swis ourselves
;	add	r8,r8,#&10000	; restore r8 swi#
	tst	r14,#&ff0000	; was this 'xx00xxxx' swi?
	bne	aborttask
	cmp	r14,#&11	; exit called
	beq	taskexit

	cmp	r14,#&10	; return top of memory
	beq	rettopofmem


	ldmfd	sp!,{r8,r9,r10,r11,r14}	; restore regs
	ldr	r15,oldswi

swi_ignore
	stmfd	sp!,{r14}
	ldmfd	sp!,{r15}^

rettopofmem
	mov	r1,#0		; we have no mem for this operation
	ldmfd	sp!,{r8,r9,r10,r11,r15}^	; restore regs

	IMPORT	cswi_overrun
swi_overrun
	stmfd	sp!,{r8}
	mrs	r8,CPSR
	bic	r8,r8,#&80	; clear Ibit, so debugger can run
	msr	CPSR_ctl,r8
	b	cswi_overrun

;------------------------------------------------------------
;	SWI Handler
;  R8:
;  R9:
; R11:	Work register for return
; R12:	SWI function to call
; R14:  work register
;------------------------------------------------------------
newswi
;	Paranoid swi handler
;	The swi code is in r12  msb16bits for folio# and lsb16bits for function

;	But first, to coexist with the debugger:
;	We will check to make sure it is a swi #0
	stmfd	sp!,{r8,r9,r10,r11,r14}	; save working registers
;	make sure we did not come from supervisor state
	mrs	r8,SPSR
	ands	r8,r8,#&03	; get mode bits
	bne	swi_overrun

	ldr	r14,[r14,#-4]	; get the SWI instruction
	bics	r14,r14,#&ff000000	; clear upper 8 bits
	moveq	r14,r12		; if zero we use r12 then

; clear the I bit so debug interrupts can still occur

; got_r12 ... it is now r14
reenter_to_kill

 [	DEVELOPMENT=1
	mrs	r8,CPSR
	bic	r8,r8,#&80	; clear Ibit, so debugger can run
	msr	CPSR_ctl,r8
 ]

	ldr	r8,KernelBase	; get ptr to Kernel Data structures
	ldr	r9,[r8,#kb_ItemTable]
;	check to see if folio# is correct
;	quick check for folio#0,1,2,3,...,15
	mov	r11,r14,lsr #16	; get Folio #
	cmp	r11,#16		; first 15 items are reserved for folios
	blt	skiptests

; Do complete Item lookup processing
; use r14 as a scratch register, I think this is ok, its already
; been saved.
	ldr	r12,[r8,#kb_MaxItem]	; get Maximum number of Items
	cmp	r12,r11			; cmp to requested folio#
	ble	aborttask
;	Get ptr to Item then
;	get correct table
	mov	r12,r11,lsr #7		; 128 items per block
	ldr	r9,[r9,r12,lsl #2]	; get pointer to nth item table
;	now get item
	and	r12,r11,#&0000007f	; just want 0-127
	ldr	r9,[r9,r12,lsl #3]
;	Is this a folio we are pointing to now?
	teq	r9,#0
	beq	aborttask
; This could be quicker, but depends on byte-order, endianness
	ldrb	r11,[r9,#N_SubsysType]
	cmp	r11,#KERNELNODE
	bne	aborttask
	ldrb	r11,[r9,#N_Type]
	cmp	r11,#FOLIONODE
	beq	reenter
	b	aborttask

skiptests
;	Get ptr to Folio then
	ldr	r9,[r9]		; get first table
	ldr	r9,[r9,r11,lsl #3]
	cmp	r9,#0

;	Has this folio been created?
	beq	maybedodebug	; have r11=item#, r14=swi #

;	Is it the debugger folio (id 0)
	cmp	r11,#0
	bne	reenter
;	Yes, the debugger folio is installed
	cmp	r14,#&00000100	; first 256 swis are reserved for kernel use
	blt	maybedodebug
	sub	r14,r14,#&00000100	; offset the swi# by 0x100 for the
					;  debugger folio
;	valid function?
reenter
	ldrb	r11,[r9,#|fl_MaxSwiFunctions|]
	and	r14,r14,#&000000ff	; extract vector #
	cmp	r14,r11
	bge	aborttask

;	set up supervisor stack limit register

	ldr	r11,[r8,#kb_CurrentTask]	; get current running task ptr
	ldr	sl,[r11,#t_ssl]			; get super limit

;	get ptr to functions and jump to it
	ldrb	r11,[r9,#fl_MaxUserFunctions]
	add	r11,r14,r11	; skip to start of swi table
	add	r11,r11,#1	; and then bump another


; multithreaded code starts here
; lock the swi semaphore
   IF	:DEF:|_MULTIT|
	IMPORT internalLockSemaphore
	stmfd	sp!,{r0,r1,r2,r3,r9}	; save working registers
	ldr	r0,SwiSemaphore
	mov	r1,#1
	mov	r9,r8			; load in kernelbase
	bl	internalLockSemaphore
	ldmfd	sp!,{r0,r1,r2,r3,r9}	; restore folio regs
   ENDIF ; |_MULTIT|

; calculate return address
	mov	r14,pc	; get return address
	ldr	pc,[r9,-r11,lsl #2]	; call swi routine

    IF	:DEF:|_MULTIT|
	IMPORT internalUnlockSemaphore
	stmfd	sp!,{r0,r1,r2,r3,r9}	; save working registers
	ldr	r0,SwiSemaphore
	mov	r9,r8
	bl	internalUnlockSemaphore
	ldmfd	sp!,{r0,r1,r2,r3,r9}	; restore folio regs
    ENDIF ; |_MULTIT|


;	ldr	r11,[r8,#kb_CurrentTask]	; get current running task ptr
;	str	sl,[r11,#t_ssl]			; set super limit

	; fall into the general SWI return code...

;------------------------------------------------------------
;	General SWI Return Routine
;------------------------------------------------------------

	EXPORT	swi_ret
swi_ret
;	r9 still points to the swis library base,
;	not necessarily kernelbase!
	DISABLE
	ldrb	r11,[r8,#kb_PleaseReschedule]
	teq	r11,#0
	ldmeqfd	sp!,{r8,r9,r10,r11,r15}^ ; restore and back to user (no request)

; FORBID
;	ldrb	r11,[r8,#kb_Forbid]	; hopefully this is ok.
;	teq	r11,#0
;	ldmnefd	sp!,{r8,r9,r11,r15}^	; restore and back to user (Forbidden)
;	nop

; Is there another task waiting to run?
	ldr	r3,[r8,#kb_TaskReadyQ]
	ldr	r2,[r3,#l_Head]		; get ptr to first node on ReadyQ
	add	r1,r3,#l_filler		; compute anchor
	teq	r2,r1
	ldmeqfd	sp!,{r8,r9,r10,r11,r15}^	; restore and back to user (no new tsk)
	ldr	r1,[r8,#kb_CurrentTask]	; get current running task ptr
	teq	r1,#0			; Is the current task gone?
	bne	compare_em		; do we need to swap em?

;	We lost the current task some how!
;	Dump his superstack also
	mov	r9,r8		; set up real kernelbase
	add	r1,r13,#20+4	; stack now at stack top

	ldr	r10,SuperLimit	; get the old stack limit
	ldr	r13,SuperStack	; get the old stack
;	mov	r10,#0

	cmp	r1,r13		; are we already on the SuperStack ?
	beq	start_next	; yes...don't free it, please

	ldr	r0,[r9,#kb_MemFreeLists]

	ldr	r2,SuperStackSize ;SuperStackSize also used by task.c
	sub	r1,r1,r2	; subtract superstack size

	IMPORT	FreeMemToMemLists
	bl	FreeMemToMemLists

start_next
;	get the next task back again
	b	StartNextTask

; Does the next task have proper priority?
compare_em
	ldrb	r2,[r2,#N_Priority]	; priority of next task on readyq
	ldrb	r3,[r1,#N_Priority]	; priority of current task
	cmp	r3,r2
; If Current tasks priority is greater then next's return now
	ldmgtfd	sp!,{r8,r9,r10,r11,r15}^	; restore and back to user (no new tsk)

;	Got through all the tests, really swap now.
	mov	r3,r8			; r3 = KernelBase now
	ldmfd	sp!,{r8,r9,r10,r11,r14}	; get all registers back now, fix stack
; If we get here we need to swap tasks.
;	r1 currently points to running task (CurrentTask)
	str	r13,[r1,#t_ssp]		; save his superstack ptr
;	str 	r10,[r1,#t_ssl]

	add	r2,r1,#t_regs		; ptr to reg save area
	stmia	r2,{r0-r14}^		; save all user registers
	str	r14,[r1,#t_pc]		; save his pc
	mrs	r9,SPSR_all		; get saved progam status register
	str	r9,[r1,#t_psr]		; save it
	mov	r9,r3			; r9 = KernelBase
;	Now have all registers free to use
	mov	r8,r1			; save task ptr here
	mov	r0,r8			; arg is task ptr

	mov	sl,#0		; ***** Turn off stack limit for this next C call
	bl	CLand		; call C routine to tidy up

	mov	r1,r8
	ldr	r0,[r9,#kb_TaskReadyQ]	; Insert task in this list
	bl	TailInsertNode
;	CurrentTask has been laid to rest

;	Now get the next task
StartNextTask
	ldr	r0,[r9,#kb_TaskReadyQ]
	bl	RemHead
	mov	r5,r0

; 	stack limit should be ok at this point...
	bl	CLaunch
	mov	r0,r5
	b	Launch


;------------------------------------------------------------
;
;------------------------------------------------------------

addr_swi_ignore DCD swi_ignore

;	EXPORT	interceptirq
;interceptirq DCD 0

	EXPORT	irqhandler
	IMPORT	cirq
;	EXPORT	oldirq

;irqPC	DCD	0
;irqCPSR	DCD	0

;oldirq	DCD	0

;qwe_rret
;qwe_ret
;	ldr	r14,irqPC
;	subs	pc,r14,#4
;	nop


;------------------------------------------------------------
;	IRQ Handler
;------------------------------------------------------------

; hack until I figure this out right
irqhandler
	stmfd	sp!,{r0-r12,r14}	; save working registers
	ldr	r9,KernelBase
	bl	cirq
	ldmfd	sp!,{r0-r12,r14}	; restore working registers
	subs	pc,r14,#4


;	str	r14,irqPC	; save this here
;	mrs	r14,SPSR	; get the save processor status
;	str	r14,irqCPSR	; save this here as well
;	tst	r14,#&40	; was firqs disabled?
;	mrs	r14,CPSR	; get current stuff
;	bicne	r14,r14,#&40	; clear if it was clr
;	orreq	r14,r14,#&40	; set if it was set
;	msr	CPSR,r14	; make sure firqs are enabled
;	orr	r14,r14,#&c1	; disable firqs on the way back
;	bic	r14,r14,#&2	; and be in firq mode, and a free set of regs
;	msr	SPSR_all,r14	; we want to come back here in this mode
;	add	r14,pc,#4
;	b oldirq
;
;	nop
;;	load up the original environment
;	ldr	r14,irqCPSR
;	msr	SPSR_all,r14
;;	check to see if we need to do anything new
;;	were we in non-user mode?
;	tsts	r14,#&0f
;	bne qwe_ret
;
;	ldr	r14,interceptirq
;	teq	r14,#0
;	beq	qwe_ret
;
;	ldr	r9,KernelBase
;	ldrb	r14,[r9,#kb_PleaseReschedule]
;	teq	r14,#0
;	beq	qwe_rret	; nothing else todo
;; FORBID
;;	ldrb	r14,[r9,#kb_Forbid]
;;	teq	r14,#0
;;	bne	qwe_rret	; return if forbidden
;
;;	now we need more registers
;;	get return state back again
;	ldr	r14,irqPC
;	sub	r14,r14,#4
;	stmfd	sp!,{r0-r3,r14}	; save firq frame
;
;	ldr	r0,[r9,#kb_CurrentTask]
;
;	ldr	r14,[r9,#kb_TaskReadyQ]	; check to more than one ready task
;	add	r2,r14,#l_filler	; compute anchor
;	ldr	r14,[r14,#l_Head]
;	teq	r14,r2
;	ldmeqfd	sp!,{r0-r3,r15}^	; no other task to check
;
;	ldrb	r2,[r14,#N_Priority]	; get next tasks priority
;	ldrb	r1,[r0,#N_Priority]	; get current tasks priority
;	cmp	r1,r2
;	ldmgtfd	sp!,{r0-r3,r15}^
;
;;	really want to switch tasks!
;;	ldmfd	sp!,{r0-r3,r15}^
;	b	from_irq
;	nop




;------------------------------------------------------------
;	Fast IRQ Handler
;
;------------------------------------------------------------

	IMPORT	cfirq
	IMPORT	roundrobin
;;;	IMPORT	CheckACS
FirqHandler
	sub	r14,r14,#4
	stmfd	sp!,{r0-r4,r14}	; save c scratch, and limit register
	ldr	r9,KernelBase
	bl	cfirq		; call c routine to do work
;;;	bl	CheckACS	; perhaps run some ACS
	bl	roundrobin	; what other task should we run?

	teq	r0,#0		; should we return now?
	ldmeqfd	sp!,{r0-r4,r15}^	; restore c scratch and return

;;; if we were not in user mode, do nothing else.
	mrs	r12,SPSR		; this gets SPSR_firq
	ands	r12,r12,#&0f	; any mode bits set? ie nonuser mode?
	ldmnefd	sp!,{r0-r4,r15}^	; restore c scratch and return

	mov	r8,r0		; Save CurrentTask here
	bl	CLand		; do Landing cleanup

;	Get ptr to Task Ready Q
	ldr	r4,[r9,#kb_TaskReadyQ]

;	Get first one on the list
	mov	r0,r4		; task readyQ
	bl	RemHead
	mov	r11,r0		; save result in r11

;	Now put old task back on ReadyQ
	mov	r1,r8		; ptr to CurrentTask again

;	ldr	r0,[r9,#kb_TaskReadyQ]
	mov	r0,r4		; task readyQ

	bl	TailInsertNode

	mov	r0,r11		; get new task again
	bl CLaunch

	ldmfd	sp!,{r0-r4,r14}	; restore registers

;	r8 points to CurrentTask
	add	r12,r8,#t_regs

;	current mode is FIRQ mode
;	save current tasks user r0-r15 contents to task control block
	stmia	r12,{r0-r14}^	; store users r0-r14

	mov	r4,r14
	mrs	r5,SPSR		; get saved PSR

; Task is now complete saved away
;	Don't need to save registers any more

;	Now jump into supervisor mode from firq mode
	mov	r3,r9		; save ptr to KernelBase
	mov	r2,r8		; save ptr to oldtask here
	mov	r0,r11		; save ptr to starting task here

; switch to super mode and jump to Launch
	mov	r1,#&d3		; firq and irq disabled
	msr	CPSR_ctl,r1	; goto to super, no interrupts
	nop
	mov	r9,r3		; set up KernelBase
	add	r12,r2,#t_pc
	mov	r7,r13
	stmia	r12,{r4-r7}	; store 4 longs fast

;	Fall into the Launch code
;	b	Launch


;------------------------------------------------------------
;	Launch A User Task
;------------------------------------------------------------

	EXPORT	Launch
;	Launch the user task pointed to by R0 (Task *)
;	This routine does not return
Launch

	add	r1,r0,#t_pc	; ptr to base of interesting regs
	ldmia	r1,{r10-r13}	; get all four longs quickly

;	If there is a delay for memory maybe this will be free?
	add	r2,r0,#t_regs	; ptr to base of register stack

	ands	r1,r11,#15	; check for non-user mode

	bne	superlaunch
	msr	SPSR_all,r11	; otherwise return in usermode

	mov	r14,r10		; put r10 (t_pc) in a non shared register
	ldmia	r2,{r0-r14}^	; load 'm up
	nop
	movs	pc,r14		; to user land


;------------------------------------------------------------
;	Launch from supervisor mode
;------------------------------------------------------------

superlaunch
	add	r1,r0,#t_Usersp
	ldmia	r1,{r13,r14}^	; load up the users r13,r14 registers
	nop
	msr	SPSR_all,r12	; load up spsr for user
	msr	CPSR_all,r11	; load up new tasks super cpsr
	ldmia	r2,{r0-r15}	; get all the rest and return
	nop

;------------------------------------------------------------
;	Land
;------------------------------------------------------------

	EXPORT	Land
;	save C process context ;
Land
	str r13,[r0,#t_ssp]	; save super stack pointer here
	str r10,[r0,#t_ssl]

	add r1,r0,#t_Usersp	; User r13 register

	stmia	r1,{r13,r14}^	; Save user registers
	mrs	r1,SPSR		; get User psr
	str	r1,[r0,#t_Userpsr]
	mrs	r1,CPSR		; get current mode
	str	r1,[r0,#t_psr]	; save in task structure
	add r1,r0,#t_regs	; r1 points to base of 16 registers
	stmia r1,{r0-r15}	; save all registers
	sub	r0,r0,r0	;+8 clear r0
	mov	pc,r14		;+12 normal return

;	the next line is required, do not remove!
	mov	pc,r14		; return with r0 = task ptr


;------------------------------------------------------------
;	Disable Interrupts
;------------------------------------------------------------

	EXPORT	Disable

;	Only called in supervisor mode
;	returns current prior state of interrupt bits
;	Disable FIRQ interrupt, we ignore the IRQ interrupt
;
Disable
;	32bit
	mrs	r0,CPSR		; get current psr

;	DISABLE
	orr	r12,r0,#&c0	; or FIRQ disable
	msr	CPSR_ctl,r12	; now disabled
;	mov	r12,r0		; save here for return
;	orr	r12,r12,#&40	; disable FIRQ
;	msr	CPSR_ctl,r12	; load new psr
	mov	pc,r14		; return


;------------------------------------------------------------
;	Enable Irqs
;------------------------------------------------------------

	EXPORT EnableIrq
EnableIrq
	mrs	r1,CPSR		; copy cpsr->r1
	bic	r1,r1,#&80	; clear interrupt bits first
	msr	CPSR_ctl,r1	; copy r1->cpsr_ctl
	mov	pc,r14


;------------------------------------------------------------
;	Enable Interrupts
;------------------------------------------------------------

;	Only called in Supervisor mode
	EXPORT	Enable
Enable
	; 32bit mode
	and	r0,r0,#&c0	; make sure mode stays the same
	mrs	r1,CPSR		; copy cpsr->r1
	bic	r1,r1,#&c0	; clear interrupt bits first
	orr	r1,r1,r0	; or new values in
	msr	CPSR_ctl,r1	; copy r1->cpsr_ctl
	mov	pc,r14


;------------------------------------------------------------
;
;------------------------------------------------------------

;	dummy routines to get compiler to shut up!
	EXPORT	make_func
	EXPORT	make_int
make_func
make_int
	mov	pc,r14

;------------------------------------------------------------
;
;------------------------------------------------------------

	EXPORT	GetUserSP
GetUserSP
	stmfd	r13,{r13}^	; Get User SP
	ldmfd	r13,{r0}		; put it in r0
	mov	pc,r14

	EXPORT	GetUserSL
GetUserSL
	mov	r0,r10
	mov	pc,r14

	IMPORT csuperstackoverflow
	IMPORT cextenduserstack
	EXPORT stackoverflow

;	dummy for stack overflow problems

|__rt_stkovf_split_small|
	mov	ip,sp
|__rt_stkovf_split_big|
stackoverflow

;	sub	sp,fp,#30*4	; for disjoint case
;	stmfd	sp!,{r0-r9,r14}

	stmfd	sp!,{r0-r4,r9,r14}
	mrs	r0,CPSR		; check for mode so proper stack gets handled
	ands	r0,r0,#&03	; get mode bits
	bne	super_overflow

	sub	r0,sp,ip	; find out stack needed

	mov	r1,r10		; give them the old limit register
	mov	r2,lr		; and the calling task PC
	mov	r3,sp		; and the current stack pointer

;	stmfd	sp!,{r10}	; save their old limit register

	mov	r10,#0		; getting new limit register anyway
	ldr	r9,KernelBase
	bl cextenduserstack
	mov	r10,r0		; get new limit register

;	ldmfd	sp!,{r10}	; restore the limit register
;	ldr	r1,[r0]		; see if we extended the stack, or got another one
;	cmp	r1,#0
;	beq	disjointstack
;	sub	r10,r10,r1	; yes, we extended, just calc the new limit


	ldmfd	sp!,{r0-r4,r9,pc}	; and return
;	ldmfd	sp!,{r0-r9,pc}	; and return


;; we have a disjoint stack to deal with
;;disjointstack
;
;	ldr	r1,[r0,#4]	; get the size of the new stack chunk size
;
;	subs	v1,fp,#31*4	; reserved temp fp area
;	add 	sl,r0,#128	; make new limit register
;	add	sp,r0,r1	; new stack pointer
;
;	add	r2,v1,#7*4	; temp fp in old frame
;	LDMDA	fp,{v2-v5}	; old fp, sp, lr, pc
;	adr	v5,stackreentry+12
;	mov	r3, r0		; save stack base
;	STMDA	r2,{r3,v2-v5}	; new return frame in old chunk
;	adr	lr,stackoverflowexit
;	mov	r3,sp		; save p in old frame = NEW sp
;	STMDB	fp,{r2,r3,lr}	; pervert old frame to return here
;
;
;	ldmda	v1,{r0-r9,pc}
;
;	nop
;	nop
;	nop
;	nop
;
;stackreentry
;	stmfd	sp!,{fp,ip,lr,pc}
;
;	EXPORT stackoverflowexit
;stackoverflowexit
;
; freemem goes here
;; need to restore reasonable (ie correct) limit register
;
;	ldr	sl, [fp,#16]
;	ldmdb	fp, {fp,sp,pc}
;
;

	EXPORT super_overflow
super_overflow
	mov	r10,#0
	ldr	r9,KernelBase
	bl csuperstackoverflow
	ldmfd	sp!,{r0-r4,r9,pc}	; and return (and probably die)



	EXPORT	swapstack
swapstack
	mov	r1,r13
	mov	r13,r0
	mov	r0,r1
	mov	pc,r14


; for use with Wait(0) only, swap stack and give 0 limit register
	EXPORT	swapstack2

swapstack2
	mov	r1,r13
	mov	r13,r0
	mov	r0,r1
	mov	r10,#0
	mov	pc,r14

 [	EXT_MISC=1
; code to support misc_code
	EXPORT	callaif
callaif
	stmfd	sp!,{r4-r11,r14}
	mov	r5,r0
	mov	r6,r1
	mov	r7,r2
	mov	r8,r15	; get return address
	mov	r15,r3
	ldmfd	sp!,{r4-r11,r15}
 ]
