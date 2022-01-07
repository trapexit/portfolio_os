; $Id: ecc32.s,v 1.2 1994/12/23 06:55:54 bungee Exp $

; $Log: ecc32.s,v $
; Revision 1.2  1994/12/23  06:55:54  bungee
; Fixed nasty bug in detectQErr:  we weren't saving r3 (qRow) and then setting
; r3 to the generator value (0x11D)...which is supplied to the S1GEN macro.
; Interestingly enough, due to the nature of this bug, we STILL would not
; unknowingly return bad sectors (because detectPErr worked properly).  But
; what this bug DID do was cause us to fail more often than we should have.
; And if there was a smudge or small scratch on the disc, (which we would
; expect that we should be able to correct), it would fail to correct it.
;
; Revision 1.1  1994/11/02  13:56:38  bungee
; Initial revision
;

; ecc32.s contains the support for software ecc for LCCD.
;
; currently implements a full pass of P parity, then Q, then P, and finally
; Q.  only performs subsequent passes of P and Q if any error (correctable or
; not) was found in the current pass.
;
; also, currently performs all manipulations on 32-bit words (instead of bytes)

	AREA	|ASMCODE|, CODE, READONLY
	
	INCLUDE	macros.i
	INCLUDE	registers.i
	INCLUDE	stack.i

sectorPcols	%	86		; (43*2) account for MSB/LSB byte planes
sectorQrows	%	52		; (26*2) account for MSB/LSB byte planes
	ALIGN
gf_log		%	256		; log table for divide (NOTE: These are bytes)
;gf_log		%	1024		; (256*4) log tables for multiply/divide (NOTE: These are 4-byte words)
;gf_exp		%	2048		; (512*4)twice as many for decode... (NOTE: These are 4-byte words)

qRow2Addr		DCD	0
		DCD	84
		DCD	172
		DCD	256
		DCD	344
		DCD	428
		DCD	516
		DCD	600
		DCD	688
		DCD	772
		DCD	860
		DCD	944
		DCD	1032
		DCD	1116
		DCD	1204
		DCD	1288
		DCD	1376
		DCD	1460
		DCD	1548
		DCD	1632
		DCD	1720
		DCD	1804
		DCD	1892
		DCD	1976
		DCD	2064
		DCD	2148

qRow2CorAddr	DCD	0
		DCD	86
		DCD	172
		DCD	258
		DCD	344
		DCD	430
		DCD	516
		DCD	602
		DCD	688
		DCD	774
		DCD	860
		DCD	946
		DCD	1032
		DCD	1118
		DCD	1204
		DCD	1290
		DCD	1376
		DCD	1462
		DCD	1548
		DCD	1634
		DCD	1720
		DCD	1806
		DCD	1892
		DCD	1978
		DCD	2064
		DCD	2150

gErrCount		DCD	0		; placeholder for total error count (return value)

kMultiErr		EQU	-1
kNoErr		EQU	0
kSingleErr	EQU	1

kGoodCell		EQU	0
kBadCell		EQU	1

kPchk		EQU	0x01
kQchk		EQU	0x02


	MACRO	
		MemSet	$ptr, $val, $len		; set 'len' bytes to 'val' starting at 'ptr'
01		SUBS	$len, $len, #1		; len--
		STRPLB	$val, [$ptr, $len]
		BGT	%01			; while (len > 0), loop
	MEND

	MACRO
		Mod	$dst, $src, $val
		MOV	$dst, $src
01		CMP	$dst, $val
		SUBGE	$dst, $dst, $val
		BGE	%01
	MEND

	MACRO
$label		MSALLDR	$dst, $src, $int, $index
$label		LDR	$dst, $src, #4
		MOV	$dst, $dst, LSL #16
		LDR	$int, $src, $index		; use $index instead of #84?
		ORR	$dst, $dst, $int, LSR #16
	MEND
		
	MACRO
$label		S1GEN	$dst, $gen, $reg
$label		MOV	$gen, #0			; clear "generator" value
		ADDS	$dst, $dst, $dst		; s1 <<= 1, set Carry if needed
		MOVCS	$gen, #0x1D000000		; if C=1, "generate" the MSB
		TST	$dst, #0x01000000		; if bit set
		EORNE	$gen, $gen, $reg, LSL #16	; ... generate mid-hi-byte
		TST	$dst, #0x00010000		; if bit set
		EORNE	$gen, $gen, $reg, LSL #8	; ... generate mid-lo-byte
		TST	$dst, #0x00000100		; if bit set
		EORNE	$gen, $gen, $reg		; ... generate LSB
		EOR	$dst, $dst, $gen		; "generate" s1 value(s)
	MEND

	MACRO
$label		MOV2	$dst, $msb, $lsb
$label		MOV	$dst, $msb
		ADD	$dst, $dst, $lsb
	MEND


InitECC		PROC
		PUSH	r0-r4			; save r0-r4

;	ADRL	r0, gf_exp		; start of galois exponent table
		ADRL	r1, gf_log		; start of galois log table

		MOV	r2, #1			; v index
		MOV	r3, #0			; i index
		MOV2	r4, #0x0100, #0x001D
	
GFloop
;	STR	r2, [r0, r3, LSL #2]	; gf_exp[i] = v        (r3 LSL 2, because it's a table of Words)
	
;	CMP	r3, #255			; if (i < 255)	
;	STRLT	r3, [r1, r2, LSL #2]	; ... gf_log[v] = i    (r2 LSL 2, because it's a table of Words)
		STRB	r3, [r1, r2]		; gf_log[v] = i    (table of bytes)
		
		ADD	r2, r2, r2		; v <<= 1
		TST	r2, #0x100		; if (v & 0x100)
		EORNE	r2, r2, r4		; ... v ^= 0x11D
		
		ADD	r3, r3, #1		; i++
;	CMP	r3, #512			; if (i < 512)
		CMP	r3, #255			; if (i < 255)
		BLT	GFloop			; ... goto loop
;	BNE	GFloop			; ... goto loop
		
		POP	r0-r4			; restore r0-r4
		MOV	pc, lr			; return


; r0 : s0
; r1 : s1
; r2 : buf
; r3 : pCol
; r4-r11 : row values
; r12 : generator value

detectPErr
		PUSH	"r1-r4,lr"		; save r1-r4,lr
	
		MOV	r2, r0			; save buf
		MOV	r3, r1			; save pCol (NOTE:  pCol is byte col#, NOT word col#)
		MOV	r0, #0			; s0 = 0
		MOV	r1, #0			; s1 = 0
		PUSH	r2			; save untouched buf on stack
		ADD	r2, r2, r3		; bumpedBuf = buf + pCol (adjust for BYTE column number)
		PUSH	r3			; save pCol for later use
		MOV2	r3, #0x0100, #0x001D	; r3 = voodoo generator value
	
; rows 0 thru 7
		LDR	r4, [r2], #84		; r4 = row0 = bumpedBuf + 86*0
		MSALLDR	r5, [r2], r12, #84		; r5 = row1 =((bumpedBuf + (86*1)-2) << 16) | ((bumpedBuf + (86*1)+2) >> 16)
		LDR	r6, [r2], #84		; r6 = row2 = bumpedBuf + 86*2
		MSALLDR	r7, [r2], r12, #84		; r7 = row3 =((bumpedBuf + (86*3)-2) << 16) | ((bumpedBuf + (86*3)+2) >> 16)
		LDR	r8, [r2], #84		; r8 = row4 = bumpedBuf + 86*4
		MSALLDR	r9, [r2], r12, #84		; r9 = row5 = ((bumpedBuf + (86*5)-2) << 16) | ((bumpedBuf + (86*5)+2) >> 16)
		LDR	r10, [r2], #84		; r10 = row6 = bumpedBuf + 86*6
		MSALLDR	r11, [r2], r12, #84		; r11 = row7 = ((bumpedBuf + (86*7)-2) << 16) | ((bumpedBuf + (86*7)+2) >> 16)

		EOR	r0, r0, r4
		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r4 ^ r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

;		S1GEN	r1, r12, r3		; not needed before first EOR (because s1=0 before & after S1GEN)
		EOR	r1, r1, r4
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; rows 8 thru 15
		LDR	r4, [r2], #84		; r4 = row1 = bumpedBuf + 86*0
		MSALLDR	r5, [r2], r12, #84		; r5 = row2 =((bumpedBuf + (86*1)-2) << 16) | ((bumpedBuf + (86*1)+2) >> 16)
		LDR	r6, [r2], #84		; r6 = row3 = bumpedBuf + 86*2
		MSALLDR	r7, [r2], r12, #84		; r7 = row4 =((bumpedBuf + (86*3)-2) << 16) | ((bumpedBuf + (86*3)+2) >> 16)
		LDR	r8, [r2], #84		; r8 = row5 = bumpedBuf + 86*4
		MSALLDR	r9, [r2], r12, #84		; r9 = row6 = ((bumpedBuf + (86*5)-2) << 16) | ((bumpedBuf + (86*5)+2) >> 16)
		LDR	r10, [r2], #84		; r10 = row7 = bumpedBuf + 86*6
		MSALLDR	r11, [r2], r12, #84		; r11 = row8 = ((bumpedBuf + (86*7)-2) << 16) | ((bumpedBuf + (86*7)+2) >> 16)

		EOR	r0, r0, r4
		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r4 ^ r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r4
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; rows 16 thru 23
		LDR	r4, [r2], #84		; r4 = row1 = bumpedBuf + 86*0
		MSALLDR	r5, [r2], r12, #84		; r5 = row2 =((bumpedBuf + (86*1)-2) << 16) | ((bumpedBuf + (86*1)+2) >> 16)
		LDR	r6, [r2], #84		; r6 = row3 = bumpedBuf + 86*2
		MSALLDR	r7, [r2], r12, #84		; r7 = row4 =((bumpedBuf + (86*3)-2) << 16) | ((bumpedBuf + (86*3)+2) >> 16)
		LDR	r8, [r2], #84		; r8 = row5 = bumpedBuf + 86*4
		MSALLDR	r9, [r2], r12, #84		; r9 = row6 = ((bumpedBuf + (86*5)-2) << 16) | ((bumpedBuf + (86*5)+2) >> 16)
		LDR	r10, [r2], #84		; r10 = row7 = bumpedBuf + 86*6
		MSALLDR	r11, [r2], r12, #84		; r11 = row8 = ((bumpedBuf + (86*7)-2) << 16) | ((bumpedBuf + (86*7)+2) >> 16)

		EOR	r0, r0, r4
		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r4 ^ r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r4
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; rows 24 thru 25
		LDR	r4, [r2], #84		; r4 = row1 = bumpedBuf + 86*0
		MSALLDR	r5, [r2], r6, #84		; r5 = row2 =((bumpedBuf + (86*1)-2) << 16) | ((bumpedBuf + (86*1)+2) >> 16)

		EOR	r0, r0, r4
		EOR	r0, r0, r5		; s0 ^= (r4 ^ r5)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r4
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r5


; r0 : s0 (4 bytes)
; r1 : s1 (4 bytes)
; r2 : bumpedBuf
; r3 : pCol (or qRow)
; r4 : gf_log
; r10: indicates whether we're correcting a P byte or a Q byte
; r11: gErrCount
; r12: return value for detectPErr
; done with r5-r10

		POP	r3			; restore pCol from stack

		CMP	r3, #84			; if (pCol == 84)
		BICEQ	r0, r0, #0x000000FF
		BICEQ	r0, r0, #0x0000FF00		; ... only pay attention to the hi 16 bits of s0
		BICEQ	r1, r1, #0x000000FF
		BICEQ	r1, r1, #0x0000FF00		; ... only pay attention to the hi 16 bits of s1
		
		MOV	r10, #kPchk		; indicate where performing P correction
		
QChkBytes		ORRS	r5, r0, r1		; if (!(s0 | s1))		<sets CPSR>
		MOVEQ	r0, #kNoErr		; ... indicate kNoErr
		LDMEQFD	sp!, {r2}			; ... remove unused buf from stack
		LDMEQFD	sp!, {r1-r4,lr}		; ... restore r1-r4,lr
		MOVEQ	pc, lr			; ... return

; set up regs needed during all byte lanes

		ADRL	r4, gErrCount
		LDR	r11, [r4]			; r11 = gErrCount
; yes, commented	ADRL	r4, gf_log		; start of galois log table
		MOV	r12, #0			; clear r12 (response from detectPErr)
		
; scan s0,s1 [31:24] for error		

Pbyte3		TST	r5, #0xFF000000		; err in 1st byte col?
		ORRNE	r12, r12, #0x01000000	; ... yes
		TSTNE	r1, #0xFF000000		; if so, is the associated s1 zero?
		TSTNE	r0, #0xFF000000		; if so, is the associated s0 zero?

		MOVNE	r6, r0, LSR #24		; get s0' byte, r5[7:0] = r0[31:24]
		MOVNE	r7, r1, LSR #24		; get s1' byte
		MOVNE	r8, #00			; indicate error is in even byte lane

		BLNE	correctPbyte
		
; scan s0,s1 [23:16] for error		

Pbyte2		CMP	r10, #kPchk
		ADDEQ	r3, r3, #1		; adjust for next pCol (byte lane 2)

		TST	r5, #0x00FF0000		; err in 2nd byte col?
		ORRNE	r12, r12, #0x00010000	; ... yes
		TSTNE	r1, #0x00FF0000		; if so, is the associated s1 zero?
		TSTNE	r0, #0x00FF0000		; if so, is the associated s0 zero?
;		BEQ	Pbyte1			; ... no (noErr OR multErr), goto next byte lane

		MOVNE	r6, r0, LSR #16		; get s0 byte, r5[7:0] = r0[23:16]
		MOVNE	r7, r1, LSR #16		; get s1 byte
		ANDNE	r6, r6, #0xFF
		ANDNE	r7, r7, #0xFF
		MOVNE	r8, #01			; indicate error is in odd byte lane

		BLNE	correctPbyte
		
; scan s0,s1 [15:8] for error		

Pbyte1		CMP	r10, #kPchk		; P or Q?
		ADDEQ	r3, r3, #1		; P... adjust for next pCol (byte lane 2)
		SUBNE	r3, r3, #1		; Q... r3 = qRow - 1
		MOVS	r3, r3			; set CPSR
		ADDMI	r3, r3, #26		; rap around to #25, if necessary

		TST	r5, #0x0000FF00		; err in 3rd byte col?
		ORRNE	r12, r12, #0x00000100	; ... yes
		TSTNE	r1, #0x0000FF00		; if so, is the associated s1 zero?
		TSTNE	r0, #0x0000FF00		; if so, is the associated s0 zero?
;		BEQ	Pbyte0			; ... no (noErr OR multErr), goto next byte lane

		MOVNE	r6, r0, LSR #8		; get s0 byte, r5[7:0] = r0[15:8]
		MOVNE	r7, r1, LSR #8		; get s1 byte
		ANDNE	r6, r6, #0xFF
		ANDNE	r7, r7, #0xFF
		MOVNE	r8, #00			; indicate error is in even byte lane

		BLNE	correctPbyte

; scan s0,s1 [7:0] for error		

Pbyte0		CMP	r10, #kPchk		; if (performing P)
		ADDEQ	r3, r3, #1		; ... adjust for next pCol (byte lane 2)

		TST	r5, #0x000000FF		; err in 4th byte col?
		ORRNE	r12, r12, #0x00000001	; ... yes
		TSTNE	r1, #0x000000FF		; if so, is the associated s1 zero?
		TSTNE	r0, #0x000000FF		; if so, is the associated s0 zero?
;		BEQ	Pdone			; ... no (noErr OR multErr), goto next byte lane

		ANDNE	r6, r0, #0xFF		; get s0 byte, r5[7:0] = r0[7:0]
		ANDNE	r7, r1, #0xFF		; get s1 byte
		MOVNE	r8, #01			; indicate error is in odd byte lane

		BLNE	correctPbyte

		ADRL	r4, gErrCount
		STR	r11, [r4]			; save updated gErrCount
		MOV	r0, r12			; set return value
		POP	r2
		POP	"r1-r4,lr"		; restore r1-r4,lr
		MOV	pc, lr			; return


correctPbyte
		PUSH	r8			; save byte lane indicator
		
		ADRL	r4, gf_log		; start of galois log table
		LDRB	r8, [r4, r7]		; logA = gf_log[s1]     (gf_log is table of BYTEs)
		LDRB	r9, [r4, r6]		; logB = gf_log[s0]
		SUBS	r7, r8, r9		; s1' = logA - logB		<sets CPSR>
		ADDMI	r7, r7, #255		; if (logA < logB) then s1 += 255

		CMP	r10, #kPchk		; are we doing P or Q correction?
		RSBEQ	r7, r7, #25		; P... s1' = 25 - s1'	(adjust for offset into pCol)
		RSBNE	r7, r7, #44		; Q... s1' = 44 - s1'	(adjust for offset into qRow)
		MOVS	r7, r7			; set CPSR
		LDMMIFD	sp!, {r8}			; remove saved byte from stack
		MOVMI	pc, lr			; return (index out of range)
;		BMI	Pbyte2			; if (s1 > 25), then multiple errors...goto next byte lane
		
; done with r7, r8

; don't mark the other rows/cols since we're sharing the code between P and Q correction
;		SUBS	r7, r6, r3		; chkQrow = s1' - pCol
;		ADDMI	r7, r7, #26		; if (chkQrow < 0), then chkQrow += 26
;		ADRL	r8, sectorQrows
;		AND	r9, r3, #0x01		; bytePlane = pCol & 0x01 (even or odd?)
;		ADD	r8, r8, r9		; sectorQRows[???][bytePlane]
;		MOV	r9, #kBadCell
;		STRB	r9, [r8, r7, LSL #1]	; sectorQRows[chkQrow][bytePlane] = kBadCell

; done with r7-r9

		POP	r8			; remove byte lane indicator from stack
		LDR	r2, [sp]			; restore untouched 'buf'
		
		CMP	r10, #kPchk		; are we doing P or Q correction?
		BNE	Qcor

Pcor		MOV	r8, #86			; P... r8 = stride value
		MLA	r9, r8, r7, r3		; P... addr = r8*r7 + r3 = 86*s1' + pCol

		ADD	r9, r9, r2		; addr = addr + buf

		LDRB	r7, [r9]
		EOR	r7, r7, r6
		STRB	r7, [r9]			; *addr ^= s0'

		ADD	r11, r11, #1		; gErrCount++
		MOV	pc,lr			; return

Qcor		CMP	r7, #43			; if (s1' >= 43)	(then it's in the Q-parity columns)
		ADDGE	r4, r8, r3, LSL #1		; ... r4 = byteplane + 2*qRow
		MOVGE	r8, #52			; ... r8 = 26*2 (multiple by two because these col#'s are in 16-bit words)
		MLAGE	r9, r7, r8, r4		; ... r9 = r7*(26*2) + (2*qRow + bytelane)
		MOVGE	r8, #0x0900
		ADDGE	r8, r8, #0x24		; r8 = 2340 (just make r8 larger than it will ever get, so that the Mod below doesn't affect r9)

		ADRLTL	r4, qRow2CorAddr		; Q... convert qRow to sector offset
		LDRLT	r4, [r4, r3, LSL #2]	; Q... starting offset for this qRow
		ADDLT	r4, r4, r8		; Q... BL3/2: r4 = qRow; BL1/0: r4 = qRow-1
		MOVLT	r8, #88
		MLALT	r9, r8, r7, r4		; addr = r8*r7 + r4 = (88*s1' + bytelane) + 86*qRow
		MOVLT	r8, #0x0800
		ADDLT	r8, r8, #0xBC		; r8 = 2236
		Mod	r9, r9, r8		; addr = addr MOD 2236	(NOTE: it won't hurt to do this on the P offset)
		ADD	r9, r9, r2		; addr = addr + buf

		LDRB	r7, [r9]
		EOR	r7, r7, r6
		STRB	r7, [r9]			; *addr ^= s0'

		ADD	r11, r11, #1		; gErrCount++
		
		MOV	pc,lr			; return




detectQErr
		PUSH	"r1-r4,lr"		; save r1-r4,lr
	
		MOV	r2, r0			; save buf
		MOV	r3, r1			; save qRow
		MOV	r0, #0			; s0 = 0
		MOV	r1, #0			; s1 = 0
		PUSH	r2			; save untouched buf on stack
		ADRL	r4, qRow2Addr
		LDR	r6, [r4, r3, LSL #2]	; starting offset for this qRow
		MOV2	r12, #0x0800, #0x00BC	; r12 = mod value = 2236
		PUSH	r3			; save qRow for later use
		MOV2	r3, #0x0100, #0x001D	; r3 = voodoo generator value
		
		SUBS	r5, r6, #88		; offset of first word in 'pair' row (qRow - 1)
		ADDMI	r5, r5, r12		; if (neg), offset += 2236
		LDR	r0, [r2, r5]		; pre-load s0 [15:0] with LSW of previous qRow
		BIC	r0, r0, #0x00FF0000
		BIC	r0, r0, #0xFF000000
		MOV	r1, r0			; pre-load s1 [15:0]		

		ADD	r4, r2, r12		; r4 = buf + modval
		ADD	r2, r2, r6		; bumpedBuf = buf + offset
	
; columns 0 thru 12 (13)
		LDR	r5, [r2], #88		; r5[31:16] = r(x)c0[31:16], r5[15:0] = r(X-1)c2[15:0]
		CMP	r2, r4			; if (addr >= (buf+2236))
		SUBGE	r2, r2, r12		; ... addr -= 2236
		LDR	r6, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r7, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r8, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r9, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r10, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r11, [r2], #88		; r11[31:16] = r(x)c12[31:16], r11[15:0] = r(x-1)c14[15:0]
		CMP	r2, r4
		SUBGE	r2, r2, r12

		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3		; "generate" pre-loaded s1 value
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; columns 14 thru 26 (27)
		MOV2	r12, #0x0800, #0x00BC	; r2 = mod value = 2236

		LDR	r5, [r2], #88		; r5[31:16] = r(x)c14[31:16], r5[15:0] = r(X-1)c16[15:0]
		CMP	r2, r4			; if (addr >= 2236)
		SUBGE	r2, r2, r12		; ... addr -= 2236
		LDR	r6, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r7, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r8, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r9, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r10, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r11, [r2], #88		; r11[31:16] = r(x)c26[31:16], r11[15:0] = r(x-1)c28[15:0]
		CMP	r2, r4
		SUBGE	r2, r2, r12

		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; columns 28 thru 40
		MOV2	r12, #0x0800, #0x00BC	; r12 = mod value = 2236

		LDR	r5, [r2], #88		; r5[31:16] = r(x)c28[31:16], r5[15:0] = r(X-1)c30[15:0]
		CMP	r2, r4			; if (addr >= 2236)
		SUBGE	r2, r2, r12		; ... addr -= 2236
		LDR	r6, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r7, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r8, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r9, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r10, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r11, [r2], #88		; r11[31:16] = r(x)c40[31:16], r11[15:0] = r(x-1)c42[15:0]
		CMP	r2, r4
		SUBGE	r2, r2, r12

		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; columns 42 thru 54
		MOV2	r12, #0x0800, #0x00BC	; r12 = mod value = 2236

		LDR	r5, [r2], #88		; r5[31:16] = r(x)c42[31:16], r5[15:0] = r(X-1)c44[15:0]
		CMP	r2, r4			; if (addr >= 2236)
		SUBGE	r2, r2, r12		; ... addr -= 2236
		LDR	r6, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r7, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r8, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r9, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r10, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r11, [r2], #88		; r11[31:16] = r(x)c54[31:16], r11[15:0] = r(x-1)c56[15:0]
		CMP	r2, r4
		SUBGE	r2, r2, r12

		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; columns 56 thru 68
		MOV2	r12, #0x0800, #0x00BC	; r12 = mod value = 2236

		LDR	r5, [r2], #88		; r5[31:16] = r(x)c56[31:16], r5[15:0] = r(X-1)c58[15:0]
		CMP	r2, r4			; if (addr >= 2236)
		SUBGE	r2, r2, r12		; ... addr -= 2236
		LDR	r6, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r7, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r8, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r9, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r10, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r11, [r2], #88		; r11[31:16] = r(x)c68[31:16], r11[15:0] = r(x-1)c70[15:0]
		CMP	r2, r4
		SUBGE	r2, r2, r12

		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; columns 70 thru 82
		MOV2	r12, #0x0800, #0x00BC	; r12 = mod value = 2236

		LDR	r5, [r2], #88		; r5[31:16] = r(x)c70[31:16], r5[15:0] = r(X-1)c72[15:0]
		CMP	r2, r4			; if (addr >= 2236)
		SUBGE	r2, r2, r12		; ... addr -= 2236
		LDR	r6, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r7, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r8, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r9, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r10, [r2], #88
		CMP	r2, r4
		SUBGE	r2, r2, r12
		LDR	r11, [r2], #88		; r11[31:16] = r(x)c82[31:16], r11[15:0] = r(x-1)c84[15:0]
		CMP	r2, r4
		SUBGE	r2, r2, r12

		EOR	r0, r0, r5
		EOR	r0, r0, r6
		EOR	r0, r0, r7
		EOR	r0, r0, r8
		EOR	r0, r0, r9
		EOR	r0, r0, r10
		EOR	r0, r0, r11		; s0 ^= (r5 ^ r6 ^ r7 ^ r8 ^ r9 ^ r10 ^ r11)

		S1GEN	r1, r12, r3
		EOR	r1, r1, r5
		S1GEN	r1, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r7
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8
		S1GEN	r1, r12, r3
		EOR	r1, r1, r9
		S1GEN	r1, r12, r3
		EOR	r1, r1, r10
		S1GEN	r1, r12, r3
		EOR	r1, r1, r11

; columns 84 thru 88
		; this process re-alligns the word halves
		LDR	r5, [r2]			; r11[31:16] = r(x)c84[31:16], r11[15:0] = unneeded data
		BIC	r5, r5, #0x000000FF
		BIC	r5, r5, #0x0000FF00		; only affect [31:16] of s0,s1
		EOR	r0, r0, r5		; s0 ^= r5
		S1GEN	r1, r12, r3
		EOR	r1, r1, r5		; s1 ^= r5
		MOV	r5, r1			; copy s1 to work on high 16 bits
		S1GEN	r5, r12, r3		; s1 <<= 1, if (s1 & gen) then s1 ^= gen
		BIC	r1, r1, #0x00FF0000
		BIC	r1, r1, #0xFF000000		; clear hi 16 bits of s1
		BIC	r5, r5, #0x000000FF
		BIC	r5, r5, #0x0000FF00		; clear lo 16 bits of s1'
		ORR	r1, r1, r5		; merge affected hi word of s1' with lo word of s1

		LDR	r5, [sp]		; grab a copy of qRow off the stack

		; remember...r4 contains buf+2236 at this point
		LDR	r6, [r4, r5, LSL #1]!	; r6 = *(buf + 2236 + 2*qRow), r5 = qRow	(row X, cols 86-87)
		
		CMP	r5, #0			; qRow == 0?
		LDREQ	r7, [r4, #48]		; (row X-1, cols 86-87)
		LDRNE	r7, [r4, #-4]
		
		LDR	r8, [r4, #52]!		; r8 = *(buf + 2236 + 2*qRow + 52), r4 += 52	(row X, cols 88-89)
		
		LDREQ	r9, [r4, #48]		; if (qRow == 0)...		(row X-1, cols 88-89)
		LDRNE	r9, [r4, #-4]

		BIC	r6, r6, #0x000000FF
		BIC	r6, r6, #0x0000FF00
		BIC	r7, r7, #0x00FF0000
		BIC	r7, r7, #0xFF000000
		ORR	r6, r6, r7		; r6 = r6[31:16] | r7[15:0]
		
		BIC	r8, r8, #0x000000FF
		BIC	r8, r8, #0x0000FF00
		BIC	r9, r9, #0x00FF0000
		BIC	r9, r9, #0xFF000000
		ORR	r8, r8, r9		; r8 = r8[31:16] | r9[15:0]

		EOR	r0, r0, r6
		EOR	r0, r0, r8		; s0 ^= (r6 ^ r8)
		
		; no S1GEN needed here (done above when re-alligning the word halves)
		EOR	r1, r1, r6
		S1GEN	r1, r12, r3
		EOR	r1, r1, r8

; at this point we now have for s0 and s1 values in r0, r1

		POP	r3			; restore qRow from stack

; r0 : s0 (4 bytes)
; r1 : s1 (4 bytes)
; r2 : bumpedBuf
; r3 : qRow
; r4 : gf_log
; r10: indicates whether we're correcting a P byte or a Q byte
; r11: gErrCount
; r12: return value for detectPErr
; done with r5-r10
		MOV	r10, #kQchk		; indicate where performing Q correction

		B	QChkBytes			; assuming we do NOT mark the rows/cols of the OTHER
						; type, this code is the same for P and Q


SectorECC	PROC
		PUSH	"r1-r12,lr"
		
		MOV	r2, r0			; save copy of buf
		MOV	r4, #0			; clear indicator bits
		ADRL	r11, gErrCount
		STR	r4, [r11]			; initialize gErrCount to zero

; detectPErr
;    input:  r0 - buf
;            r1 - pCol
;   output:  r0 - bytelane bits indicating error(s) occured.

Pchk		MOV	r0, r2			; restore copy of buf
		MOV	r1, #0			; pCol initially zero
		MOV	r3, #0			; clear error bits

		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 4
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 8
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 12
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 16
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 20
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 24
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 28
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 32
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 36
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 40
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 44
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 48
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 52
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 56
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 60
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 64
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 68
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 72
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 76
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 80
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check
		ADD	r1, r1, #4		; pCol = 84
		MOV	r0, r2
		BL	detectPErr
		ORR	r3, r3, r0		; incorporate results from P check

		ADD	r4, r4, #0x010000		; indicate that we just made a pass of P

		CMP	r3, #0			; if (no [more] errors found)
		MOVEQ	r0, r4 			; ... set return value (P/Q passes performed counts)
		ADREQL	r5, gErrCount		; ... get updated error count
		LDREQ	r6, [r5]
		ADDEQ	r0, r0, r6		; ... incorporate error count into return value
		LDMEQFD	sp!, {r1-r12,lr}		; ... pop the regs off the stack
		MOVEQ	pc, lr			; ... and return
						; otherwise, make a pass of Q to verify no more errors
		
; detectQErr
;    input:  r0 - buf
;            r1 - qRow
;   output:  r0 - bytelane bits indicating error(s) occured.

Qchk		MOV	r0, r2			; restore buf
		MOV	r1, #0			; qRow = 0 (and 25)
		MOV	r3, #0			; reset err bits
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 2 (and 1)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 4 (and 3)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 6 (and 5)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 8 (and 7)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 10 (and 9)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 12 (and 11)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 14 (and 13)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 16 (and 15)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 18 (and 17)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 20 (and 19)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 22 (and 21)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check
		ADD	r1, r1, #2		; qRow = 24 (and 23)
		MOV	r0, r2
		BL	detectQErr
		ORR	r3, r3, r0		; incorporate results from Q check

		ADD	r4, r4, #0x100000		; indicate that we just made a pass of Q

		CMP	r3, #0			; if (no [more] errors found)
		MOVEQ	r0, r4 			; ... set return value (P/Q passes performed counts)
		ADREQL	r5, gErrCount		; ... get updated error count
		LDREQ	r6, [r5]
		ADDEQ	r0, r0, r6		; ... incorporate error count into return value
		LDMEQFD	sp!, {r1-r12,lr}		; ... pop the regs off the stack
		MOVEQ	pc, lr			; ... return
		
		TST	r4, #0x200000		; if (this our second pass of Q)

		MOVNE	r0, r4 			; ... set return value (P/Q passes performed counts)
		ADRNEL	r5, gErrCount		; ... get updated error count
		LDRNE	r6, [r5]
		ADDNE	r0, r0, r6		; ... incorporate error count into return value
		ORRNE	r0, r0, #0x80000000		; ... indicate ECC failed
		LDMNEFD	sp!, {r1-r12,lr}		; ... pop the regs off the stack
		MOVNE	pc, lr			; ... return

		B	Pchk			; otherwise, make another pass of P to verify no errors
		
		END
