	;; $Id: samplesystemtime.s,v 1.2 1994/09/22 23:34:03 vertex Exp $

	GET	structs.i
	GET	time.i
	GET	registers.i

;;;	AUTODOC PUBLIC spg/kernel/samplesystemtime
;;;	SampleSystemTime - Sample the system time with very low overhead.
;;;
;;;	  Synopsis
;;;
;;;	    uint32 SampleSystemTime( void )
;;;
;;;	  Description
;;;
;;;	    This function samples the current system time, and returns
;;;	    it to you. The primary return value is the seconds count of
;;;	    the system clock. A secondary value available in the r1 register
;;;	    on ARM processors is the microseconds count of the system clock.
;;;	    For a C-friendly version of this call, see SampleSystemTimeTV().
;;;
;;;	    This function is very low overhead and is meant for high-accuracy
;;;	    timings.
;;;
;;;	    The time value returned by this function corresponds to the
;;;	    time maintained by the TIMERUNIT_USEC unit of the timer device.
;;;
;;;	  Return Value
;;;
;;;	    This function returns the seconds count of the system clock. It
;;;	    also returns the microseconds count in the r1 ARM processor
;;;	    register.
;;;
;;;	  Implementation
;;;
;;;	    SWI implemented in kernel folio V24.
;;;
;;;	  Associated Files
;;;
;;;	    time.h
;;;
;;;	  See Also
;;;
;;;	    SampleSystemTimeTV()                      ANSI C Prototype
;;;

	AREA	|ASMCODE|, CODE, READONLY

	ALIGN	4		; alignment paranoia

;------------------------------------------------------------------------------

	IMPORT TimeStamp
	EXPORT SampleSystemTime
;
; [r0 = sec, r1 = usec] SampleSystemTime(void)
;
; This is a called as a SWI
;
SampleSystemTime
	stmfd	sp!,{lk}	; save the return adr
	sub	sp,sp,#8	; make room for two longwords
	mov	r0,sp		; get top of the stack
	bl	TimeStamp	; call TimeStamp
	ldr	r0,[sp]		; load tv_sec value
	ldr	r1,[sp,#4]	; load tv_usec value
	add	sp,sp,#8	; remove temp storage from stack
	ldmfd	sp!,{pc}	; return

;------------------------------------------------------------------------------

	END
