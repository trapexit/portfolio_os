;	$Id: drvhdr.s,v 1.1 1994/07/19 22:52:20 markn Exp $
;	Header for "swappable" dipir device driver files (XXX.drv).
;	Each dipir driver gets linked with this header and put into driver.pak.

	AREA	|ASMCODE|, CODE, READONLY

	; The 3DOBinHeader
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0

;	Driver vector table
	IMPORT	InitDisc
	DCD	InitDisc
	DCD	0
	DCD 	0
	DCD 	0

	DCD	&3512ABC0	; Magic number for dipir drivers
	DCD	&1		; version of this header

	DCW	MANUFACTURER_ID	 ; The device which this driver drives.
	DCW	MANUFACTURER_DEV


	EXPORT	|__main|
	EXPORT |__main|

;	Just return to caller.
;	Return address is in r8 (see callaif2 in dipir.s).
;	The only purpose of the AIF entry point for a driver is to
;	do relocation.  The "main" entry point doesn't need to do anything.
;	It just returns (to the caller of callaif2, not its own caller).
	ENTRY
|__main|
	mov	r0,r5
	mov	r1,r6
	mov	r9,r7
	mov	r15,r8

|__hangstack|
	b  |__hangstack|

	END
