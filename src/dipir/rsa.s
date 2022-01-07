	;; $Id: rsa.s,v 1.5 1994/02/09 01:27:17 limes Exp $
;	Assembly flags
DEBUG		EQU	1
;    Copyright (C) 1992, The 3DO Company, Inc.

		INCLUDE	macros.i
		INCLUDE	structs.i
		INCLUDE	nodes.i
		INCLUDE	mem.i
		INCLUDE	folio.i
		INCLUDE	list.i
		INCLUDE	graphics.i
		INCLUDE	item.i
		INCLUDE	kernelmacros.i
		INCLUDE	kernel.i
		INCLUDE	kernelnodes.i
		INCLUDE	operamath.i
		INCLUDE	registers.i
		INCLUDE	hardware.i
		INCLUDE	task.i

		INCLUDE	stack.i

		AREA	code_area,CODE
Op1		RN	9
Op2		RN	10
Result		RN	11
ResultOld	RN	12

	BEGINSTRUCT	RSAStruct
		PTR	output
		PTR	input
		LONG	exponent
		PTR	Source2Ptr
		LONG	ShiftTemp
		LONG	ShiftCount
		LONG	WordCount
		STRUCTRESERVE	MulModReturn,12,4
		STRUCTRESERVE	InputBuffer,64,4
		STRUCTRESERVE	Source1,64,4
		STRUCTRESERVE	Mod,68,4
		STRUCTRESERVE	buffer1,68,4
		STRUCTRESERVE	buffer2,68,4
	ENDSTRUCT

;
;	Name:
;		RSA
;	Purpose:
;		Security check 
;	Entry:
;		input
;	Exit:
;		None
;	C Prototype:
;		int RSA (unsigned char *output,unsigned char *input,unsigned char *mod,int32 exponent)
;
;

aRSA		PROC
		ENTER	r4-r10
		PUSH	r11
		sub	sp,sp,#_SIZEOF_RSAStruct
		stmia	sp,{r0-r1,r3}	; output,input,exponent

;	copy mod to buffer

		add	r0,sp,#Mod
		mov	r3,#0
		str	r3,[r0],#3
		mov	r4,#65
0
		ldrb	r3,[r2],#1
		strb	r3,[r0],#1
		subs	r4,r4,#1
		bgt	%0

;	copy input to InputBuffer

		add	r0,sp,#InputBuffer
		mov	r2,#64
1
		ldrb	r3,[r1],#1
		strb	r3,[r0],#1
		subs	r2,r2,#1
		bgt	%1

;	copy InputBuffer to Source1

		add	r1,sp,#InputBuffer
		add	r0,sp,#Source1
		ldmia	r1!,{r3-r10}
		stmia	r0!,{r3-r10}
		ldmia	r1!,{r3-r10}
		stmia	r0!,{r3-r10}

;	init result buffers

		add	ResultOld,sp,#buffer1
		add	Result,sp,#buffer2

;	find msb of exponent

		ldr	r0,[sp,#exponent]
		movs	r1,r0
		moveq	r0,#-1
		beq	%99		;error not correct RSA number
		mov	r0,#16
		movs	r2,r1,LSR r0
		addne	r0,r0,#8
		subeq	r0,r0,#8
		movs	r2,r1,LSR r0
		addne	r0,r0,#4
		subeq	r0,r0,#4
		movs	r2,r1,LSR r0
		addne	r0,r0,#2
		subeq	r0,r0,#2
		movs	r2,r1,LSR r0
		addne	r0,r0,#1
		subeq	r0,r0,#1
		movs	r2,r1,LSR r0
		addne	r0,r0,#1

;	raise to power

1
		subs	r0,r0,#1
		ble	%90
		add	r2,sp,#Source1
		str	r2,[sp,#Source2Ptr]
		bl	MulMod		; square
		movs	r2,r1,LSR r0
		bcc	%1
		add	r2,sp,#InputBuffer
		str	r2,[sp,#Source2Ptr]
		bl	MulMod		; Source1 * input
		b	%1

;	copy result to output
90
		add	r1,sp,#Source1
		ldr	r0,[sp,#output]
		mov	r2,#64
91
		ldrb	r3,[r1],#1
		strb	r3,[r0],#1
		subs	r2,r2,#1
		bgt	%91

;	did calc okay

		mov	r0,#0
99
		add	sp,sp,#_SIZEOF_RSAStruct
		POP	r11
		EXIT	r4-r10

;	Source1 = Mod(Source1 * [Source2Ptr]) 

MulMod		PROC
		add	r2,sp,#MulModReturn
		stmia	r2,{r0-r1,LK}

;	zero the result
		mov	Op1,ResultOld
		mov	r0,#0
		mov	r1,#0
		mov	r2,#0
		mov	r3,#0
		mov	r4,#0
		mov	r5,#0
		mov	r6,#0
		mov	r7,#0
		mov	r8,#0
		stmia	Op1!,{r0-r7}
		stmia	Op1,{r0-r8}

;	get first mod

		bl	CheckSub
		mov	r0,#0
0
		str	r0,[sp,#WordCount]

;	get shift word

		ldr	r1,[sp,#Source2Ptr]
		ldr	r1,[r0,r1]
		str	r1,[sp,#ShiftTemp]

;	shift 32 bits

		mov	r0,#32
1
		str	r0,[sp,#ShiftCount]
		bl	Shift

;	next shift

		ldr	r0,[sp,#ShiftCount]
		subs	r0,r0,#1
		bgt	%1

;	next word

		ldr	r0,[sp,#WordCount]
		add	r0,r0,#4
		cmp	r0,#64
		blt	%0

;	copy ResultOld to Source1

		add	Op1,ResultOld,#68
		add	Op2,sp,#Source1+64
		ldmdb	Op1!,{r0-r7}
		stmdb	Op2!,{r0-r7}
		ldmdb	Op1!,{r0-r7}
		stmdb	Op2!,{r0-r7}

		add	r0,sp,#MulModReturn
		ldmia	r0,{r0-r1,PC}		; return
Shift
		add	Op1,ResultOld,#68

		ldmdb	Op1,{r0-r7}
		movs	r7,r7,LSL #1
		adcs	r6,r6,r6
		adcs	r5,r5,r5
		adcs	r4,r4,r4
		adcs	r3,r3,r3
		adcs	r2,r2,r2
		adcs	r1,r1,r1
		adcs	r0,r0,r0
		stmdb	Op1!,{r0-r7}

		ldmdb	Op1,{r0-r8}
		adcs	r8,r8,r8
		adcs	r7,r7,r7
		adcs	r6,r6,r6
		adcs	r5,r5,r5
		adcs	r4,r4,r4
		adcs	r3,r3,r3
		adcs	r2,r2,r2
		adcs	r1,r1,r1
		adcs	r0,r0,r0
		stmdb	Op1!,{r0-r8}
CheckAdd
		ldr	r0,[sp,#ShiftTemp]
		movs	r0,R0,LSL #1
		str	r0,[sp,#ShiftTemp]
		bcc	CheckSub
;	add
		add	Op1,ResultOld,#68
		add	Op2,sp,#Source1+64
		add	ResultOld,ResultOld,#68

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		adds	r3,r3,r7
		adcs	r2,r2,r6
		adcs	r1,r1,r5
		adcs	r0,r0,r4
		stmdb	ResultOld!,{r0-r3}

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		adcs	r3,r3,r7
		adcs	r2,r2,r6
		adcs	r1,r1,r5
		adcs	r0,r0,r4
		stmdb	ResultOld!,{r0-r3}

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		adcs	r3,r3,r7
		adcs	r2,r2,r6
		adcs	r1,r1,r5
		adcs	r0,r0,r4
		stmdb	ResultOld!,{r0-r3}

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		adcs	r3,r3,r7
		adcs	r2,r2,r6
		adcs	r1,r1,r5
		adcs	r0,r0,r4
		stmdb	ResultOld!,{r0-r3}

		ldmdb	Op1!,{r0}
		adcs	r0,r0,#0
		stmdb	ResultOld!,{r0}
CheckSub
		mov	Op1,ResultOld
		add	Op2,sp,#Mod
		ldmia	Op1!,{r0-r3}
		ldmia	Op2!,{r4-r7}
		cmp	r0,r4
		beq	%10
		bhi	%20
		mov	PC,LK
10
		cmp	r1,r5
		bhi	%20
		beq	%11
		mov	PC,LK
11
		cmp	r2,r6
		bhi	%20
		beq	%12
		mov	PC,LK
12
		cmp	r3,r7
		bhi	%20
		beq	%13
		mov	PC,LK
13
		ldmia	Op1!,{r0-r3}
		ldmia	Op2!,{r4-r7}
		cmp	r0,r4
		bhi	%20
		beq	%14
		mov	PC,LK
14
		cmp	r1,r5
		bhi	%20
		beq	%15
		mov	PC,LK
15
		cmp	r2,r6
		bhi	%20
		beq	%16
		mov	PC,LK
16
		cmp	r3,r7
		bhi	%20
		beq	%20
		mov	PC,LK
20
;	Sub
		add	Op1,ResultOld,#68
		add	Op2,sp,#Mod+68
		add	Result,Result,#68

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		subs	r3,r3,r7
		sbcs	r2,r2,r6
		sbcs	r1,r1,r5
		sbcs	r0,r0,r4
		stmdb	Result!,{r0-r3}

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		sbcs	r3,r3,r7
		sbcs	r2,r2,r6
		sbcs	r1,r1,r5
		sbcs	r0,r0,r4
		stmdb	Result!,{r0-r3}

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		sbcs	r3,r3,r7
		sbcs	r2,r2,r6
		sbcs	r1,r1,r5
		sbcs	r0,r0,r4
		stmdb	Result!,{r0-r3}

		ldmdb	Op1!,{r0-r3}
		ldmdb	Op2!,{r4-r7}
		sbcs	r3,r3,r7
		sbcs	r2,r2,r6
		sbcs	r1,r1,r5
		sbcs	r0,r0,r4
		stmdb	Result!,{r0-r3}

		ldmdb	Op1!,{r0}
		ldmdb	Op2!,{r4}
		sbcs	r0,r0,r4
		stmdb	Result!,{r0}

		movcc	PC,LK		; underflow disregard result and return

		SWPREG	Result,ResultOld
		b	CheckSub



		END

