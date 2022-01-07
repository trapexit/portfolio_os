	;; $Id: sherry.s,v 1.12 1994/09/23 17:26:07 vertex Exp $
;
; definition of addresses for ugo hardware
;

	AREA	|ASMCODE|, CODE, READONLY

ADDR_SRAM	EQU	&03700000
ADDR_VRAM	EQU	&00000000

Addr_INTREG	EQU	&02380000
TopRam		EQU	ADDR_SRAM+&040000
ENVID_BIT	EQU	&1000
BIG_ENDIAN	EQU	&4000
ENCLUT_BIT	EQU	&0200
ENVINT_BIT	EQU	&0800
CLIO		EQU	&03400000
MADAM		EQU	&03300000
	ALIGN	4		; alignment paranoia

	EXPORT	enable_firq_interrupt
enable_firq_interrupt
	mov	r0,#CLIO	; address of CLIO
	mov	r1,#5		; line 4 for vblank interrupts
	str	r1,[r0,#12]	; set scan line for V1
	mov	r1,#2		; VINT1 bit
	str	r1,[r0,#&48]	; enable V1 interrupt
;	mov	r2,#TopRam
;	mov	r1,#Addr_INTREG
;;	32bit
;	mov	r0,#(ENVID_BIT+ENCLUT_BIT+BIG_ENDIAN)
;	str	r0,[r1]
;	orr	r0,r0,#ENVINT_BIT
;	str	r0,[r1]
;	str	r0,[r2,#-&13c]
	mov	pc,r14

;	EXPORT	IsSherry
;IsSherry
;;	r0 parameter, 0 = Madam
;;                     1 = Clio
;	cmp	r0,#(1)
;	moveq	r0,#CLIO	; if 1 in r0, return CLIO, anything else
;	movne	r0,#MADAM	; gets the MADAM return
;
;	ldr	r0,[r0]
;	mov	pc,r14

;	EXPORT	GetIntBits
;	r0 parameter, 0 = Int0Bits
;                     1 = Int1Bits
;GetIntBits
;	mov	r12,#CLIO
;	add	r12,r12,r0,lsl #5
;	ldr	r1,[r12,#&48]	; get interrupt enables
;	ldr	r0,[r12,#&40]	; get all interrupt bits
;	and	r0,r0,r1	; only interested in the ones enabled.
;	mov	pc,r14

	EXPORT	Ptr_Addr_SRAM
Ptr_Addr_SRAM	DCD	ADDR_SRAM
	EXPORT	Len_SRAM
Len_SRAM	DCD	256*1024

	EXPORT	GetRowCnt
GetRowCnt
	mov	r0,#CLIO
	orr	r0,r0,#&34
	ldr	r0,[r0]
	and	r0,r0,#255
	mov	pc,r14

	EXPORT	PtrMacPkt
;PtrMacPkt	DCD	&ff80
PtrMacPkt	DCD	ADDR_SRAM+&3ff80

	EXPORT	DebugTrigger
	EXPORT	DebugAbortTrigger
DebugTrigger
	;b	ADDR_VRAM+&40
	mov	r0,#0
DebugAbortTrigger
;	passes r0,r1,r2,r3 onto to debugger
	mov	r15,#&40
;	b	&0fec0


	EXPORT	read3ctrs
;	read 48 bit counter (3 ctrs linked)
;	Use the middle word as a mini timestamp while
;	reading the two order words. If it does not
;	change from beginning to end then we were
;	successful in reading all three without a
;	cascade occuring. This could also be done
;	by using the lsw for checking but this should
;	work as well and the chance of needed to reread
;	is much lower.
read3ctrs
	mov	r12,#CLIO
	orr	r12,r12,#&100
	ldr	r2,[r12,#8]	; read lsw secs
	ldr	r1,[r12]	; read usecs/ticks
	ldr	r3,[r12,#16]	; read msw secs
	ldr	r12,[r12,#8]
	cmp	r12,r2		; still the same?
	bne	read3ctrs	; nope, try again

	orr	r2,r2,r3,lsl #16	; assemble 32 bit seconds ctr
	mvn	r2,r2
	str	r2,[r0]		; store 32bit secs

	str	r1,[r0,#4]
	mov	pc,r14

;	EXPORT	read4ctrs
;;	read 64 bit counter (4 ctrs linked)
;;	Use the middle word as a mini timestamp while
;;	reading the two order words. If it does not
;;	change from beginning to end then we were
;;	successful in reading all three without a
;;	cascade occuring. This could also be done
;;	by using the lsw for checking but this should
;;	work as well and the chance of needed to reread
;;	is much lower.
;;	r0 = ptr to ulong t[2]
;read4ctrs
;	mov	r12,#CLIO
;	orr	r12,r12,#&100
;
;	; we depend on the upper 16 bits being zero! */
;	ldr	r2,[r12,#8]	; read msw usecs
;	ldr	r1,[r12,#24]	; read msw of seconds
;	ldr	r3,[r12,#16]	; read lsw of seconds
;	orr	r3,r3,r1,lsl #16	; assemble 32 bit seconds ctr
;	ldr	r1,[r12]	; read lsw of usecs
;	ldr	r12,[r12,#8]	; read msw usecs again
;	cmp	r12,r2		; still the same?
;	bne	read4ctrs
;	mvn	r3,r3
;	str	r3,[r0]		; store the secs
;	orr	r1,r1,r12,lsl #16	; construct 32 bit word
;	str	r1,[r0,#4]
;	mov	pc,r14

	END
