	;; $Id: DriverletGlue.s,v 1.3 1994/02/09 01:27:17 limes Exp $
;	c initial startup code for tasks

	AREA	|ASMCODE|, CODE, READONLY

;	extended header for 3do binaries
;	filled in by 3do bin tool
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0
	DCD	0,0,0,0,0,0,0,0

	EXPORT	|CuriousGreenIdeasSleptFuriously|
	IMPORT	DriverletEntry

	EXPORT |CuriousGreenIdeasSleptFuriously|
;	on entry r0 = pointer to pod structure
;	         r1 = pointer to KernelBase
;	we are running within the domain of the Event Broker

	ENTRY
|CuriousGreenIdeasSleptFuriously|

	mov	r1,r9
	ldr	r2,adrKB
	str	r9,[r2]

	b	DriverletEntry		; scream and leap

STACKOVERFLOW	EQU	-(31*4)

	EXPORT	|__rt_stkovf_split_small|
	EXPORT	|__rt_stkovf_split_big|

	
|__rt_stkovf_split_small|
|__rt_stkovf_split_big|

	stmfd	sp!,{r0}
	ldr	r0,adrKB
	ldr	r0,[r0]
	stmfd	sp!,{r0}
	ldr	pc,[r0,#STACKOVERFLOW]	; return address is already set
	ldmfd	sp!,{r0,pc}

|__hangstack|
	b  |__hangstack|

adrKB	DCD	|_KernelBase|

	EXPORT	|_KernelBase|
|_KernelBase|	DCD	0

	END
