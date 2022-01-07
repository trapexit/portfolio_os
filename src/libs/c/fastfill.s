	;; $Id: fastfill.s,v 1.2 1994/06/22 18:20:27 limes Exp $


	AREA	|ASMCODE|, CODE, READONLY

;	FillMemoryWithWord (uint32 *ptr, uint32 size, uint32 pat)
	EXPORT	|_FillMemoryWithWord|
	EXPORT	FillMemoryWithWord

|_FillMemoryWithWord|
FillMemoryWithWord
	STMFD	R13!,{R4-R9,R14}
	MOV	R1,R1,LSR #5
	MOV	R3,R2
	MOV	R4,R2
	MOV	R5,R2
	MOV	R6,R2
	MOV	R7,R2
	MOV	R8,R2
	MOV	R9,R2
FFL
	STMIA	R0!,{R2-R9}
	SUBS	R1,R1,#1
	BNE	FFL

	LDMFD	R13!,{R4-R9,R15}
	
	END
