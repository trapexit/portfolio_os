	;; $Id: samplesystemtimetv.s,v 1.2 1994/09/21 16:59:58 vertex Exp $

	GET	structs.i
	GET	time.i
	GET	registers.i

;;;	AUTODOC PUBLIC spg/kernel/samplesystemtimetv
;;;	SampleSystemTimeTV - Sample the system time with very low overhead.
;;;
;;;	  Synopsis
;;;
;;;	    void SampleSystemTimeTV( TimeVal *time )
;;;
;;;	  Description
;;;
;;;	    This function records the current system time in the supplied
;;;	    TimeVal structure. This is a very low overhead call giving a very
;;;	    high accuracy timing.
;;;
;;;	    The time value returned by this function corresponds to the
;;;	    time maintained by the TIMERUNIT_USEC unit of the timer device.
;;;
;;;	  Arguments
;;;
;;;	    time                        A pointer to a TimeVal structure which
;;;	                                will receive the current system time.
;;;
;;;	  Implementation
;;;
;;;	    Convenience call implemented in clib.lib V24.
;;;
;;;	  Associated Files
;;;
;;;	    time.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  See Also
;;;
;;;	    SampleSystemTime()
;;;

	AREA	|ASMCODE|, CODE, READONLY

	ALIGN	4		; alignment paranoia

;------------------------------------------------------------------------------

	EXPORT SampleSystemTimeTV
;
; void SampleSystemTimeTV(TimeVal *tv)
;
SampleSystemTimeTV
	stmfd	sp!,{r0,lk}		; save the return adr and TimeVal ptr
	swi	&10026			; call SampleSystemTime()
	ldmfd	sp!,{r2,lk}		; restore return adr and TimeVal ptr
	str	r0,[r2,#tv_sec]		; store returned sec
	str	r1,[r2,#tv_usec]	; store returned usec
	mov	pc,lk			; return to caller

;------------------------------------------------------------------------------

	END
