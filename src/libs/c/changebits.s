;;; $Id: changebits.s,v 1.2 1995/02/07 01:09:06 limes Exp $
;;;
;;; These functions atomicly clear or set bits in a word
;;; they are not currently usable from unprivileged code,
;;; since they disable interrupts. They will become available
;;; in the M2 release for use by unprivileged code without
;;; the word "Super" in their names.
;;;
;;; uint32 SuperAtomicClearBits(uint32 *ptr, uint32 bits)
;;; uint32 SuperAtomicSetBits(uint32 *ptr, uint32 bits)

        AREA |C$$code|, CODE, READONLY
|x$codeseg|

        EXPORT  |SuperAtomicClearBits|
|SuperAtomicClearBits|
	MRS	 r3,CPSR
	ORR	 r2,r3,#&c0	; disable interrupts
	MSR	 CPSR,r2

        LDR      r2,[r0,#0]
        BIC      r1,r2,r1	; the real work
        STR      r1,[r0,#0]

        MOV      r0,r2
	MSR	 CPSR,r3	; restore interrupts
        MOV      pc,lr

        EXPORT  |SuperAtomicSetBits|
|SuperAtomicSetBits|
	MRS	 r3,CPSR
	ORR	 r2,r3,#&c0	; disable interrupts
	MSR	 CPSR,r2

        LDR      r2,[r0,#0]
        ORR      r1,r2,r1	; the real work
        STR      r1,[r0,#0]

        MOV      r0,r2
	MSR	 CPSR,r3	; restore interrupts
        MOV      pc,lr

        END
