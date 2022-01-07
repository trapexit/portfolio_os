	;; $Id: supervectors.s,v 1.30 1994/09/30 18:43:08 vertex Exp $
	GET	structs.i

	IMPORT SuperDo

;;; ========================================================================
;;; The first part file contains entry points for supervisor calls
;;; that correspond to functions available to users via SWIs.

	AREA	|STUBSuperWaitIO|, CODE, READONLY

	EXPORT	SuperWaitIO
SuperWaitIO
	mov	r12,#41
	b	SuperDo

	AREA	|STUBSuperWaitPort|, CODE, READONLY

	EXPORT	SuperWaitPort
SuperWaitPort
	mov	r12,#40
	b	SuperDo

	AREA	|STUBSuperDoIO|, CODE, READONLY

	EXPORT	SuperDoIO
SuperDoIO
	mov	r12,#37
	b	SuperDo

	AREA	|STUBSuperFindAndOpenItem|, CODE, READONLY

	EXPORT	SuperFindAndOpenItem
SuperFindAndOpenItem
	mov	r12,#36
	b	SuperDo

	AREA	|STUBSuperDiscOsVersion|, CODE, READONLY

	EXPORT	SuperDiscOsVersion
SuperDiscOsVersion
	mov	r12,#35
	b	SuperDo

	AREA	|STUBSuperSystemScavengeMem|, CODE, READONLY

	EXPORT	SuperSystemScavengeMem
SuperSystemScavengeMem
	mov	r12,#33
	b	SuperDo

	AREA	|STUBSuperSetItemOwner|, CODE, READONLY

	EXPORT	SuperSetItemOwner
SuperSetItemOwner
	mov	r12,#28
	b	SuperDo

	AREA	|STUBSuperSendIO|, CODE, READONLY

	EXPORT	SuperSendIO
SuperSendIO
	mov	r12,#24
	b	SuperDo

	AREA	|STUBSuperSetFunction|, CODE, READONLY

	EXPORT	SuperSetFunction
SuperSetFunction
	mov	r12,#23
	b	SuperDo

	AREA	|STUBSuperFreeSignal|, CODE, READONLY

	EXPORT	SuperFreeSignal
SuperFreeSignal
	mov	r12,#22
	b	SuperDo

	AREA	|STUBSuperAllocSignal|, CODE, READONLY

	EXPORT	SuperAllocSignal
SuperAllocSignal
	mov	r12,#21
	b	SuperDo

	AREA	|STUBSuperControlMem|, CODE, READONLY

	EXPORT	SuperControlMem
SuperControlMem
	mov	r12,#20
	b	SuperDo

	AREA	|STUBSuperGetMsg|, CODE, READONLY

	EXPORT	SuperGetMsg
SuperGetMsg
	mov	r12,#19
	b	SuperDo

	AREA	|ASMCODE|, CODE, READONLY

	AREA	|STUBSuperReplyMsg|, CODE, READONLY

	EXPORT	SuperReplyMsg
SuperReplyMsg
	mov	r12,#18
	b	SuperDo

	AREA	|STUBSuperReadHardwareRandomNumber|, CODE, READONLY

	EXPORT	SuperReadHardwareRandomNumber
SuperReadHardwareRandomNumber
	mov	r12,#17
	b	SuperDo

	AREA	|STUBSuperSendMsg|, CODE, READONLY

	EXPORT	SuperSendMsg
	EXPORT	SuperSendSmallMsg
SuperSendMsg
SuperSendSmallMsg
	mov	r12,#16
	b	SuperDo

	AREA	|STUBSuperGetThisMsg|, CODE, READONLY

	EXPORT	SuperGetThisMsg
SuperGetThisMsg
	mov	r12,#15
	b	SuperDo

	AREA	|STUBSuperkprintf|, CODE, READONLY

	EXPORT	Superkprintf
Superkprintf
	mov	r12,#14
	b	SuperDo

	AREA	|STUBSuperCloseItem|, CODE, READONLY

	EXPORT	SuperCloseItem
SuperCloseItem
	mov	r12,#8
	b	SuperDo

	AREA	|STUBSuperLockSemaphore|, CODE, READONLY

	EXPORT	SuperLockSemaphore
SuperLockSemaphore
	mov	r12,#7
	b	SuperDo

	AREA	|STUBSuperUnlockSemaphore|, CODE, READONLY

	EXPORT	SuperUnlockSemaphore
SuperUnlockSemaphore
	mov	r12,#6
	b	SuperDo

	AREA	|STUBSuperOpenItem|, CODE, READONLY

	EXPORT	SuperOpenItem
SuperOpenItem
	mov	r12,#5
	b	SuperDo

	AREA	|STUBSuperFindItem|, CODE, READONLY

	EXPORT	SuperFindItem
SuperFindItem
	mov	r12,#4
	b	SuperDo

	AREA	|STUBSuperDeleteItem|, CODE, READONLY

	EXPORT	SuperDeleteItem
SuperDeleteItem
	mov	r12,#3
	b	SuperDo

	AREA	|STUBSuperWait|, CODE, READONLY

	EXPORT	SuperWait
SuperWait
	EXPORT	SuperWaitSignal
SuperWaitSignal
	mov	r12,#1
	b	SuperDo

	AREA	|STUBSuperCreateSizedItem|, CODE, READONLY

	EXPORT	SuperCreateSizedItem
SuperCreateSizedItem
	mov	r12,#0
	b	SuperDo

;;; ========================================================================
;;; The second part file contains entry points for supervisor calls
;;; that do not correspond to functions available to users via SWIs.

	AREA	|STUBSuperInternalPutMsg|, CODE, READONLY

	EXPORT	SuperInternalPutMsg
SuperInternalPutMsg
	mov	r12,#-1
	b	SuperDo

	AREA	|STUBDisable|, CODE, READONLY

	EXPORT	Disable
Disable
	mov	r12,#-2
	b	SuperDo

	AREA	|STUBEnable|, CODE, READONLY

	EXPORT	Enable
Enable
	mov	r12,#-3
	b	SuperDo

	AREA	|STUBSuperDebugTrigger|, CODE, READONLY

	EXPORT	SuperDebugTrigger
SuperDebugTrigger
	mov	r12,#-4
	b	SuperDo

	AREA	|STUBFirqInterruptControl|, CODE, READONLY

	EXPORT	FirqInterruptControl
FirqInterruptControl
	mov	r12,#-5
	b	SuperDo

	AREA	|STUBSuperInternalSendIO|, CODE, READONLY

	EXPORT	SuperInternalSendIO
SuperInternalSendIO
	mov	r12,#-6
	b	SuperDo

	AREA	|STUBSuperGetItem|, CODE, READONLY

	EXPORT	SuperGetItem
SuperGetItem
	mov	r12,#-7
	b	SuperDo

	AREA	|STUBSuperCompleteIO|, CODE, READONLY

	EXPORT	SuperCompleteIO
SuperCompleteIO
	mov	r12,#-8
	b	SuperDo

	AREA	|STUBSuperInternalAbortIO|, CODE, READONLY

	EXPORT	SuperInternalAbortIO
SuperInternalAbortIO
	mov	r12,#-9
	b	SuperDo

	AREA	|STUBSuperInternalControlSuperMem|, CODE, READONLY

	EXPORT	SuperInternalControlSuperMem
SuperInternalControlSuperMem
	mov	r12,#-10
	b	SuperDo

	AREA	|STUBSuperValidateMem|, CODE, READONLY

	EXPORT	SuperValidateMem
SuperValidateMem
	mov	r12,#-11
	b	SuperDo

	AREA	|STUBSuperInternalSignal|, CODE, READONLY

	EXPORT	SuperInternalSignal
SuperInternalSignal
	mov	r12,#-12
	b	SuperDo

	AREA	|STUBSuperIsRamAddr|, CODE, READONLY

	EXPORT	SuperIsRamAddr
SuperIsRamAddr
	mov	r12,#-13
	b	SuperDo

	AREA	|STUBSuperSwitch|, CODE, READONLY

	EXPORT	SuperSwitch
SuperSwitch
	mov	r12,#-14
	b	SuperDo

	AREA	|STUBTagProcessor|, CODE, READONLY

	EXPORT	TagProcessor
TagProcessor
	mov	r12,#-15
	b	SuperDo

	AREA	|STUBTimeStamp|, CODE, READONLY

	EXPORT	TimeStamp
TimeStamp
	mov	r12,#-16
	b	SuperDo

	AREA	|STUBSuperInternalUnlockSemaphore|, CODE, READONLY

	EXPORT	SuperInternalUnlockSemaphore
SuperInternalUnlockSemaphore
	mov	r12,#-17
	b	SuperDo

	AREA	|STUBSuperInternalLockSemaphore|, CODE, READONLY

	EXPORT	SuperInternalLockSemaphore
SuperInternalLockSemaphore
	mov	r12,#-18
	b	SuperDo

	AREA	|STUBSuperInternalDeleteItem|, CODE, READONLY

	EXPORT	SuperInternalDeleteItem
SuperInternalDeleteItem
	mov	r12,#-19
	b	SuperDo

	AREA	|STUBSuperInternalFreeSignal|, CODE, READONLY

	EXPORT	SuperInternalFreeSignal
SuperInternalFreeSignal
	mov	r12,#-20
	b	SuperDo

	AREA	|STUBAllocACS|, CODE, READONLY

	EXPORT	AllocACS
AllocACS
	mov	r12,#-21
	b	SuperDo

	AREA	|STUBPendACS|, CODE, READONLY

	EXPORT	PendACS
PendACS
	mov	r12,#-22
	b	SuperDo

	AREA	|STUBRegisterPeriodicVBLACS|, CODE, READONLY

	EXPORT	RegisterPeriodicVBLACS
RegisterPeriodicVBLACS
	mov	r12,#-23
	b	SuperDo

	AREA	|STUBRegisterSingleVBLACS|, CODE, READONLY

	EXPORT	RegisterSingleVBLACS
RegisterSingleVBLACS
	mov	r12,#-24
	b	SuperDo

	AREA	|STUBSuperReportEvent|, CODE, READONLY

	EXPORT	SuperReportEvent
SuperReportEvent
	mov	r12,#-25
	b	SuperDo

	AREA	|STUBSuperQuerySysInfo|, CODE, READONLY

	EXPORT	SuperQuerySysInfo
SuperQuerySysInfo
	mov	r12,#-26
	b	SuperDo

	AREA	|STUBSuperSetSysInfo|, CODE, READONLY

	EXPORT	SuperSetSysInfo
SuperSetSysInfo
	mov	r12,#-27
	b	SuperDo

	AREA	|STUBSectorECC|, CODE, READONLY

	EXPORT	SectorECC
SectorECC
	mov	r12,#-28
	b	SuperDo

	AREA	|STUBGetDirectoryCache|, CODE, READONLY

	EXPORT	GetDirectoryCache
GetDirectoryCache
	mov	r12,#-29
	b	SuperDo

	AREA	|STUBGetDirectoryBuffer|, CODE, READONLY

	EXPORT	GetDirectoryBuffer
GetDirectoryBuffer
	mov	r12,#-30
	b	SuperDo

	AREA	|STUBReleaseDirectoryBuffer|, CODE, READONLY

	EXPORT	ReleaseDirectoryBuffer
ReleaseDirectoryBuffer
	mov	r12,#-31
	b	SuperDo

	AREA	|STUBSuperInternalWaitPort|, CODE, READONLY

	EXPORT	SuperInternalWaitPort
SuperInternalWaitPort
	mov	r12,#-32
	b	SuperDo

	AREA	|STUBSuperInternalWaitIO|, CODE, READONLY

	EXPORT	SuperInternalWaitIO
SuperInternalWaitIO
	mov	r12,#-33
	b	SuperDo

	AREA	|STUBSuperInternalDoIO|, CODE, READONLY

	EXPORT	SuperInternalDoIO
SuperInternalDoIO
	mov	r12,#-34
	b	SuperDo

	END
