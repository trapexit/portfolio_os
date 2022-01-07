	;; $Id: udiv.s,v 1.3 1994/02/09 01:27:17 limes Exp $

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	|__rt_udiv|

|__rt_udiv|                                                           ; udiv ;
;
; Unsigned divide of a2 by a1: returns quotient in a1, remainder in a2.
; Also destroys a3, a4 and ip

; new code from Steve Landrum

	RSBS	r2, r0, r1, LSR #31
	SBCCS	r1, r1, r0, LSL #31
	ADC	r3, r3, r3		; 1
	RSBS	r2, r0, r1, LSR #30
	SBCCS	r1, r1, r0, LSL #30
	ADC	r3, r3, r3		; 2
	RSBS	r2, r0, r1, LSR #29
	SBCCS	r1, r1, r0, LSL #29
	ADC	r3, r3, r3		; 3
	RSBS	r2, r0, r1, LSR #28
	SBCCS	r1, r1, r0, LSL #28
	ADC	r3, r3, r3		; 4
	RSBS	r2, r0, r1, LSR #27
	SBCCS	r1, r1, r0, LSL #27
	ADC	r3, r3, r3		; 5
	RSBS	r2, r0, r1, LSR #26
	SBCCS	r1, r1, r0, LSL #26
	ADC	r3, r3, r3		; 6
	RSBS	r2, r0, r1, LSR #25
	SBCCS	r1, r1, r0, LSL #25
	ADC	r3, r3, r3		; 7
	RSBS	r2, r0, r1, LSR #24
	SBCCS	r1, r1, r0, LSL #24
	ADC	r3, r3, r3		; 8
	RSBS	r2, r0, r1, LSR #23
	SBCCS	r1, r1, r0, LSL #23
	ADC	r3, r3, r3		; 9
	RSBS	r2, r0, r1, LSR #22
	SBCCS	r1, r1, r0, LSL #22
	ADC	r3, r3, r3		; 10
	RSBS	r2, r0, r1, LSR #21
	SBCCS	r1, r1, r0, LSL #21
	ADC	r3, r3, r3		; 11
	RSBS	r2, r0, r1, LSR #20
	SBCCS	r1, r1, r0, LSL #20
	ADC	r3, r3, r3		; 12
	RSBS	r2, r0, r1, LSR #19
	SBCCS	r1, r1, r0, LSL #19
	ADC	r3, r3, r3		; 13
	RSBS	r2, r0, r1, LSR #18
	SBCCS	r1, r1, r0, LSL #18
	ADC	r3, r3, r3		; 14
	RSBS	r2, r0, r1, LSR #17
	SBCCS	r1, r1, r0, LSL #17
	ADC	r3, r3, r3		; 15
	RSBS	r2, r0, r1, LSR #16
	SBCCS	r1, r1, r0, LSL #16
	ADC	r3, r3, r3		; 16
	RSBS	r2, r0, r1, LSR #15
	SBCCS	r1, r1, r0, LSL #15
	ADC	r3, r3, r3		; 17
	RSBS	r2, r0, r1, LSR #14
	SBCCS	r1, r1, r0, LSL #14
	ADC	r3, r3, r3		; 18
	RSBS	r2, r0, r1, LSR #13
	SBCCS	r1, r1, r0, LSL #13
	ADC	r3, r3, r3		; 19
	RSBS	r2, r0, r1, LSR #12
	SBCCS	r1, r1, r0, LSL #12
	ADC	r3, r3, r3		; 20
	RSBS	r2, r0, r1, LSR #11
	SBCCS	r1, r1, r0, LSL #11
	ADC	r3, r3, r3		; 21
	RSBS	r2, r0, r1, LSR #10
	SBCCS	r1, r1, r0, LSL #10
	ADC	r3, r3, r3		; 22
	RSBS	r2, r0, r1, LSR #9
	SBCCS	r1, r1, r0, LSL #9
	ADC	r3, r3, r3		; 23
	RSBS	r2, r0, r1, LSR #8
	SBCCS	r1, r1, r0, LSL #8
	ADC	r3, r3, r3		; 24
	RSBS	r2, r0, r1, LSR #7
	SBCCS	r1, r1, r0, LSL #7
	ADC	r3, r3, r3		; 25
	RSBS	r2, r0, r1, LSR #6
	SBCCS	r1, r1, r0, LSL #6
	ADC	r3, r3, r3		; 26
	RSBS	r2, r0, r1, LSR #5
	SBCCS	r1, r1, r0, LSL #5
	ADC	r3, r3, r3		; 27
	RSBS	r2, r0, r1, LSR #4
	SBCCS	r1, r1, r0, LSL #4
	ADC	r3, r3, r3		; 28
	RSBS	r2, r0, r1, LSR #3
	SBCCS	r1, r1, r0, LSL #3
	ADC	r3, r3, r3		; 29
	RSBS	r2, r0, r1, LSR #2
	SBCCS	r1, r1, r0, LSL #2
	ADC	r3, r3, r3		; 30
	RSBS	r2, r0, r1, LSR #1
	SBCCS	r1, r1, r0, LSL #1
	ADC	r3, r3, r3		; 31
	RSBS	r2, r0, r1
	SBCCS	r1, r1, r0
	ADC	r0, r3, r3		; 32 bits of result (Whew!)

	MOV	pc, r14

;        MOVS    a3, a1
;;	BEQ     dividebyzero
;        MOV     a4, #0
;        MOV     ip, #&80000000
;        CMP     a2, ip
;        MOVLO   ip, a2
;u_loop
;        CMP     ip, a3, ASL #0
;        BLS     u_shifted0mod8
;        CMP     ip, a3, ASL #1
;        BLS     u_shifted1mod8
;        CMP     ip, a3, ASL #2
;        BLS     u_shifted2mod8
;        CMP     ip, a3, ASL #3
;        BLS     u_shifted3mod8
;        CMP     ip, a3, ASL #4
;        BLS     u_shifted4mod8
;        CMP     ip, a3, ASL #5
;        BLS     u_shifted5mod8
;        CMP     ip, a3, ASL #6
;        BLS     u_shifted6mod8
;        CMP     ip, a3, ASL #7
;        MOVHI   a3, a3, ASL #8
;        BHI     u_loop
;u_loop2
;u_shifted7mod8
;        CMP     a2, a3, ASL #7
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #7
;u_shifted6mod8
;        CMP     a2, a3, ASL #6
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #6
;u_shifted5mod8
;        CMP     a2, a3, ASL #5
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #5
;u_shifted4mod8
;        CMP     a2, a3, ASL #4
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #4
;u_shifted3mod8
;        CMP     a2, a3, ASL #3
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #3
;u_shifted2mod8
;        CMP     a2, a3, ASL #2
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #2
;u_shifted1mod8
;        CMP     a2, a3, ASL #1
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #1
;u_shifted0mod8
;        CMP     a2, a3, ASL #0
;        ADC     a4, a4, a4
;        SUBHS   a2, a2, a3, ASL #0
;        CMP     a1, a3, LSR #1
;        MOVLS   a3, a3, LSR #8
;        BLS     u_loop2
;        MOV     a1, a4
;        MOV    pc, r14

	END
