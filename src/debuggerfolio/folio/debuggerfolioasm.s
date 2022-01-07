; $Id: debuggerfolioasm.s,v 1.4 1995/02/15 21:45:58 anup Exp $

;
; DebuggerFolio.s
;
;	Written by: Anup K Murarka
;
; ©1994 by The 3DO Company, all rights reserved.
;

					INCLUDE	debuggerfolioequates.i


					AREA	|DebuggerFolioASM|, CODE, READONLY
					
					EXPORT	|FolioMonitorVector|
	
FolioMonitorVector	ROUT
					LDR		PC,FolioMonitorAddr

FolioMonitorAddr	DCD		Monitor_Folio_Vector
					ROUT


					EXPORT	|SayHelloVector|
	
SayHelloVector		ROUT
					LDR		PC,MonitorSayHello

MonitorSayHello		DCD		Monitor_Hello_Vector
					ROUT
					
					EXPORT	|GoMonitorSWIHandler|
	
GoMonitorSWIHandler	ROUT
					LDMFD	R13!,{R8,R9,R10,R11,R14}
					LDR		PC,MonitorSWIHandler

MonitorSWIHandler	DCD		Monitor_swi_Vector
					ROUT

					EXPORT	|swiDebugTaskLaunch|
					IMPORT	|SendTaskLaunchCmd|

swiDebugTaskLaunch	ROUT

; Execute task launch messaging code
					STMFD	R13!,{R14}					;SAVE Original Link Reg
					
					SUB		R0,R14,#4					;Get address of original swi				
					BL		SendTaskLaunchCmd

; Execute normal swi handling
					LDMFD	R13!,{R14}					;Restore Original Link Reg
					LDMFD	R13!,{R8,R9,R10,R11,R14}
					LDR		PC,MonitorSWIHandler

					ROUT


;
; This is the handler that intercepts all swi calls and hands
; them off to the folio for processing
;
; code is needed here to do a quick check of our TraceEnable flag
; and return immediately if it is false
;
					EXPORT	|SwiTraceEntry|
					EXPORT	|OldSwiVector|

SwiTraceEntry		ROUT
					IMPORT	|SWIIntercept|
					STMDB	R13!,{R0-R12}				; save all regs for display
	; Set up parameters to C function
					LDR		R0,[R14,#-4]					; get swi opcode
					MOV		R1,R14						; store return address
					MOV		R2,R13						; arg3 to C code is pointer to regs

					STMDB	R13!,{R14}					; save link reg
					BL		SWIIntercept				; jump to C code
					LDMIA	R13!,{R14}

					LDMIA	R13!,{R0-R12}
					LDR		PC,OldSwiVector				; jump to old swi handler

OldSwiVector		DCD		0							; Filled with the opcode from 0x08
					ROUT


;
; This is actually a duplicate of the routine found in startup.s
; but I couldn't find the library in which this was exported and 
; accessible form the debuggerfolio
;
					EXPORT	|EnableIrq|

EnableIrq			ROUT

					mrs		r1,CPSR
					bic		r1,r1,#&80
					msr		CPSR_ctl,r1
					mov		pc,r14
					
					ROUT



					END
