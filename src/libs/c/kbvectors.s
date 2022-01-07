	;; $Id: kbvectors.s,v 1.30 1995/02/25 00:01:04 vertex Exp $
	GET	structs.i
	GET	nodes.i
	GET	list.i
	GET	folio.i
	GET	kernel.i

LOCATEITEM	EQU	-12
GETPAGESIZE	EQU	-15
CHECKITEM	EQU	-16
SCAVENGEMEM	EQU	-11
USECS2TICKS	EQU	-18
TICKS2TIMEVAL	EQU	-23
FINDMH		EQU	-25
ALLOCMEMML	EQU	-26
FREEMEMML	EQU	-27
ALLOCMEMLIST	EQU	-28
FREEMEMLIST	EQU	-29
ISITEMOPENED	EQU	-32
GETMEMTRACKSIZE	EQU	-40
CHECKIO		EQU	-41
ISMEMREADABLE	EQU	-42
ISMEMWRITABLE	EQU	-43

	MACRO
$label	GETKERNELBASE
	ldr	r9,adrKB
	ldr	r9,[r9]
	MEND

	IMPORT	KernelBase

	AREA	|STUBKernelBase|, CODE, READONLY

adrKB	DCD	KernelBase

	AREA	|STUBCheckItem|, CODE, READONLY

	EXPORT	CheckItem
;	Call through vector interface
CheckItem
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#CHECKITEM<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBLookupItem|, CODE, READONLY

	EXPORT	LookupItem
;	Call through vector interface
LookupItem
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#LOCATEITEM<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBGetPageSize|, CODE, READONLY

	EXPORT	|_GetPageSize|
	EXPORT	GetPageSize
;	Call through vector interface
|_GetPageSize|
GetPageSize
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#GETPAGESIZE<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBScavengeMem|, CODE, READONLY

	EXPORT	|_ScavengeMem|
	EXPORT	ScavengeMem
;	Call through vector interface
|_ScavengeMem|
ScavengeMem
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#SCAVENGEMEM<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBUSecToTicks|, CODE, READONLY

	EXPORT	|_USecToTicks|
	EXPORT	USecToTicks
;	Call through vector interface
|_USecToTicks|
USecToTicks
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#USECS2TICKS<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBTicksToTimeVal|, CODE, READONLY

	EXPORT	|_TicksToTimeVal|
	EXPORT	TicksToTimeVal
;	Call through vector interface
|_TicksToTimeVal|
TicksToTimeVal
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#TICKS2TIMEVAL<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBFindMH|, CODE, READONLY

	EXPORT	|_FindMH|
	EXPORT	FindMH
;	Call through vector interface
|_FindMH|
FindMH
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#FINDMH<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBAllocMemFromMemList|, CODE, READONLY

	EXPORT	|_AllocMemFromMemList|
	EXPORT	AllocMemFromMemList
;	Call through vector interface
|_AllocMemFromMemList|
AllocMemFromMemList
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#ALLOCMEMML<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBFreeMemToMemList|, CODE, READONLY

	EXPORT	|_FreeMemToMemList|
	EXPORT	FreeMemToMemList
;	Call through vector interface
|_FreeMemToMemList|
FreeMemToMemList
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#FREEMEMML<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBAllocMemList|, CODE, READONLY

	EXPORT	|_AllocMemList|
	EXPORT	AllocMemList
;	Call through vector interface
|_AllocMemList|
AllocMemList
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#ALLOCMEMLIST<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBFreeMemList|, CODE, READONLY

	EXPORT	|_FreeMemList|
	EXPORT	FreeMemList
;	Call through vector interface
|_FreeMemList|
FreeMemList
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#FREEMEMLIST<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBIsItemOpened|, CODE, READONLY

	EXPORT	|_ItemOpened|
	EXPORT	|_IsItemOpened|
	EXPORT	ItemOpened
	EXPORT	IsItemOpened
;	Call through vector interface
|_IsItemOpened|
|_ItemOpened|
IsItemOpened
ItemOpened
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#ISITEMOPENED<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBGetMemTrackSize|, CODE, READONLY

	EXPORT	GetMemTrackSize
;	Call through vector interface
GetMemTrackSize
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#GETMEMTRACKSIZE<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBCheckIO|, CODE, READONLY

	EXPORT	CheckIO
;	Call through vector interface
CheckIO
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#CHECKIO<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBIsMemReadable|, CODE, READONLY

	EXPORT	IsMemReadable
;	Call through vector interface
IsMemReadable
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#ISMEMREADABLE<<2]
	ldmfd	sp!,{r9,r15}	; return

	AREA	|STUBIsMemWritable|, CODE, READONLY

	EXPORT	IsMemWritable
;	Call through vector interface
IsMemWritable
	stmfd	sp!,{r9,r14}	; save r9, and return
	GETKERNELBASE
	mov	r14,r15		; get return address
	ldr	r15,[r9,#ISMEMWRITABLE<<2]
	ldmfd	sp!,{r9,r15}	; return

	END
