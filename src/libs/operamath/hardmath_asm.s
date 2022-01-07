;------------------------------------------------------
;
; File:		hardmath.s
; By:		Stephen H. Landrum
; Last update:	11-Jun-93
;
; Math routines for the Opera
;
; Routines that access the matrix hardware
;
; Copyright (c) 1992, 1993, The 3DO Company, Inc.
; All rights reserved.
; This document is proprietary and confidential
;
;------------------------------------------------------
;
; Reminders:
;
; micro-optimization - cut down on addresses set up for LDR and STR instructions
;
;------------------------------------------------------


		INCLUDE	registers.i
		INCLUDE macros.i
		INCLUDE structs.i
		INCLUDE inthard.i


;------------------------------------------------------

		AREA	ASMCODE2, CODE

	IF :DEF:_SUPPORT_RED

;------------------------------------------------------
;
; int32 Dot3_F16 (vec3f16 v1, vec3f16 v2)
;
; Return the dot product of two vectors of 16.16 values
;

Dot3_F16	PROC

		STMFD	sp!, {r4-r6, lk}

		MOV	r4, r0
		MOV	r5, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	ip, #MADAM
		ADD	ip, ip, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r4, {r0-r2}
		STMIA	ip, {r0-r2}

		ADD	ip, ip, #(MATH_STACK+&40)-MATH_STACK		; B0-X
		LDMIA	r5!, {r0-r2}
		STMIA	ip, {r0-r2}
		ADD	r1, ip, #MATH_START-(MATH_STACK+&40)		; MATH_START
		MOV	r2, #1			; Red hardware only!!!!	; Do 3x3 multiply
		STR	r2, [r1]
		ADD	r5, r1, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r3, r5, #(MATH_STACK+&70)-MATH_STATUS		; B1-OUTX

00		  LDR	r0, [r5]
		  TST	r0, #1			; Red hardware only!!!! ; Check for multiply done
		 BNE	%00
		LDR	r6, [r3]

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

		MOV	r0, r6

99		LDMFD	sp!, {r4-r6, pc}	; RTS

	ENDIF	; _SUPPORT_RED


;------------------------------------------------------
;
; int32 gDot3_F16 (vec3f16 v1, vec3f16 v2)
;
; Return the dot product of two vectors of 16.16 values
;

gDot3_F16	PROC

		STMFD	sp!, {r4-r5, lk}

		MOV	r4, r0
		MOV	r5, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	ip, #MADAM
		ADD	ip, ip, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r4, {r0-r2}
		STMIA	ip, {r0-r2}

		ADD	ip, ip, #(MATH_STACK+&40)-MATH_STACK		; B0-X
		LDMIA	r5!, {r0-r2}
		STMIA	ip, {r0-r2}
		MOV	r2, #2			; Green hardware only!!	; Do 3x3 multiply
		STR	r2, [ip, #MATH_START-(MATH_STACK+&40)]

00		  LDR	r0, [ip, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	r0, #1			; Green hardware only!!	; Check for multiply done
		 BNE	%00
		MOV	r0, #0
		STR	r0, [ip, #MATH_START-(MATH_STACK+&40)]		; swap
		LDR	r4, [ip, #(MATH_STACK+&60)-(MATH_STACK+&40)]	; B0-OUTX

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

		MOV	r0, r4

99		LDMFD	sp!, {r4-r5, pc}	; RTS

	
	IF :DEF:_SUPPORT_RED

;------------------------------------------------------
;
; int32 Dot4_F16 (vec4f16 v1, vec4f16 v2)
;
; Return the dot product of two vectors of 16.16 values
;

Dot4_F16	PROC

		STMFD	sp!, {r4-r6, lk}

		MOV	r4, r0
		MOV	r5, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	ip, #MADAM
		ADD	ip, ip, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r4, {r0-r3}
		STMIA	ip, {r0-r3}

		ADD	ip, ip, #(MATH_STACK+&40)-MATH_STACK		; B0-X
		LDMIA	r5!, {r0-r3}
		STMIA	ip, {r0-r3}
		ADD	r1, ip, #MATH_START-(MATH_STACK+&40)		; MATH_START
		MOV	r2, #0			; Red hardware only!!!!	; Do 4x4 multiply
		STR	r2, [r1]
		ADD	r5, r1, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r3, r5, #(MATH_STACK+&70)-MATH_STATUS		; B1-OUTX

00		  LDR	r0, [r5]
		  TST	r0, #1			; Red hardware only!!!! ; Check for multiply done
		 BNE	%00
		LDR	r6, [r3]

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

		MOV	r0, r6

99		LDMFD	sp!, {r4-r6, pc}	; RTS

	ENDIF	; _SUPPORT_RED


;------------------------------------------------------
;
; int32 gDot4_F16 (vec4f16 v1, vec4f16 v2)
;
; Return the dot product of two vectors of 16.16 values
;

gDot4_F16	PROC

		STMFD	sp!, {r4-r6, lk}

		MOV	r4, r0
		MOV	r5, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	ip, #MADAM
		ADD	ip, ip, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r4, {r0-r3}
		STMIA	ip, {r0-r3}

		ADD	ip, ip, #(MATH_STACK+&40)-MATH_STACK		; B0-X
		LDMIA	r5!, {r0-r3}
		STMIA	ip, {r0-r3}
		ADD	r1, ip, #MATH_START-(MATH_STACK+&40)		; MATH_START
		MOV	r2, #1			; Green hardware only!!	; Do 4x4 multiply
		STR	r2, [r1]
		ADD	r5, r1, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r3, r5, #(MATH_STACK+&60)-MATH_STATUS		; B0-OUTX

00		  LDR	r0, [r5]
		  TST	r0, #1			; Green hardware only!! ; Check for multiply done
		 BNE	%00
		MOV	r0, #0
		STR	r0, [r1]		; swap
		LDR	r6, [r3]

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

		MOV	r0, r6

99		LDMFD	sp!, {r4-r6, pc}	; RTS


	IF	:DEF:_SUPPORT_RED

;------------------------------------------------------
;
; void Cross3_F16 (vec3f16 dest, vec3f16 v1, vec3f16 v2)
;
; Return the cross product of two vectors of 16.16 values
;

Cross3_F16	PROC

		STMFD	sp!, {r4-r9, lk}

		MOV	r4, r0
		MOV	r5, r2
		MOV	r6, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6, {r3,r6,r7}
		RSB	r0, r6, #0
		MOV	r1, #0
		RSB	r2, r7, #0
		MOV	r8, #0
		RSB	ip, r3, #0
		STMIA	r9!, {r1,r2,r6}
		ADD	r9, r9, #4
		STMIA	r9!, {r7,r8,ip}
		ADD	r9, r9, #4
		STMIA	r9!, {r0,r3,r8}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&2c)	; B0-X
		ADD	r6, r9, #MATH_START-(MATH_STACK+&40)		; MATH_START
		MOV	r1, #1			; Red hardware only!!!!	; Do 3x3 multiply
		LDMIA	r5!, {r2,r3,r8}		; load vector
		STMIA	r9, {r2,r3,r8}
		STR	r1, [r6]		; start multiply
		ADD	ip, r6, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r0, ip, #(MATH_STACK+&70)-MATH_STATUS		; B1-OUTX

00		  LDR	r2, [ip]
		  TST	r2, #1			; Red hardware only!!!! ; Check for multiply done
		 BNE	%00
		LDMIA	r0, {r2,r3,r8}
		STRT	r2, [r4],#4
		STRT	r3, [r4],#4
		STRT	r8, [r4],#4

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, pc}	; RTS

	ENDIF	; _SUPPORT_RED


;------------------------------------------------------
;
; void gCross3_F16 (vec3f16 dest, vec3f16 v1, vec3f16 v2)
;
; Return the cross product of two vectors of 16.16 values
;

gCross3_F16	PROC

		STMFD	sp!, {r4-r9, lk}

		MOV	r4, r0
		MOV	r5, r2
		MOV	r6, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6, {r3,r6,r7}
		RSB	r0, r6, #0
		MOV	r1, #0
		RSB	r2, r7, #0
		MOV	r8, #0
		RSB	ip, r3, #0
		STMIA	r9!, {r1,r2,r6}
		ADD	r9, r9, #4
		STMIA	r9!, {r7,r8,ip}
		ADD	r9, r9, #4
		STMIA	r9!, {r0,r3,r8}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&2c)	; B0-X
		ADD	r6, r9, #MATH_START-(MATH_STACK+&40)		; MATH_START
		MOV	r1, #2			; Green hardware only!!	; Do 3x3 multiply
		LDMIA	r5!, {r2,r3,r8}		; load vector
		STMIA	r9, {r2,r3,r8}
		STR	r1, [r6]		; start multiply
		ADD	ip, r6, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r0, ip, #(MATH_STACK+&60)-MATH_STATUS		; B0-OUTX

00		  LDR	r2, [ip]
		  TST	r2, #1			; Green hardware only!! ; Check for multiply done
		 BNE	%00
		MOV	r2, #0
		STR	r2, [r6]
		LDMIA	r0, {r2,r3,r8}
		STRT	r2, [r4],#4
		STRT	r3, [r4],#4
		STRT	r8, [r4],#4

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, pc}	; RTS


	IF	:DEF:_SUPPORT_RED

;------------------------------------------------------
;
; void MulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat)
;
; Multiply a 3x3 matrix of 16.16 values by a vector and return the vector result
;

MulVec3Mat33_F16	PROC

		STMFD	sp!, {r4-r9, lk}

		MOV	r4, r0
		MOV	r5, r1
		MOV	r6, r2

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6, {r0-r3,r6-r8,ip,lk}
		STMIA	r9!, {r0,r3,r8}
		ADD	r9, r9, #4
		STMIA	r9!, {r1,r6,ip}
		ADD	r9, r9, #4
		STMIA	r9!, {r2,r7,lk}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&2c)	; B0-X
		ADD	r6, r9, #MATH_START-(MATH_STACK+&40)		; MATH_START
		MOV	r1, #1			; Red hardware only!!!!	; Do 3x3 multiply

		LDMIA	r5!, {r2,r3,r8}		; load vector
		STMIA	r9, {r2,r3,r8}
		STR	r1, [r6]		; start multiply
		ADD	ip, r6, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r0, ip, #(MATH_STACK+&70)-MATH_STATUS		; B1-OUTX
00		  LDR	r2, [ip]
		  TST	r2, #1			; Red hardware only!!!! ; Check for multiply done
		 BNE	%00
		LDMIA	r0, {r2,r3,r8}
		STRT	r2, [r4],#4
		STRT	r3, [r4],#4
		STRT	r8, [r4],#4

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, pc}	; RTS

	ENDIF	; _SUPPORT_RED

	
;------------------------------------------------------
;
; void gMulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat)
;
; Multiply a 3x3 matrix of 16.16 values by a vector and return the vector result
;

gMulVec3Mat33_F16	PROC

		STMFD	sp!, {r4-r9, lk}

		MOV	r4, r0
		MOV	r5, r1
		MOV	r6, r2

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6, {r0-r3,r6-r8,ip,lk}
		STMIA	r9!, {r0,r3,r8}
		ADD	r9, r9, #4
		STMIA	r9!, {r1,r6,ip}
		STMIB	r9!, {r2,r7,lk}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&28)	; B0-X
		MOV	r1, #2			; Green hardware only!!	; Do 3x3 multiply

		LDMIA	r5!, {r2,r3,r8}		; load vector
		STMIA	r9, {r2,r3,r8}
		STR	r1, [r9, #MATH_START-(MATH_STACK+&40)]		; start multiply
		ADD	r0, r9, #(MATH_STACK+&60)-(MATH_STACK+&40)	; B0-OUTX
00		  LDR	r2, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	r2, #1			; Green hardware only!!	; Check for multiply done
		 BNE	%00
		MOV	r2, #0
		STR	r2, [r9, #MATH_START-(MATH_STACK+&40)]		; swap
		LDMIA	r0, {r2,r3,r8}
		STRT	r2, [r4],#4
		STRT	r3, [r4],#4
		STRT	r8, [r4],#4

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, pc}	; RTS


;------------------------------------------------------
;
; void gMulManyVec3Mat33DivZ_F16 (mmv3m33d *s)
;
; Multiply a 3x3 matrix of 16.16 values by multiple vectors, multiply x and y by n/z
; Return the result vectors {x*n/z, y*n/z, z}

gMulManyVec3Mat33DivZ_F16	PROC

		STMFD	sp!, {r4-r9, r11, lk}

		LDMIA	r0, {r4-r8}
		B	_mmv3m33d_entry

;------------------------------------------------------
;
; void gMulVec3Mat33DivZ_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, frac16 n)
;
; Multiply a 3x3 matrix of 16.16 values by a vector, multiply x and y by n/z
; Return the result vector {x*n/z, y*n/z, z}
;

gMulVec3Mat33DivZ_F16	PROC

		STMFD	sp!, {r4-r9, r11, lk}

		MOV	r4, r0
		MOV	r5, r1
		MOV	r6, r2
		MOV	r7, r3
		MOV	r8, #1

_mmv3m33d_entry
		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		STR	r7, [r9, #(MATH_STACK+&80)-MATH_STACK]		; initialize n
		MOV	r7, #0
		STR	r7, [r9, #(MATH_STACK+&84)-MATH_STACK]
		LDMIA	r6, {r0-r3,r6,r7,r11,ip,lk}
		STMIA	r9!, {r0,r3,r11}
		ADD	r9, r9, #4
		STMIA	r9!, {r1,r6,ip}
		STMIB	r9!, {r2,r7,lk}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&28)	; B0-X
		ADD	r0, r9, #(MATH_STACK+&60)-(MATH_STACK+&40)	; B0-OUTX
		MOV	r1, #3			; Green hardware only!!	; Do 3x3 multiply/divide
		MOV	r6, #0			; Green hardware only!! ; swap registers

10		  LDMIA	r5!, {r2,r3,ip}		; load vector
		  STMIA	r9, {r2,r3,ip}
		  STR	r1, [r9, #MATH_START-(MATH_STACK+&40)]		; start multiply
00		    LDR	r2, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		    TST	r2, #1			; Green hardware only!!	; Check for multiply done
		   BNE	%00
		  STR	r6, [r9, #MATH_START-(MATH_STACK+&40)]		; swap registers
		  LDMIA	r0, {r2,r3,ip}
		  CMP	ip, #&40000000		; check for hardware glitch
		  RSBCSS lk, ip, #&c0000000
		  BCS	%02
		  STRT	r2, [r4],#4
		  STRT	r3, [r4],#4
		  STRT	ip, [r4],#4
01		  SUBS	r8, r8, #1
		 BNE	%10

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, r11, pc}	; RTS

02		MOV	ip, #2
		STR	ip, [r9, #MATH_START-(MATH_STACK+&40)]		; 3x3 multiply
03		  LDR	r2, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	r2, #1			; Green hardware only!!	; Check for multiply done
		 BNE	%03
		STR	r6, [r9, #MATH_START-(MATH_STACK+&40)]		; swap registers
		STMFD	sp!, {r0,r1,r6,r8,r9}
		LDMIA	r0, {r6-r8}
		LDR	r0, [r9, #(MATH_STACK+&80)-(MATH_STACK+&40)]	; n
		MOV	r1, r8						; z
		LIBCALL	DivSF16
		MOV	r9, r0						; n/z
		MOV	r1, r6						; x
		LIBCALL	MulSF16
		MOV	r6, r0						; x*n/z
		MOV	r0, r9						; n/z
		MOV	r1, r7						; y
		LIBCALL	MulSF16
		STRT	r6, [r4],#4					; x*n/z
		STRT	r0, [r4],#4					; y*n/z
		STRT	r8, [r4],#4					; z
		LDMFD	sp!, {r0,r1,r6,r8,r9}
		B	%01


;------------------------------------------------------
;
; void aMulManyVec3Mat33DivZ_F16 (mmv3m33d *s)
;
; Multiply a 3x3 matrix of 16.16 values by multiple vectors, multiply x and y by n/z
; Return the result vectors {x*n/z, y*n/z, z}

aMulManyVec3Mat33DivZ_F16	PROC

		STMFD	sp!, {r4-r9, r11, lk}

		LDMIA	r0, {r4-r8}
		LDR	r0, MSPtr		; Lock math hardware access
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0			; If error then exit
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		STR	r7, [r9, #(MATH_STACK+&80)-MATH_STACK]		; initialize n
		MOV	r7, #0
		STR	r7, [r9, #(MATH_STACK+&84)-MATH_STACK]
		LDMIA	r6, {r0-r3,r6,r7,r11,ip,lk}
		STMIA	r9!, {r0,r3,r11}
		ADD	r9, r9, #4
		STMIA	r9!, {r1,r6,ip}
		STMIB	r9!, {r2,r7,lk}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&28)	; B0-X
		MOV	r1, #3		; Anvil/green	; Do 3x3 multiply/divide

		LDMIA	r5!, {r2,r3,ip}		; load vector
		STMIA	r9, {r2,r3,ip}		; place vector in hardware
		STR	r1, [r9, #MATH_START-(MATH_STACK+&40)]	; start multiply
		MOV	r6, #0		; Anvil/green	; swap registers
		ADD	r0, r9, #(MATH_STACK+&60)-(MATH_STACK+&40)	; B0-OUTX
10		  LDMIA	r5!, {r2,r3,ip}		; load next vector
		  STMIA	r9, {r2,r3,ip}
00		    LDR	r2, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		    TST	r2, #1			; Anvil/green	; Check multiply done
		   BNE	%00
		  STR	r1, [r9, #MATH_START-(MATH_STACK+&40)]	; start multiply

		  LDMIA	r0, {r2,r3,ip}		; get previous results
		  STRT	r2, [r4],#4		; save previous results
		  STRT	r3, [r4],#4
		  STRT	ip, [r4],#4
01		  SUBS	r8, r8, #1		; check for last vector
		 BNE	%10

11		  LDR	r2, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	r2, #1			; Anvil/green	; Check multiply done
		 BNE	%11

		STR	r6, [r9, #MATH_START-(MATH_STACK+&40)]	; swap registers
		LDMIA	r0, {r2,r3,ip}		; get final results
		STRT	r2, [r4],#4		; save final results
		STRT	r3, [r4],#4
		STRT	ip, [r4],#4
		
		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, r11, pc}	; RTS



;------------------------------------------------------
;
; void aMulVec3Mat33DivZ_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, frac16 n)
;
; Multiply a 3x3 matrix of 16.16 values by a vector, multiply x and y by n/z
; Return the result vector {x*n/z, y*n/z, z}
;

aMulVec3Mat33DivZ_F16	PROC

		STMFD	sp!, {r4-r9, r11, lk}

		MOV	r4, r0
		MOV	r5, r1
		MOV	r6, r2
		MOV	r7, r3

		LDR	r0, MSPtr		; Lock math hardware access
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0			; If error then exit
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		STR	r7, [r9, #(MATH_STACK+&80)-MATH_STACK]		; initialize n
		MOV	r7, #0
		STR	r7, [r9, #(MATH_STACK+&84)-MATH_STACK]
		LDMIA	r6, {r0-r3,r6,r7,r11,ip,lk}
		STMIA	r9!, {r0,r3,r11}
		ADD	r9, r9, #4
		STMIA	r9!, {r1,r6,ip}
		STMIB	r9!, {r2,r7,lk}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&28)	; B0-X
		MOV	r1, #3		; Anvil/green	; Do 3x3 multiply/divide

		LDMIA	r5!, {r2,r3,ip}		; load vector
		STMIA	r9, {r2,r3,ip}		; place vector in hardware
		STR	r1, [r9, #MATH_START-(MATH_STACK+&40)]	; start multiply
		MOV	r6, #0		; Anvil/green	; swap registers
		ADD	r0, r9, #(MATH_STACK+&60)-(MATH_STACK+&40)	; B0-OUTX

11		  LDR	r2, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	r2, #1			; Anvil/green	; Check multiply done
		 BNE	%11

		STR	r6, [r9, #MATH_START-(MATH_STACK+&40)]	; swap registers
		LDMIA	r0, {r2,r3,ip}		; get final results
		STRT	r2, [r4],#4		; save final results
		STRT	r3, [r4],#4
		STRT	ip, [r4],#4
		
		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, r11, pc}	; RTS


	
	
	IF	:DEF:_SUPPORT_RED

;------------------------------------------------------
;
; void MulMat33Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat)
;
; Multiply two 3x3 matrices of 16.16 values and return the result
;

MulMat33Mat33_F16	PROC

		MOV	r3, #3			; treat matrix as array of 3 vectors

;		B	MulManyVec3Mat33_F16	; and fall into the multiply many vector routine

;------------------------------------------------------
;
; void MulManyVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, int32 count)
;
; Multiply a 3x3 matrix by a number of vectors
;

MulManyVec3Mat33_F16	PROC

		SUBS	r3, r3, #1		; pre-decrement count and check for single multiply
		BEQ	MulVec3Mat33_F16

		STMFD	sp!, {r4-r9, r11, lk}

		MOV	r4, r0
		MOV	r5, r1
		MOV	r6, r2
		MOV	r7, r3

		LDR	r0, MSPtr		; lock access to math hardware
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0			; error getting lock, just exit
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6, {r0-r3,r6,r8,r11,ip,lk}
		STMIA	r9!, {r0,r3,r11}
		ADD	r9, r9, #4
		STMIA	r9!, {r1,r6,ip}
		ADD	r9, r9, #4
		STMIA	r9!, {r2,r8,lk}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&2c)	; B0-X
		ADD	r6, r9, #MATH_START-(MATH_STACK+&40)		; MATH_START
		ADD	ip, r6, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r0, ip, #(MATH_STACK+&70)-MATH_STATUS		; B1-OUTX
		MOV	r1, #1			; Red hardware only!!!!	; Do 3x3 multiply

		LDMIA	r5!, {r2,r3,r8}		; load vector
		STMIA	r9, {r2,r3,r8}		; place vector in hardware
		STR	r1, [r6]		; start multiply
01		  LDMIA	r5!, {r2,r3,r8}		; load next vector
00		    LDR	lk, [ip]
		    TST	lk, #1			; Red hardware only!!!! ; Check for multiply done
		   BNE	%00

		  STMIA	r9, {r2,r3,r8}		; place next vector in hardware
		  LDMIA	r0, {r2,r3,r8}		; get previous results
		  STR	r1, [r6]		; start multiply
		  STRT	r2, [r4],#4		; save results
		  STRT	r3, [r4],#4
		  STRT	r8, [r4],#4
		  SUBS	r7, r7, #1		; check for last vector
		 BNE	%01

10		  LDR	lk, [ip]
		  TST	lk, #1			; Red hardware only!!!! ; Check for multiply done
		 BNE	%10
		LDMIA	r0, {r2,r3,r8}		; get final results
		STRT	r2, [r4],#4		; save final results
		STRT	r3, [r4],#4
		STRT	r8, [r4],#4

		LDR	r0, MSPtr		; unlock hardware
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, r11, pc}	; RTS

	ENDIF	; _SUPPORT_RED

	
;------------------------------------------------------
;
; void gMulMat33Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat)
;
; Multiply two 3x3 matrices of 16.16 values and return the result
;

gMulMat33Mat33_F16	PROC

		MOV	r3, #3			; treat matrix as array of 3 vectors

;		B	gMulManyVec3Mat33_F16	; and fall into the multiply many vector routine

;------------------------------------------------------
;
; void gMulManyVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, int32 count)
;
; Multiply a 3x3 matrix by a number of vectors
;

gMulManyVec3Mat33_F16	PROC

		SUBS	r3, r3, #1		; pre-decrement count and check for single multiply
		BEQ	gMulVec3Mat33_F16

		STMFD	sp!, {r4-r9, r11, lk}

		MOV	r4, r0
		MOV	r5, r1
		MOV	r6, r2
		MOV	r7, r3

		LDR	r0, MSPtr		; lock access to math hardware
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0			; error getting lock, just exit
		BMI	%99

		MOV	r9, #MADAM
		ADD	r9, r9, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6, {r0-r3,r6,r8,r11,ip,lk}
		STMIA	r9!, {r0,r3,r11}
		ADD	r9, r9, #4
		STMIA	r9!, {r1,r6,ip}
		STMIB	r9!, {r2,r8,lk}

		ADD	r9, r9, #(MATH_STACK+&40)-(MATH_STACK+&28)	; B0-X
		MOV	r1, #2			; Green hardware only!!	; Do 3x3 multiply

		LDMIA	r5!, {r2,r3,r8}		; load vector
		STMIA	r9, {r2,r3,r8}		; place vector in hardware
		STR	r1, [r9, #MATH_START-(MATH_STACK+&40)]		; start multiply
		ADD	r0, r9, #(MATH_STACK+&60)-(MATH_STACK+&40)	; B0-OUTX
01		  LDMIA	r5!, {r2,r3,r8}		; load next vector
		  STMIA	r9, {r2,r3,r8}		; place next vector in hardware
00		    LDR	lk, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		    TST	lk, #1			; Green hardware only!! ; Check for multiply done
		   BNE	%00

		  STR	r1, [r9, #MATH_START-(MATH_STACK+&40)]		; start multiply
		  LDMIA	r0, {r2,r3,r8}		; get previous results
		  STRT	r2, [r4],#4		; save results
		  STRT	r3, [r4],#4
		  STRT	r8, [r4],#4
		  SUBS	r7, r7, #1		; check for last vector
		 BNE	%01

10		  LDR	lk, [r9, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	lk, #1			; Green hardware only!!	; Check for multiply done
		 BNE	%10
		MOV	lk, #0
		STR	lk, [r9, #MATH_START-(MATH_STACK+&40)]
		LDMIA	r0, {r2,r3,r8}		; get final results
		STRT	r2, [r4],#4		; save final results
		STRT	r3, [r4],#4
		STRT	r8, [r4],#4

		LDR	r0, MSPtr		; unlock hardware
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, r11, pc}	; RTS

	
	IF	:DEF:_SUPPORT_RED

;------------------------------------------------------
;
; void MulVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat)
;
; Multiply a 4x4 matrix of 16.16 values by a vector and return the vector result
;

MulVec4Mat44_F16	PROC

		STMFD	sp!, {r4-r9, lk}

		MOV	r4, r0		; dest
		MOV	r5, r1		; vec
		MOV	r6, r2		; mat

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	lk, #MADAM
		ADD	lk, lk, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6!, {r0-r3,r7-r9,ip}	; load 4x4 matrix
		STMIA	lk!, {r0,r7}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}
		SUB	lk, lk, #48
		LDMIA	r6!, {r0-r3,r7-r9,ip}
		STMIA	lk!, {r0,r7}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}

		LDMIA	r5!, {r0-r3}		; load vector
		STMIA	lk, {r0-r3}

		ADD	r6, lk, #MATH_START-(MATH_STACK+&40)		; MATH_START
		MOV	r9, #0			; Red hardware only!!!!	; Do 4x4 multiply
		STR	r9, [r6]		; start multiply
		ADD	r7, r6, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r8, r7, #(MATH_STACK+&70)-MATH_STATUS		; B1-OUTX
00		  LDR	r0, [r7]
		  TST	r0, #1			; Red hardware only!!!! ; Check for multiply done
		 BNE	%00
		LDMIA	r8, {r0-r3}
		STRT	r0, [r4],#4
		STRT	r1, [r4],#4
		STRT	r2, [r4],#4
		STRT	r3, [r4],#4

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, pc}	; RTS

	ENDIF	; _SUPPORT_RED

	
;------------------------------------------------------
;
; void gMulVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat)
;
; Multiply a 4x4 matrix of 16.16 values by a vector and return the vector result
;

gMulVec4Mat44_F16	PROC

		STMFD	sp!, {r4-r9, lk}

		MOV	r4, r0		; dest
		MOV	r5, r1		; vec
		MOV	r6, r2		; mat

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	lk, #MADAM
		ADD	lk, lk, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6!, {r0-r3,r7-r9,ip}	; load 4x4 matrix
		STMIA	lk!, {r0,r7}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}
		SUB	lk, lk, #48
		LDMIA	r6!, {r0-r3,r7-r9,ip}
		STMIA	lk!, {r0,r7}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}

		LDMIA	r5!, {r0-r3}		; load vector
		STMIA	lk, {r0-r3}

		MOV	r9, #1			; Green hardware only!!	; Do 4x4 multiply
		STR	r9, [lk, #MATH_START-(MATH_STACK+&40)]		; start multiply
		ADD	r8, lk, #(MATH_STACK+&60)-(MATH_STACK+&40)	; B0-OUTX
00		  LDR	r0, [lk, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	r0, #1			; Green hardware only!! ; Check for multiply done
		 BNE	%00
		MOV	r0, #0
		STR	r0, [lk, #MATH_START-(MATH_STACK+&40)]
		LDMIA	r8, {r0-r3}
		STRT	r0, [r4],#4
		STRT	r1, [r4],#4
		STRT	r2, [r4],#4
		STRT	r3, [r4],#4

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, pc}	; RTS


	IF	:DEF:_SUPPORT_RED

;------------------------------------------------------
;
; void MulMat44Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat)
;
; Multiply a 4x4 matrix of 16.16 values by a vector and return the vector result
;

MulMat44Mat44_F16	PROC

		MOV	r3, #4			; treat matrix as array of 4 vectors

;		B	MulManyVec4Mat44_F16	; and fall into multiple vector routine

;------------------------------------------------------
;
; void MulManyVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat, int32 count)
;
; Multiply a 4x4 matrix of 16.16 values by a vector and return the vector result
;

MulManyVec4Mat44_F16	PROC

		STMFD	sp!, {r4-r9, r11, lk}

		MOV	r4, r0		; dest
		MOV	r5, r1		; vec
		MOV	r6, r2		; mat
		MOV	r7, r3		; count

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	lk, #MADAM
		ADD	lk, lk, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6!, {r0-r3,r8,r9,r11,ip}	; load 4x4 matrix
		STMIA	lk!, {r0,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r11}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}
		SUB	lk, lk, #48
		LDMIA	r6!, {r0-r3,r8,r9,r11,ip}
		STMIA	lk!, {r0,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r11}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}

		ADD	r6, lk, #MATH_START-(MATH_STACK+&40)		; MATH_START
		ADD	ip, r6, #MATH_STATUS-MATH_START			; MATH_STATUS
		ADD	r8, ip, #(MATH_STACK+&70)-MATH_STATUS		; B1-OUTX
		MOV	r9, #0			; Red hardware only!!!!	; Do 4x4 multiply

01		  LDMIA	r5!, {r0-r3}		; load vector
		  STMIA	lk, {r0-r3}
		  STR	r9, [r6]		; start multiply
00		    LDR	r0, [ip]
		    TST	r0, #1			; Red hardware only!!!! ; Check for multiply done
		   BNE	%00
		  LDMIA	r8, {r0-r3}
		  STRT	r0, [r4],#4
		  STRT	r1, [r4],#4
		  STRT	r2, [r4],#4
		  STRT	r3, [r4],#4
		  SUBS	r7, r7, #1
		 BNE	%01

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, r11, pc}	; RTS

	ENDIF	; _SUPPORT_RED

	
;------------------------------------------------------
;
; void gMulMat44Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat)
;
; Multiply a 4x4 matrix of 16.16 values by a vector and return the vector result
;

gMulMat44Mat44_F16	PROC

		MOV	r3, #4			; treat matrix as array of 4 vectors

;		B	gMulManyVec4Mat44_F16	; and fall into multiple vector routine

;------------------------------------------------------
;
; void gMulManyVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat, int32 count)
;
; Multiply a 4x4 matrix of 16.16 values by a vector and return the vector result
;

gMulManyVec4Mat44_F16	PROC

		SUBS	r3, r3, #1	; predecrement count and check for single vector
		BEQ	gMulVec4Mat44_F16

		STMFD	sp!, {r4-r9, r11, lk}

		MOV	r4, r0		; dest
		MOV	r5, r1		; vec
		MOV	r6, r2		; mat
		MOV	r7, r3		; count

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	lk, #MADAM
		ADD	lk, lk, #MATH_STACK-MADAM			; MATRIX00
		LDMIA	r6!, {r0-r3,r8,r9,r11,ip}	; load 4x4 matrix
		STMIA	lk!, {r0,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r11}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}
		SUB	lk, lk, #48
		LDMIA	r6!, {r0-r3,r8,r9,r11,ip}
		STMIA	lk!, {r0,r8}
		ADD	lk, lk, #8
		STMIA	lk!, {r1,r9}
		ADD	lk, lk, #8
		STMIA	lk!, {r2,r11}
		ADD	lk, lk, #8
		STMIA	lk!, {r3,ip}

		ADD	r8, lk, #(MATH_STACK+&60)-(MATH_STACK+&40)	; B0-OUTX
		MOV	r9, #1			; Green hardware only!!	; Do 4x4 multiply

		LDMIA	r5!, {r0-r3}		; load vector
		STMIA	lk, {r0-r3}
		STR	r9, [lk, #MATH_START-(MATH_STACK+&40)]		; start multiply
01		  LDMIA	r5!, {r0-r3}		; load next vector
		  STMIA	lk, {r0-r3}
00		    LDR	r11, [lk, #MATH_STATUS-(MATH_STACK+&40)]
		    TST	r11, #1			; Green hardware only!! ; Check for multiply done
		   BNE	%00

		  STR	r9, [lk, #MATH_START-(MATH_STACK+&40)]		; start multiply
		  LDMIA	r8, {r0-r3}		; get previous results
		  STRT	r0, [r4],#4		; save results
		  STRT	r1, [r4],#4
		  STRT	r2, [r4],#4
		  STRT	r3, [r4],#4
		  SUBS	r7, r7, #1		; check for last vector
		 BNE	%01

10		  LDR	r11, [lk, #MATH_STATUS-(MATH_STACK+&40)]
		  TST	r11, #1			; Green hardware only!! ; Check for multiply done
		 BNE	%10
		MOV	r11, #0
		STR	r11, [lk, #MATH_START-(MATH_STACK+&40)]
		LDMIA	r8, {r0-r3}		; get last results
		STRT	r0, [r4],#4		; save results
		STRT	r1, [r4],#4
		STRT	r2, [r4],#4
		STRT	r3, [r4],#4

		LDR	r0, MSPtr		; unlock hardware
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4-r9, r11, pc}	; RTS


	
	IF	{FALSE}
;------------------------------------------------------
;
; int32 qdiv (int64 *n, int32 d);

qdiv		PROC

		STMFD	sp!, {r4, r5, lk}

		MOV	r4, r0
		MOV	r5, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	ip, #MADAM
		ADD	ip, ip, #(MATH_STACK+&80)-MADAM
		LDMIA	r4, {r0,r1}		; load n into hardware
		STMIA	ip, {r0,r1}
		STR	r5, [ip, #(MATH_STACK+&40)-(MATH_STACK+&80)]	; load d into hardware
		MOV	r0, #&3					; code for small divide
		STR	r0, [ip, #MATH_START-(MATH_STACK+&80)]	; trigger small divide
00		  LDR	r0, [ip, #MATH_STATUS-(MATH_STACK+&80)]	; wait for hardware to finish
		  TST	r0, #2			; Green hardware - test for selected process on
		 BNE	%00

		LDR	r4, [ip, #(MATH_STACK+&98)-(MATH_STACK+&80)]	; get result
;		MOV 	r4, #0

		LDR	r0, MSPtr		; unlock hardware
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

		MOV	r0, r4			; return result in r0

99		LDMFD	sp!, {r4, r5, pc}	; RTS


;------------------------------------------------------
;
; void doccb (struct ccbin *, struct ccbout *);

doccb		PROC

		STMFD	sp!, {r4, r5, lk}

		MOV	r4, r0
		MOV	r5, r1

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalLockSemaphore
		CMP	r0, #0
		BMI	%99

		MOV	ip, #MADAM
		ADD	ip, ip, #(MATH_STACK+&40)-MADAM
		ADD	lk, ip, #(MATH_STACK+&60)-(MATH_STACK+&40)
		LDMIA	r4!, {r0-r3}
		STMIA	ip!, {r0-r3}
		LDMIA	r4!, {r0-r3}
		STMIA	lk!, {r0-r3}
		LDMIA	r4!, {r0, r1}
		ADD	r2, lk, #(MATH_STACK+&80)-(MATH_STACK+&70)
		STMIA	r2, {r0, r1}
		ADD	ip, ip, #8
		MOV	r0, #&8
		STR	r0, [r2, #MATH_START-(MATH_STACK+&80)]
00		  LDR	r0, [r2, #MATH_STATUS-(MATH_STACK+&80)]
		  TST	r0, #2
		 BNE	%00
		LDMIA	ip, {r0, r1}
		STMIA	r5!, {r0, r1}
		LDMIA	lk, {r0-r3}
		STMIA	r5!, {r0-r3}

		LDR	r0, MSPtr
		LDR	r0, [r0]
		MOV	r1, #1
		LIBCALL	_SuperInternalUnlockSemaphore

99		LDMFD	sp!, {r4, r5, pc}	; RTS


;------------------------------------------------------
;
; int32 getnz (void)

getnz		PROC

		MOV	r0, #MADAM
		ADD	r0, r0, #MATH_STACK-MADAM
		LDR	r0, [r0, #(MATH_STACK+&98)-MATH_STACK]

		MOV	pc, lk			; RTS

;------------------------------------------------------
	ENDIF

		IMPORT	MathSemaphore

MSPtr		DCD	MathSemaphore

;------------------------------------------------------


		END

