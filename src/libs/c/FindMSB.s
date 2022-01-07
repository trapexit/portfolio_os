	;; $Id: FindMSB.s,v 1.12 1994/09/10 02:52:22 vertex Exp $
;
;	Name:
;		FindMSB
;	Purpose:
;		To find the Most significant bit position in
;		a 32 bit word.
;	Entry:
;		r0 = int32 number
;	Exit:
;		r0 = int32 msb position = 0-$20
;
;	C prototype:
;		int32 FindMSB(int32 val)

;;;	AUTODOC PUBLIC spg/clib/findmsb
;;;	FindMSB - Find the highest-numbered bit.
;;;
;;;	  Synopsis
;;;
;;;	    int FindMSB( uint32 bits )
;;;
;;;	  Description
;;;
;;;	    This procedure finds the highest-numbered bit that is set in the
;;;	    argument.  The least significant bit is bit number 1, and the most
;;;	    significant bit is bit 32.
;;;
;;;	  Arguments
;;;
;;;	    bits                        The 32-bit word to check
;;;
;;;	  Return Value
;;;
;;;	    The procedure returns the number of the highest-numbered bit that
;;;	    is set in the argument.  If no bits are set, the procedure returns
;;;	    0.
;;;
;;;	  Implementation
;;;
;;;	    Convenience call implemented in clib.lib V20.
;;;
;;;	  Associated Files
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  See Also
;;;
;;;	    FindLSB(), ffs()
;;;

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	FindMSB

FindMSB
	movs	r1,r0
	moveq	pc,lr
	movs	r2,r1,LSR #16
	movne	r0,#24
	moveq	r0,#8
	movs	r2,r1,LSR r0
	addne	r0,r0,#4
	subeq	r0,r0,#4
	movs	r2,r1,LSR r0
	addne	r0,r0,#2
	subeq	r0,r0,#2
	movs	r2,r1,LSR r0
	addne	r0,r0,#1
	subeq	r0,r0,#1
	movs	r2,r1,LSR r0
	addne	r0,r0,#1

	mov	pc,lr

	END
