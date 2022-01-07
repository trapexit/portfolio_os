	;; $Id: rangerand.s,v 1.2 1994/02/09 01:27:17 limes Exp $


	AREA	|ASMCODE|, CODE, READONLY

	GET	structs.i
	GET	registers.i

; uint32 ScaledRand (uint32 scale)
;
; returns a random number in the range [0..scale-1]
; will return 0 if scale is 0
;
; original code by : Steve Landrum
;

	IMPORT	urand

	EXPORT	ScaledRand
ScaledRand

	STMFD	sp!, {r0, lk}	; save off scale

	BL	urand	; get random number (assuming all 32 bits return)
			; need to scale return if only 31 bits return
			; Dale -can we remove the AND statement from rand()?

	LDMFD	sp!, {r1, lk}	; restore link register and scale

	MOV	r2, r0, LSR #16	; split multiplicands into 16 bit parts
	MOV	r3, r1, LSR #16
	BIC	r0, r0, r2, LSL #16
	BIC	r1, r1, r3, LSL #16

	MUL	ip, r0, r1	; multiply all of the pieces
	MUL	r1, r2, r1
	MUL	r2, r3, r2
	MUL	r3, r0, r3

	ADDS	ip, ip, r3, LSL #16	; add the pieces together
	ADC	r0, r2, r3, LSR #16
	ADDS	ip, ip, r1, LSL #16
	ADC	r0, r0, r1, LSR #16

	MOV	pc, lk		; RTS	; return high word of product


	END
