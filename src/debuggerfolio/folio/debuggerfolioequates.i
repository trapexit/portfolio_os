; $Id: debuggerfolioequates.i,v 1.2 1994/08/02 02:49:12 anup Exp $

;
; DebugFolioEquates.i
;
;	Written by: Anup K Murarka
;
; ©1994 by The 3DO Company, all rights reserved.
;

SRAMAddr				EQU		&03700000

MonitorEntryVector		EQU		SRAMAddr

Monitor_Init_Vector		EQU		MonitorEntryVector
Monitor_Undef_Vector	EQU		Monitor_Init_Vector + 4
Monitor_swi_Vector		EQU		Monitor_Undef_Vector + 4
Monitor_AbortPF_Vector	EQU		Monitor_swi_Vector + 4
Monitor_AbortD_Vector	EQU		Monitor_AbortPF_Vector + 4
Monitor_AddrExec_Vector	EQU		Monitor_AbortD_Vector + 4
Monitor_IRQ_Vector		EQU		Monitor_AddrExec_Vector + 4
Monitor_FIRQ_Vector		EQU		Monitor_IRQ_Vector + 4
Monitor_Handler_Vector	EQU		Monitor_FIRQ_Vector + 4

MonitorTopMemory		EQU		&03740000

Monitor_Hello_Vector	EQU		MonitorTopMemory-((32+32+12+4)*4)
Monitor_Folio_Vector	EQU		MonitorTopMemory-((32+32+12)*4)

						END


;@@@@@@ JUST NOTES, IT IS AFTER THE END DIRECTIVE!
FolioVector		BAL		DebuggerFolio
FrameBuff2		DCD		0
CurFrameBuff	DCD		0
bufferOffset	DCD		0
GetVideo		DCD		0
BytesInFifo		DCD		0
MustGetFrame	DCD		0
Modulo			DCD		0
Width			DCD		0
Height			DCD		0
HardBP			DCD		0
Skip			DCD		0

