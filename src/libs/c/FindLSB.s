	;; $Id: FindLSB.s,v 1.9 1994/09/10 02:52:22 vertex Exp $
;
;	Name:
;		FindLSB
;	Purpose:
;		To find the Least significant bit position in
;		a 32 bit word.
;	Entry:
;		r0 = int32 number
;	Exit:
;		r0 = int32 lsb position = 0-$20
;
;	C prototype:
;		int32 FindLSB(int32 val)

	AREA	|ASMCODE|, CODE, READONLY

	EXPORT	FindLSB
	EXPORT	ffs

;;;	AUTODOC PUBLIC spg/clib/findlsb
;;;	FindLSB - Find the least significant bit.
;;;
;;;	  Synopsis
;;;
;;;	    int FindLSB( uint32 mask )
;;;
;;;	  Description
;;;
;;;	    FindLSB finds the lowest-numbered bit that is set in the argument.
;;;	    The least significant bit is bit number 1, and the most
;;;	    significant bit is bit 32.
;;;
;;;	  Arguments
;;;
;;;	    flags                       The 32-bit word to check
;;;
;;;	  Return Value
;;;
;;;	    FindLSB returns the number of the lowest-numbered bit that is set
;;;	    in the argument (for example, 1 if the value of the argument is 1
;;;	    or 3 or 7, 3 if the value of the argument is 4 or 12).  If no bits
;;;	    are set, the procedure returns 0.
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
;;;	    FindMSB(), ffs()
;;;

;;;	AUTODOC PUBLIC spg/clib/ffs
;;;	ffs - Find the first set bit.
;;;
;;;	  Synopsis
;;;
;;;	    int ffs( uint32 mask )
;;;
;;;	  Description
;;;
;;;	    ffs finds the lowest-numbered bit that is set in the argument.
;;;	    The least significant bit is bit number 1, and the most
;;;	    significant bit is bit 32.
;;;
;;;	  Arguments
;;;
;;;	    flags                       The 32-bit word to check
;;;
;;;	  Return Value
;;;
;;;	    ffs returns zero if no bits are set in the parameter, or the bit
;;;	    index of the first bit set in the parameter. For example, ffs
;;;	    returns 1 if the value of the argument is 1 or 3 or 7; and 3 if
;;;	    the value of the argument is 4 or 12.  If no bits are set, ffs
;;;	    returns 0.
;;;
;;;	  Implementation
;;;
;;;	    Convenience call implemented in clib.lib V20.
;;;
;;;	  Associated Files
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  Notes
;;;
;;;	    This entry point is provided for portability and compatibility
;;;	    only; code written specificly for Portfolio should use FindLSB or
;;;	    FindMSB to reduce any ambiguity over whether the least signifocant
;;;	    or most significant bit is desired.
;;;
;;;	  See Also
;;;
;;;	    FindMSB(), FindLSB()
;;;

ffs
FindLSB
	movs	r1,r0
	moveq	pc,lr
	movs	r2,r1,LSL #16
	movne	r0,#24
	moveq	r0,#8
	movs	r2,r1,LSL r0
	addne	r0,r0,#4
	subeq	r0,r0,#4
	movs	r2,r1,LSL r0
	addne	r0,r0,#2
	subeq	r0,r0,#2
	movs	r2,r1,LSL r0
	addne	r0,r0,#1
	subeq	r0,r0,#1
	movs	r2,r1,LSL r0
	addne	r0,r0,#1
	rsb	r0,r0,#&21
	mov	pc,lr

	END
