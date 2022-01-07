	;; $Id: CopyArgsToStack.s,v 1.3 1994/04/08 18:16:02 limes Exp $

	GET	registers.i

	AREA	|ASMCODE|, CODE, READONLY

	ALIGN	4		; alignment paranoia
	
	EXPORT	cstartup
cstartup
	ldr	r2,[sp]	; is there a command string?
	teq	r2,#0
	moveq	pc,lk	; return if nothing to do

	mov	r0,sp

;	fall into CopyArgsToStack

;	EXPORT CopyArgsToStack
;CopyArgsToStack
;
; On entry:
;     r0 points to an argument string, terminated by the first char < ' '.
;     r6 contains 0.
; Push a copy of this string on the proto-stack with inter-word spaces
; replaced by NULs. Purther push a NULL-terminated argv array pointing to
; the words of the argument list.
;
; On Exit:
;     r5 points to argv
;     r4 contains argc
;
	MOV	r6, #0
        MOV     r1, r0                     ; copy of cmdline ptr
        MOV     r3, #0
        MOV     r4, #0                     ; argc
00      MOV     r2, r3                     ; prevC
        LDRB    r3, [r1], #1
        CMP     r3, #32                    ; space?
        BGT     %B00                       ; no...
        CMP     r2, #32                    ; prevC was space?
        ADDNE   r4, r4, #1                 ; no: increment argc
        CMP     r3, #32                    ; end of command line?
        BGE     %B00                       ; no

; now copy arg string to stack, making argv as we go
        SUB     r1, r1, r0                 ; len of string
        SUB     sp, sp, r1
        BIC     sp, sp, #3
        MOV     r1, sp                     ; value for argv[0]

        MOV     r3, #0
        STMFD   sp!, {r3}                  ; last arg is NULL
        SUB     sp, sp, r4, LSL #2         ; space for argv
        MOV     r5, sp
        MOV     r4, #0                     ; argc

01      STR     r1, [r5, r4, LSL #2]       ; argv[argc] = ...
02      MOV     r2, r3                     ; prevC
        LDRB    r3, [r0], #1
        CMP     r3, #32                    ; space?
        STRGTB  r3, [r1], #1               ; no: store it and loop
        BGT     %B02
        CMP     r2, #32                    ; prevC was space?
        STRNEB  r6, [r1], #1               ; no: NUL terminate
        ADDNE   r4, r4, #1                 ; and increment argc
        CMP     r3, #32                    ; end of command line?
        BGE     %B01                       ; no

;	mov	r15,r14

	mov	r0,r4		; set up new argc
	mov	r1,r5		; set up new argv
	mov	pc,lk

	END
