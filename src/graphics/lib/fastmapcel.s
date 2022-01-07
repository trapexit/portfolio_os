	;; $Id: fastmapcel.s,v 1.2 1994/06/16 16:40:50 slandrum Exp $
;
;	Name:
;		FastMapCelInit
;		FastMapCel
;		FastMapCelf26
;	By:
;		Greg Omi
;	Date:
;		12-24-93
;
;	Copyright (C) 1993, The 3DO Company, Inc.
;

;	Assembly flags
DEBUG		EQU	1


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
		ALIGN
;
;	Name:
;		FastMapCelInit
;	Purpose:
;		If width and height are powers of 2 then
;			ccb_Width = log2(ccb_Width)
;			ccb_Height = log2(ccb_Height)
;		else
;			ccb_Width = -(0x10000/ccb_Width)
;			ccb_Height = 0x10000/ccb_Height
;	Entry:
;		Cel index
;	Exit:
;		None
;	C Prototype:
;		void CelInit (CCB *ccb)
;
;
FastMapCelInit	PROC
		ENTER	r4-r5
		ldr	r1,[r0,#ccb_Width]
		ldr	r2,[r0,#ccb_Height]
		sub	r3,r1,#1	;  check for power of two
		ands 	r3,r3,r1
		bne	%10
		sub	r3,r2,#1
		ands 	r3,r3,r2
		bne	%10

		mov	r3,#0
0
		movs	r1,r1,LSR #1
		addne	r3,r3,#1
		bne	%0

		mov	r4,#0
1
		movs	r2,r2,LSR #1
		addne	r4,r4,#1
		bne	%1

		str	r3,[r0,#ccb_Width]
		str	r4,[r0,#ccb_Height]
		EXIT	r4-r5
10
		mov	r5,r0
		mov	r4,r2
		mov	r0,r1
		mov	r1,#&20000
		LIBCALL	__rt_udiv
		mov	r0,r0,ASR #1
		adc	r0,r0,#0
		rsb	r0,r0,#0
		str	r0,[r5,#ccb_Width]

		mov	r0,r4
		mov	r1,#&20000
		LIBCALL	__rt_udiv
		mov	r0,r0,ASR #1
		adc	r0,r0,#0
		str	r0,[r5,#ccb_Height]

		EXIT	r4-r5
;
;	Name:
;		FastMapCel
;	Purpose:
;		Sets up delta fields to creat cel mapped to
;		four points in quad argument.
;	Entry:
;		CCB Pointer
;		Quad pointer
;	Exit:
;		None
;	C Prototype:
;		void FastMapCelf16 (CCB *ccb, Point *quad)
;	Max time:
;		Approx. 103 + 16(ENTER) + 14(EXIT) = 133 cycles or 10.64Usec
;
FastMapCel	PROC
		ENTER	r4-r9
;	load celmap
		ldmia	r1,{r1-r8}

;	calc delta's

		sub	r3,r3,r1		; HDX = x1-x0
		sub	r4,r4,r2		; HDY = y1-y0
		sub	r9,r5,r7		; x2-x3
		sub	r5,r7,r1		; VDX = x3-x0
		sub	r7,r9,r3		; HDDX = x2-x3-(x0-x1)
		sub	r9,r6,r8		; y2-y3
		sub	r6,r8,r2		; VDY = y3-y0
		sub	r8,r9,r4		; HDDY = y2-y3-(y0-y1)

;	shift around results to proper positions

		mov	r1,r1,LSL #16
		mov	r2,r2,LSL #16

		add	r9,r0,#ccb_Width
		ldmia	r9,{r9,ip}
		cmp	r9,#0
		bmi	%10

;	Power of 2 Cel

		mov	r3,r3,LSL #20
		mov	r3,r3,ASR r9	; HDX
		mov	r4,r4,LSL #20
		mov	r4,r4,ASR r9	; HDY
		mov	r5,r5,LSL #16
		mov	r5,r5,ASR ip	; VDX
		mov	r6,r6,LSL #16
		mov	r6,r6,ASR ip	; VDY
		add	r9,r9,ip
		mov	r7,r7,LSL #20
		mov	r7,r7,ASR r9	; HDDX
		mov	r8,r8,LSL #20
		mov	r8,r8,ASR r9	; HDDY

		add	r0,r0,#ccb_XPos
		stmia	r0,{r1-r8}

		EXIT	r4-r9

;	Not Power of 2 Cel
10
		rsb	r9,r9,#0

		mov	r3,r3,LSL #4
		mul	r3,r9,r3	; HDX

		mov	r4,r4,LSL #4
		mul	r4,r9,r4	; HDY

		mul	r5,ip,r5	; VDX

		mul	r6,ip,r6	; VDY

		mul	r9,ip,r9
		mov	r9,r9,LSR #16

		mov	r7,r7,LSL #4
		mul	r7,r9,r7	; HDDX

		mov	r8,r8,LSL #4
		mul	r8,r9,r8	; HDDX

		add	r0,r0,#ccb_XPos
		stmia	r0,{r1-r8}

		EXIT	r4-r9
;
;	Name:
;		FastMapCelf16
;	Purpose:
;		Sets up delta fields to creat cel mapped to
;		four frac16 points in quad argument.
;	Entry:
;		CCB Pointer
;		Quad pointer
;	Exit:
;		None
;	C Prototype:
;		void FastMapCelf16 (CCB *ccb, Point *quad)
;	Max time:
;		Approx. 111 + 16(ENTER) + 14(EXIT) = 141 cycles or 11.28Usec
;
FastMapCelf16	PROC
		ENTER	r4-r9
;	load celmap
		ldmia	r1,{r1-r8}

;	shift down values

		mov	r1,r1,ASR #16
		mov	r2,r2,ASR #16
		mov	r3,r3,ASR #16
		mov	r4,r4,ASR #16
		mov	r5,r5,ASR #16
		mov	r6,r6,ASR #16
		mov	r7,r7,ASR #16
		mov	r8,r8,ASR #16

;	calc delta's

		sub	r3,r3,r1		; HDX = x1-x0
		sub	r4,r4,r2		; HDY = y1-y0
		sub	r9,r5,r7		; x2-x3
		sub	r5,r7,r1		; VDX = x3-x0
		sub	r7,r9,r3		; HDDX = x2-x3-(x0-x1)
		sub	r9,r6,r8		; y2-y3
		sub	r6,r8,r2		; VDY = y3-y0
		sub	r8,r9,r4		; HDDY = y2-y3-(y0-y1)

;	shift around results to proper positions

		mov	r1,r1,LSL #16
		mov	r2,r2,LSL #16

		add	r9,r0,#ccb_Width
		ldmia	r9,{r9,ip}
		cmp	r9,#0
		bmi	%10

;	Power of 2 Cel

		mov	r3,r3,LSL #20
		mov	r3,r3,ASR r9	; HDX
		mov	r4,r4,LSL #20
		mov	r4,r4,ASR r9	; HDY
		mov	r5,r5,LSL #16
		mov	r5,r5,ASR ip	; VDX
		mov	r6,r6,LSL #16
		mov	r6,r6,ASR ip	; VDY
		add	r9,r9,ip
		mov	r7,r7,LSL #20
		mov	r7,r7,ASR r9	; HDDX
		mov	r8,r8,LSL #20
		mov	r8,r8,ASR r9	; HDDY

		add	r0,r0,#ccb_XPos
		stmia	r0,{r1-r8}

		EXIT	r4-r9

;	Not Power of 2 Cel
10
		rsb	r9,r9,#0

		mov	r3,r3,LSL #4
		mul	r3,r9,r3	; HDX

		mov	r4,r4,LSL #4
		mul	r4,r9,r4	; HDY

		mul	r5,ip,r5	; VDX

		mul	r6,ip,r6	; VDY

		mul	r9,ip,r9
		mov	r9,r9,LSR #16

		mov	r7,r7,LSL #4
		mul	r7,r9,r7	; HDDX

		mov	r8,r8,LSL #4
		mul	r8,r9,r8	; HDDX

		add	r0,r0,#ccb_XPos
		stmia	r0,{r1-r8}

		EXIT	r4-r9



		END

