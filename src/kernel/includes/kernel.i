

;*****************************************************************************
;*
;*  $Id: kernel.i,v 1.33 1994/12/08 19:07:09 markn Exp $
;*
;*  Kernel folio structure definition
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE	folio.i

 IF	:DEF:|_KERNEL_I|
 ELSE
	GBLL	|_KERNEL_I|

	BEGINSTRUCT	KernelBase
		STRUCT	Folio,kb
		PTR	kb_RomTags
		PTR	kb_MemFreeLists
		PTR	kb_MemHdrList
		PTR	kb_FolioList
		PTR	kb_Drivers
		PTR	kb_Devices
		PTR	kb_TaskWaitQ
		PTR	kb_TaskReadyQ
		PTR	kb_MsgPorts
		PTR	kb_Semaphores
		PTR	kb_CurrentTask
		PTR	kb_InterruptHandlers
		PTR	kb_TimerBits
		UINT32	kb_ElapsedQuanta
 IF	:DEF:|EXTERNAL_RELEASE|
		ARRAY	UINT32,kb_Private0,17
 ELSE
		UINT32	kb_VRAMHACK
		PTR	kb_ItemTable
		UINT32	kb_MaxItem
		UINT32	kb_CPUFlags
		UINT8	kb_MaxInterrupts
		UINT8	kb_Forbid
		UINT8	kb_FolioTableSize
		UINT8	kb_PleaseReschedule
		UINT32	kb_MacPkt
		UINT32	kb_Flags
		UINT32	kb_Reserved
		UINT32	kb_numticks
		UINT32	kb_denomticks
		UINT32	kb_Obsolete
		UINT8	kb_FolioTaskDataCnt
		UINT8	kb_FolioTaskDataSize
		UINT8	kb_DRAMSetSize
		UINT8	kb_VRAMSetSize
		PTR	kb_DataFolios
		PTR	kb_CatchDataAborts
		UINT32	kb_QuietAborts
		PTR	kb_RamDiskAddr
		INT32	kb_RamDiskSize
 ENDIF
		PTR	kb_ExtentedErrors
		UINT8	kb_MadamRev
		UINT8	kb_ClioRev
 IF	:DEF:|EXTERNAL_RELEASE|
		ARRAY	UINT8,kb_Private1,2
		ARRAY	UINT32,kb_Private2,2
 ELSE
		UINT8	kb_Resbyte0
		UINT8	kb_Resbyte1
		UINT32	kb_DevSemaphore
		PTR	kb_SystemStackList
 ENDIF
		UINT32	kb_NumTaskSwitches

 IF	:DEF:|EXTERNAL_RELEASE|
		UINT32	UINT32,kb_Private3,4
 ELSE
		PTR	kb_VRAM0
		UINT32	kb_VRAM0Size
		PTR	kb_VRAM1
		UINT32	kb_VRAM1Size
 ENDIF
		PTR	kb_BootVolumeName
		PTR	kb_Tasks
	ENDSTRUCT

 IF	:DEF:|EXTERNAL_RELEASE|
 ELSE
; kb_CPUFlags */

KB_BIGENDIAN   EQU 1
KB_32BITMODE   EQU 2
KB_ARM600      EQU 4
KB_SHERRY      EQU 8
KB_SHERRIE     EQU KB_SHERRY
KB_BLUE        EQU &10
KB_RED		EQU &20
KB_REDWW	EQU &40
KB_GREEN	EQU &80
KB_WIREWRAP	EQU &100


KB_ROMOVERCD	EQU &0800
KB_CONTROLPORT	EQU &1000
KB_ROMAPP	EQU &2000
KB_NODBGRPOOF	EQU &4000
KB_NODBGR	EQU &8000

KB_BROOKTREE  EQU &00010000
KB_SERIALPORT EQU &00100000
KB_REBOOTED   EQU &01000000

KB_TASK_DBG    EQU 1

REDMHZx1000	EQU	36000
MHZx1000       	EQU	49091
REDWWMHZx1000	EQU	26820

CHIP_RED_REV	EQU 0
CHIP_GREEN_REV	EQU 1

 ENDIF

 ENDIF  ; |_KERNEL_I|

	END
