; $Id: md5.i,v 1.3 1994/02/09 04:13:38 limes Exp $
A	RN	5
B	RN	6
C	RN	7
D	RN	8
a	RN	5
b	RN	6
c	RN	7
d	RN	8
X	RN	10
AC	RN	2

S11	EQU	7
S12	EQU	12
S13	EQU	17
S14	EQU	22
S21	EQU	5
S22	EQU	9
S23	EQU	14
S24	EQU	20
S31	EQU	4
S32	EQU	11
S33	EQU	16
S34	EQU	23
S41	EQU	6
S42	EQU	10
S43	EQU	15
S44	EQU	21

	BEGINSTRUCT	MD5_CTX
		STRUCTRESERVE	state,16,4
		STRUCTRESERVE	count,8,4
		STRUCTRESERVE	buffer,64,4
	ENDSTRUCT

	MACRO
	MAKEBIGEND	$reg
		and	r10,ip,$reg,ROR #8
		and	$reg,$reg,ip
		orr	$reg,r10,$reg,ROR #24
	MEND

	MACRO
	FF	$a,$b,$c,$d,$x,$s
		and	r3,$b,$c
		bic	r1,$d,$b
		orr	r3,r3,r1
		ldr	r1,[X,#$x*4]
		add	r3,r3,r1
		ldr	r1,[AC],#4
		add	r3,r3,r1
		add	$a,$a,r3
		add	$a,$b,$a,ROR #32-$s
	MEND

	MACRO
	GG	$a,$b,$c,$d,$x,$s
		and	r3,$b,$d
		bic	r1,$c,$d
		orr	r3,r3,r1
		ldr	r1,[X,#$x*4]
		add	r3,r3,r1
		ldr	r1,[AC],#4
		add	r3,r3,r1
		add	$a,$a,r3
		add	$a,$b,$a,ROR #32-$s
	MEND

	MACRO
	HH	$a,$b,$c,$d,$x,$s
		eor	r3,$b,$c
		eor	r3,r3,$d
		ldr	r1,[X,#$x*4]
		add	r3,r3,r1
		ldr	r1,[AC],#4
		add	r3,r3,r1
		add	$a,$a,r3
		add	$a,$b,$a,ROR #32-$s
	MEND

	MACRO
	II	$a,$b,$c,$d,$x,$s
		mvn	r3,$d
		orr	r3,r3,$b
		eor	r3,r3,$c
		ldr	r1,[X,#$x*4]
		add	r3,r3,r1
		ldr	r1,[AC],#4
		add	r3,r3,r1
		add	$a,$a,r3
		add	$a,$b,$a,ROR #32-$s
	MEND


		END

