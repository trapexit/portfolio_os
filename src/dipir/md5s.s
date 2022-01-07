	;; $Id: md5s.s,v 1.5 1994/04/06 21:32:48 sdas Exp $
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
		INCLUDE	md5.i

		AREA	code_area,CODE
block		PROC
		%	64
acTable
;	Round 1
		DCD	&d76aa478
		DCD	&e8c7b756
		DCD	&242070db
		DCD	&c1bdceee
		DCD	&f57c0faf
		DCD	&4787c62a
		DCD	&a8304613
		DCD	&fd469501
		DCD	&698098d8
		DCD	&8b44f7af
		DCD	&ffff5bb1
		DCD	&895cd7be
		DCD	&6b901122
		DCD	&fd987193
		DCD	&a679438e
		DCD	&49b40821

;	Round 2
		DCD	&f61e2562
		DCD	&c040b340
		DCD	&265e5a51
		DCD	&e9b6c7aa
		DCD	&d62f105d
		DCD	&2441453
		DCD	&d8a1e681
		DCD	&e7d3fbc8
		DCD	&21e1cde6
		DCD	&c33707d6
		DCD	&f4d50d87
		DCD	&455a14ed
		DCD	&a9e3e905
		DCD	&fcefa3f8
		DCD	&676f02d9
		DCD	&8d2a4c8a

;	Round 3
		DCD	&fffa3942
		DCD	&8771f681
		DCD	&6d9d6122
		DCD	&fde5380c
		DCD	&a4beea44
		DCD	&4bdecfa9
		DCD	&f6bb4b60
		DCD	&bebfbc70
		DCD	&289b7ec6
		DCD	&eaa127fa
		DCD	&d4ef3085
		DCD	&4881d05
		DCD	&d9d4d039
		DCD	&e6db99e5
		DCD	&1fa27cf8
		DCD	&c4ac5665

;	Round 4
		DCD	&f4292244
		DCD	&432aff97
		DCD	&ab9423a7
		DCD	&fc93a039
		DCD	&655b59c3
		DCD	&8f0ccc92
		DCD	&ffeff47d
		DCD	&85845dd1
		DCD	&6fa87e4f
		DCD	&fe2ce6e0
		DCD	&a3014314
		DCD	&4e0811a1
		DCD	&f7537e82
		DCD	&bd3af235
		DCD	&2ad7d2bb
		DCD	&eb86d391

MD5Transform	PROC
		ENTER	r4-r10
;	Decode
		ldmia	r1!,{r2-r9}
		mov	ip,#&ff00
		orr	ip,ip,#&ff000000
		MAKEBIGEND	r2
		MAKEBIGEND	r3
		MAKEBIGEND	r4
		MAKEBIGEND	r5
		MAKEBIGEND	r6
		MAKEBIGEND	r7
		MAKEBIGEND	r8
		MAKEBIGEND	r9
		ADR	X,block
		stmia	X,{r2-r9}
		ldmia	r1,{r2-r9}
		add	r1,X,#8*4
		MAKEBIGEND	r2
		MAKEBIGEND	r3
		MAKEBIGEND	r4
		MAKEBIGEND	r5
		MAKEBIGEND	r6
		MAKEBIGEND	r7
		MAKEBIGEND	r8
		MAKEBIGEND	r9
		stmia	r1,{r2-r9}

;	load state, X ptr, and AC ptr
		ldmia	r0,{r5-r8}
		sub	X,r1,#8*4
		ADR	AC,acTable
;	Round 1
		FF	a,b,c,d,0,7
		FF	d,a,b,c,1,12
		FF	c,d,a,b,2,17
		FF	b,c,d,a,3,22
		FF	a,b,c,d,4,7
		FF	d,a,b,c,5,12
		FF	c,d,a,b,6,17
		FF	b,c,d,a,7,22
		FF	a,b,c,d,8,7
		FF	d,a,b,c,9,12
		FF	c,d,a,b,10,17
		FF	b,c,d,a,11,22
		FF	a,b,c,d,12,7
		FF	d,a,b,c,13,12
		FF	c,d,a,b,14,17
		FF	b,c,d,a,15,22

;	Round 2
		GG	a,b,c,d,1,5
		GG	d,a,b,c,6,9
		GG	c,d,a,b,11,14
		GG	b,c,d,a,0,20
		GG	a,b,c,d,5,5
		GG	d,a,b,c,10,9
		GG	c,d,a,b,15,14
		GG	b,c,d,a,4,20
		GG	a,b,c,d,9,5
		GG	d,a,b,c,14,9
		GG	c,d,a,b,3,14
		GG	b,c,d,a,8,20
		GG	a,b,c,d,13,5
		GG	d,a,b,c,2,9
		GG	c,d,a,b,7,14
		GG	b,c,d,a,12,20

;	Round 3
		HH	a,b,c,d,5,4
		HH	d,a,b,c,8,11
		HH	c,d,a,b,11,16
		HH	b,c,d,a,14,23
		HH	a,b,c,d,1,4
		HH	d,a,b,c,4,11
		HH	c,d,a,b,7,16
		HH	b,c,d,a,10,23
		HH	a,b,c,d,13,4
		HH	d,a,b,c,0,11
		HH	c,d,a,b,3,16
		HH	b,c,d,a,6,23
		HH	a,b,c,d,9,4
		HH	d,a,b,c,12,11
		HH	c,d,a,b,15,16
		HH	b,c,d,a,2,23

;	Round 4
		II	a,b,c,d,0,6
		II	d,a,b,c,7,10
		II	c,d,a,b,14,15
		II	b,c,d,a,5,21
		II	a,b,c,d,12,6
		II	d,a,b,c,3,10
		II	c,d,a,b,10,15
		II	b,c,d,a,1,21
		II	a,b,c,d,8,6
		II	d,a,b,c,15,10
		II	c,d,a,b,6,15
		II	b,c,d,a,13,21
		II	a,b,c,d,4,6
		II	d,a,b,c,11,10
		II	c,d,a,b,2,15
		II	b,c,d,a,9,21

		ldmia	r0,{r1-r4}
		add	r5,r1,r5
		add	r6,r2,r6
		add	r7,r3,r7
		add	r8,r4,r8
		stmia	r0,{r5-r8}
		EXIT	r4-r10

Encode		PROC
		ENTER	"r4-r5,r10"
		mov	ip,#&ff00
		orr	ip,ip,#&ff000000
		cmp	r2,#8
		beq	%10
		ldmia	r1,{r2-r5}
		MAKEBIGEND	r2
		MAKEBIGEND	r3
		MAKEBIGEND	r4
		MAKEBIGEND	r5
		stmia	r0,{r2-r5}
		EXIT	"r4-r5,r10"
10
		ldmia	r1!,{r2-r3}
		MAKEBIGEND	r2
		MAKEBIGEND	r3
		stmia	r0!,{r2-r3}
		EXIT	"r4-r5,r10"

;
;	MD5 initialization. Begins an MD5 operation, writing a new context.
;

MD5Init		PROC
		ENTER	r4-r6
		ADR	r1,%10
		ldmia	r1,{r1-r6}
		stmia	r0,{r1-r6}
		EXIT	r4-r6
10
		DCD	&67452301
		DCD	&efcdab89
		DCD	&98badcfe
		DCD	&10325476
		DCD	0
		DCD	0

;
;	MD5 block update operation. Continues an MD5 message-digest
;	operation, processing another message block, and updating the
;	context.
;
;	void MD5Update (MD5_CTX *context, unsigned char *input, unsigned int inputLen)
;
context		RN	4
input		RN	5
inputLen	RN	6
index		RN	7
partLen		RN	8

MD5Update	PROC
		ENTER	R4-R9
;	preserve initial data

		mov	context,r0
		mov	input,r1
		mov	inputLen,r2

;	Compute number of bytes mod 64
;	index = (unsigned int)((context->count[0] >> 3) & 0x3F);
		ldr	index,[r4,#count]
		mov	index,index,LSR #3
		and	index,index,#&3f

;	Update number of bits
;	if ((context->count[0] += ((UINT4)inputLen << 3)) < ((UINT4)inputLen << 3))
;	  context->count[1]++;

		add	r3,context,#count
		ldmia	r3,{r0-r1}
		add	r0,r0,inputLen,LSL #3
		cmp	r0,inputLen,LSL #3
		addlt	r1,r1,#1

;	context->count[1] += ((UINT4)inputLen >> 29);

		add	r1,r1,inputLen,LSR #29
		stmia	r3,{r0-r1}

;	partLen = 64 - index;

		rsb	partLen,index,#64

;	if (inputLen >= partLen) {
;	MD5_memcpy((POINTER)&context->buffer[index], (POINTER)input, partLen);
;	MD5Transform (context->state, context->buffer);
;
;	for (i = partLen; i + 63 < inputLen; i += 64)
;	  MD5Transform (context->state, &input[i]);
;
;	index = 0;
;	}
;	else
;	i = 0;

		cmp	inputLen,partLen
		movlt	partLen,#0		; partlen = i
		blt	%20
;	{
;	MD5_memcpy((POINTER)&context->buffer[index], (POINTER)input, partLen);

		add	r0,context,#buffer
		add	r0,r0,index
		mov	r1,input
		mov	r2,partLen
		bl	MD5_memcpy

;	MD5Transform (context->state, context->buffer);

		add	r0,context,#state
		add	r1,context,#buffer
		bl	MD5Transform

;	for (i = partLen; i + 63 < inputLen; i += 64)
;	   MD5Transform (context->state, &input[i]);
;	partLen = i
		b	%10
0
		add	r0,context,#state
		add	r1,input,partLen
		bl	MD5Transform

		add	partLen,partLen,#64
10
		add	r0,partLen,#63
		cmp	r0,inputLen
		blt	%0

;	index = 0;
		mov	index,#0
;	}
20
;	Buffer remaining input
;	MD5_memcpy((POINTER)&context->buffer[index], (POINTER)&input[i],inputLen-i);
;
		add	r0,context,#buffer
		add	r0,r0,index
		add	r1,input,partLen
		sub	r2,inputLen,partLen
		bl	MD5_memcpy
		EXIT	R4-R9

;	MD5 finalization. Ends an MD5 message-digest operation, writing the
;	the message digest and zeroizing the context.
;
;	void MD5Final (unsigned char digest[16],MD5_CTX *context)
context		RN	4
digest		RN	5
index		RN	7
padLen		RN	8

bits
		DCD	0,0
PADDING
		DCD	&80000000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
MD5Final	PROC
		ENTER	r4-r9
		mov	digest,r0
		mov	context,r1

;	Encode (bits, context->count, 8);

		ADR	r0,bits
		add	r1,context,#count
		mov	r2,#8
		bl	Encode

;	Pad out to 56 mod 64.
;	index = (unsigned int)((context->count[0] >> 3) & 0x3F);

		ldr	index,[context,#count]
		mov	index,index,LSR #3
		and	index,index,#&3f

;	padLen = (index < 56) ? (56 - index) : (120 - index);

		rsbs	padLen,index,#56
		rsble	padLen,index,#120

;	MD5Update (context, PADDING, padLen);

		mov	r0,context
		ADR	r1,PADDING
		mov	r2,padLen
		bl	MD5Update

;	Append length (before padding) */
;	MD5Update (context, bits, 8);

		mov	r0,context
		ADR	r1,bits
		mov	r2,#8
		bl	MD5Update

;	Store state in digest */
;	Encode (digest, context->state, 16);

		mov	r0,digest
		add	r1,context,#state
		mov	r2,#16
		bl	Encode

;	Zeroize sensitive information.

		mov	r8,context
		mov	r0,#0
		mov	r1,#0
		mov	r2,#0
		mov	r3,#0
		mov	r4,#0
		mov	r5,#0
		mov	r6,#0
		mov	r7,#0
		stmia	r8!,{r0-r5}
		stmia	r8!,{r0-r7}
		stmia	r8,{r0-r7}
		ADRL	r8,block
		stmia	r8!,{r0-r7}
		stmia	r8,{r0-r7}
		EXIT	r4-r9

;	MD5_memcpy (POINTER output,POINTER input,unsigned int len)

MD5_memcpyLoop
		ldrb	r3,[r1],#1
		strb	r3,[r0],#1
MD5_memcpy
		subs	r2,r2,#1
		bpl	MD5_memcpyLoop
		mov	PC,LK
MD5End		PROC

NEVER	EQU	1
	IF NEVER = 0
;
;	MD5 block update operation. Continues an MD5 message-digest
;	operation, processing another message block, and updating the
;	context.
;
;	void ReadMD5Update1 (MD5_CTX *context, unsigned char *input, unsigned int inputLen)
;
context		RN	4
input		RN	5
inputLen	RN	6
index		RN	7
partLen		RN	8
endFlag		RN	9
inputEnd	RN	10
ReadMD5State
		DCD	0	; r4 context
		DCD	0	; r5 input
		DCD	0	; r6 inputLen
		DCD	0	; r7 index
		DCD	0	; r8 partLen
		DCD	0	; r9 endFlag
ReadMD5Update1	PROC
		ENTER	R4-R9
;	preserve initial data

		mov	context,r0
		mov	input,r1
		mov	inputLen,r2

;	Compute number of bytes mod 64
;	index = (unsigned int)((context->count[0] >> 3) & 0x3F);
		ldr	index,[r4,#count]
		mov	index,index,LSR #3
		and	index,index,#&3f

;	Update number of bits
;	if ((context->count[0] += ((UINT4)inputLen << 3)) < ((UINT4)inputLen << 3))
;	  context->count[1]++;

		add	r3,context,#count
		ldmia	r3,{r0-r1}
		add	r0,r0,inputLen,LSL #3
		cmp	r0,inputLen,LSL #3
		addlt	r1,r1,#1

;	context->count[1] += ((UINT4)inputLen >> 29);

		add	r1,r1,inputLen,LSR #29
		stmia	r3,{r0-r1}

;	partLen = 64 - index;

		rsb	partLen,index,#64
		mov	endFlag,#0
		ADR	r0,ReadMD5State
		stmia	r0,{r4-r9}
		EXIT	R4-R9

ReadMD5Update2	PROC
		ENTER	R4-R10
		mov	inputEnd,r0

		ADR	r0,ReadMD5State
		ldmia	r0,{r4-r9}

		cmp	endFlag,#0
		bne	%20

		sub	r0,inputEnd,input
		cmp	r0,inputLen
		beq	%1
		cmp	r0,partLen
		blt	%99
1
		cmp	partLen,#64
		bgt	%10

;	if (inputLen >= partLen) {
;	MD5_memcpy((POINTER)&context->buffer[index], (POINTER)input, partLen);
;	MD5Transform (context->state, context->buffer);
;
;	for (i = partLen; i + 63 < inputLen; i += 64)
;	  MD5Transform (context->state, &input[i]);
;
;	index = 0;
;	}
;	else
;	i = 0;

		cmp	inputLen,partLen
		movlt	partLen,#0		; partlen = i
		blt	%20
;	{
;	MD5_memcpy((POINTER)&context->buffer[index], (POINTER)input, partLen);

		add	r0,context,#buffer
		add	r0,r0,index
		mov	r1,input
		mov	r2,partLen
		bl	MD5_memcpy

;	MD5Transform (context->state, context->buffer);

		add	r0,context,#state
		add	r1,context,#buffer
		bl	MD5Transform

;	for (i = partLen; i + 63 < inputLen; i += 64)
;	   MD5Transform (context->state, &input[i]);
;	partLen = i
		b	%10
0
		add	r0,context,#state
		add	r1,input,partLen
		bl	MD5Transform

		add	partLen,partLen,#64
10
		sub	r0,inputEnd,input
		cmp	r0,inputLen
		beq	%2
		cmp	r0,partLen
		blt	%99
2
		add	r0,partLen,#63
		cmp	r0,inputLen
		blt	%0

;	index = 0;
		mov	index,#0
		mov	endFlag,#1
;	}
20
;	Buffer remaining input
;	MD5_memcpy((POINTER)&context->buffer[index], (POINTER)&input[i],inputLen-i);
;
		sub	r0,inputEnd,input
		cmp	r0,inputLen
		blt	%99
		add	r0,context,#buffer
		add	r0,r0,index
		add	r1,input,partLen
		sub	r2,inputLen,partLen
		bl	MD5_memcpy
99
		ADR	r0,ReadMD5State
		stmia	r0,{r4-r9}
		EXIT	R4-R10
	ENDIF




		END
