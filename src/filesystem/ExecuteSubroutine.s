; $Id: ExecuteSubroutine.s,v 1.2 1994/03/26 02:41:16 dplatt Exp $
;
; ExecuteAsSubroutine() implementation

	GET registers.i
	AREA	|ASMCODE|, CODE, READONLY

;-----------------------------------------------------------------------------

	EXPORT	|_ExecuteAsSub|
	IMPORT	|_KernelBase|

;-----------------------------------------------------------------------------

; Err ExecuteAsSub(void *code, uint32 argc, char **argv);
;
;	r0 has code
;	r1 has argc
;	r2 has argv
;
; We must simply copy the cmd line on the stack, and jump into the code...

|_ExecuteAsSub|
	stmfd	sp!,{r1-r12,r14,r15}	 ; save all registers

        b	enterCode	 ; skip over following 2 instructions
	nop			 ; dummy

	; this is where the called subroutine will return to us. We
	; then just shove the link register in the PC, and we're back to
	; our caller.
        mov	pc,lk


enterCode
	mov	r5,r1		 ; load argc
	mov	r6,r2		 ; load argv
	ldr	r7,adrKB
	ldr	r7,[r7]          ; load KernelBase

	mov	lk,pc		 ; set up link
	mov	pc,r0		 ; go do code

	; code doesn't return here, and instead returns a few lines
	; above...

;-----------------------------------------------------------------------------

adrKB	DCD	|_KernelBase|

;-----------------------------------------------------------------------------

	END
