; $Id: inthard.i,v 1.7 1994/02/09 02:04:35 limes Exp $
;  *************************************************************************
;  
;  Opera Internal Hardware Definitions Include File
;  
;  Copyright (C) 1992, New Technologies Group, Inc.
;  Confidential and Proprietary  -  All Rights Reserved
;  
;  This file works with any tab space from 1 to 8.
;  
;  HISTORY
;  Date   Author           Description
;  ------ ---------------- -------------------------------------------------
;  921206 -RJ              Major edit to bring up to current spec
;  921204 -RJ              Burst this file out of sherrie.h, cut the public
;                          part of sherrie.h into hardware.h
;  921104 -RJ              Fixed definition of DRAMSETZ_4MEG
;  921010 -RJ              Started name-busting this file, merged in 
;                          definitions from example.c
;  920724 -RJ Mical        Start overhaul
;  920717 Stephen Landrum  Last edits before July handoff
;  
;  **********************************************************************   

	IF :LNOT::DEF:	|__INTHARD_H|
	GBLL	|__INTHARD_H|

	INCLUDE	hardware.i



;  ===  ===================  ============================================   
;  ===                       ============================================   
;  ===  ARM CPU Definitions  ============================================   
;  ===                       ============================================   
;  ===  ===================  ============================================   

;  === 26-bit ARM processor modes ===   
ARM_MODE_USR26  EQU &00000000
ARM_MODE_FIQ26  EQU &00000001
ARM_MODE_IRQ26  EQU &00000002
ARM_MODE_SVC26  EQU &00000003


;  === 32-bit ARM processor modes ===   
ARM_MODE_USR  EQU &00000010
ARM_MODE_FIQ  EQU &00000011
ARM_MODE_IRQ  EQU &00000012
ARM_MODE_SVC  EQU &00000013
ARM_MODE_ABT  EQU &00000017
ARM_MODE_UND  EQU &0000001B


;  === 26-bit PSR bit locations ===   
ARM_MODE_MASK26  EQU &00000003
ARM_F_BIT26      EQU &04000000
ARM_I_BIT26      EQU &08000000
ARM_V_BIT26      EQU &10000000
ARM_C_BIT26      EQU &20000000
ARM_Z_BIT26      EQU &40000000
ARM_N_BIT26      EQU &80000000


;  === 32-bit PSR bit locations ===   
ARM_MODE_MASK  EQU &0000001F
ARM_F_BIT      EQU &00000040
ARM_I_BIT      EQU &00000080
ARM_V_BIT      EQU &10000000
ARM_C_BIT      EQU &20000000
ARM_Z_BIT      EQU &40000000
ARM_N_BIT      EQU &80000000


;  === ARM Exception vectors ===   
ARM_RSTV     EQU &00000000
ARM_UNDV     EQU &00000004
ARM_SWIV     EQU &00000008
ARM_PREABTV  EQU &0000000C
ARM_DATABTV  EQU &00000010
ARM_ADRV     EQU &00000014
ARM_IRQV     EQU &00000018
ARM_FIQV     EQU &0000001C


;  ===  ==========================  =====================================   
;  ===                              =====================================   
;  ===  Addresses of memory ranges  =====================================   
;  ===                              =====================================   
;  ===  ==========================  =====================================   

;??? VRAM         EQU (&00000000)
ROM          EQU (&03000000)
SLOWBUS      EQU (&03100000)
SPORT        EQU (&03200000)
MADAM        EQU (&03300000)
CLIO         EQU (&03400000)
Other1       EQU (&03404000)
Other2       EQU (&03408000)
UNCLE        EQU (&0340C000)
TRACE        EQU (&03600000)
TRACEBIGRAM  EQU (&03800000)



;  ===  ===============  ================================================   
;  ===                   ================================================   
;  ===  MADAM addresses  ================================================   
;  ===                   ================================================   
;  ===  ===============  ================================================   

MADAMREV  EQU ((MADAM+&0000))
;  Madam REV definitions   
MADAM_BROWN     EQU &00000040
MADAM_BLUE      EQU &00000044
MADAM_RED       EQU &01010000
MADAM_REDBLUE   EQU &01010001	; wirewrap only
MADAM_GREEN     EQU &01020000
MADAM_GREENWW   EQU &01020001


MADAM_GREENPLUS	EQU &01021000
MADAM_PREEN 	EQU &01022000

CLIO_GREEN	EQU &02020000
CLIO_GREENWW 	EQU &02020001
CLIO_PREEN	EQU &02022000

MSYSBits  EQU ((MADAM+&0004))
MCTL      EQU ((MADAM+&0008))
SLTIME    EQU ((MADAM+&000C))

MultiCHIP  EQU ((MADAM+&0010))

AbortBits  EQU ((MADAM+&0020))
PrivBits   EQU ((MADAM+&0024))
STATBits   EQU ((MADAM+&0028))
PRIDEC     EQU ((MADAM+&002C))

MADAMDIAG0  EQU ((MADAM+&0040))
MADAMDIAG1  EQU ((MADAM+&0044))

SPRSTRT   EQU ((MADAM+&0100))
SPRSTOP   EQU ((MADAM+&0104))
SPRCNTU   EQU ((MADAM+&0108))
SPRPAUS   EQU ((MADAM+&010C))
CCBCTL0   EQU ((MADAM+&0110))

PPMPC     EQU ((MADAM+&0120))

REGCTL0   EQU ((MADAM+&0130))
REGCTL1   EQU ((MADAM+&0134))
REGCTL2   EQU ((MADAM+&0138))
REGCTL3   EQU ((MADAM+&013C))
XYPOSH    EQU ((MADAM+&0140))
XYPOSL    EQU ((MADAM+&0144))
LINEDXYH  EQU ((MADAM+&0148))
LINEDXYL  EQU ((MADAM+&014C))
DXYH      EQU ((MADAM+&0150))
DXYL      EQU ((MADAM+&0154))
DDXYH     EQU ((MADAM+&0158))
DDXYL     EQU ((MADAM+&015C))

PLUTSTACK    EQU ((MADAM+&0180))


;  === read only addresses for the PLUT ===   
PLUT0L  EQU ((MADAM+&0180))
PLUT0R  EQU ((MADAM+&0184))
PLUT1L  EQU ((MADAM+&0188))
PLUT1R  EQU ((MADAM+&018C))
PLUT2L  EQU ((MADAM+&0190))
PLUT2R  EQU ((MADAM+&0194))
PLUT3L  EQU ((MADAM+&0198))
PLUT3R  EQU ((MADAM+&019C))
PLUT4L  EQU ((MADAM+&01A0))
PLUT4R  EQU ((MADAM+&01A4))
PLUT5L  EQU ((MADAM+&01A8))
PLUT5R  EQU ((MADAM+&01AC))
PLUT6L  EQU ((MADAM+&01B0))
PLUT6R  EQU ((MADAM+&01B4))
PLUT7L  EQU ((MADAM+&01B8))
PLUT7R  EQU ((MADAM+&01BC))
PLUT8L  EQU ((MADAM+&01C0))
PLUT8R  EQU ((MADAM+&01C4))
PLUT9L  EQU ((MADAM+&01C8))
PLUT9R  EQU ((MADAM+&01CC))
PLUTAL  EQU ((MADAM+&01D0))
PLUTAR  EQU ((MADAM+&01D4))
PLUTBL  EQU ((MADAM+&01D8))
PLUTBR  EQU ((MADAM+&01DC))
PLUTCL  EQU ((MADAM+&01E0))
PLUTCR  EQU ((MADAM+&01E4))
PLUTDL  EQU ((MADAM+&01E8))
PLUTDR  EQU ((MADAM+&01EC))
PLUTEL  EQU ((MADAM+&01F0))
PLUTER  EQU ((MADAM+&01F4))
PLUTFL  EQU ((MADAM+&01F8))
PLUTFR  EQU ((MADAM+&01FC))


;  === write only addresses for the PLUT ===   
PLUT0  EQU ((MADAM+&0180))
PLUT1  EQU ((MADAM+&0184))
PLUT2  EQU ((MADAM+&0188))
PLUT3  EQU ((MADAM+&018C))
PLUT4  EQU ((MADAM+&0190))
PLUT5  EQU ((MADAM+&0194))
PLUT6  EQU ((MADAM+&0198))
PLUT7  EQU ((MADAM+&019C))
PLUT8  EQU ((MADAM+&01A0))
PLUT9  EQU ((MADAM+&01A4))
PLUTA  EQU ((MADAM+&01A8))
PLUTB  EQU ((MADAM+&01AC))
PLUTC  EQU ((MADAM+&01B0))
PLUTD  EQU ((MADAM+&01B4))
PLUTE  EQU ((MADAM+&01B8))
PLUTF  EQU ((MADAM+&01BC))

FENCESTACK    EQU ((MADAM+&0200))


;  === FIFO read only addresses ===   
FENCE0L  EQU ((MADAM+&0230))
FENCE0R  EQU ((MADAM+&0234))
FENCE1L  EQU ((MADAM+&0238))
FENCE1R  EQU ((MADAM+&023C))

FENCE2L  EQU ((MADAM+&0270))
FENCE2R  EQU ((MADAM+&0274))
FENCE3L  EQU ((MADAM+&0278))
FENCE3R  EQU ((MADAM+&027C))


;  === FIFO write only addresses ===   
FENCE0  EQU ((MADAM+&0218))
FENCE1  EQU ((MADAM+&021C))

FENCE2  EQU ((MADAM+&0238))
FENCE3  EQU ((MADAM+&023C))


MMU  EQU ((MADAM+&0300))


DMASTACK  EQU ((MADAM+&0400))


;  === DMA Stack register offsets ===   
DMAAddress      EQU 0
DMALength       EQU 1
DMANextAddress  EQU 2
DMANextLength   EQU 3


;  === DMA Stack registers ===   
RAMtoDSPP0     EQU ((MADAM+&0400))
RAMtoDSPP1     EQU ((MADAM+&0410))
RAMtoDSPP2     EQU ((MADAM+&0420))
RAMtoDSPP3     EQU ((MADAM+&0430))
RAMtoDSPP4     EQU ((MADAM+&0440))
RAMtoDSPP5     EQU ((MADAM+&0450))
RAMtoDSPP6     EQU ((MADAM+&0460))
RAMtoDSPP7     EQU ((MADAM+&0470))
RAMtoDSPP8     EQU ((MADAM+&0480))
RAMtoDSPP9     EQU ((MADAM+&0490))
RAMtoDSPP10    EQU ((MADAM+&04A0))
RAMtoDSPP11    EQU ((MADAM+&04B0))
RAMtoDSPP12    EQU ((MADAM+&04C0))
RAMtoUNCLE     EQU ((MADAM+&04D0))
RAMtoEXTERNAL  EQU ((MADAM+&04E0))
RAMtoDSPPN     EQU ((MADAM+&04F0))
DSPPtoRAM0     EQU ((MADAM+&0500))
DSPPtoRAM1     EQU ((MADAM+&0510))
DSPPtoRAM2     EQU ((MADAM+&0520))
DSPPtoRAM3     EQU ((MADAM+&0530))
DMAtofrEXP0    EQU ((MADAM+&0540))
UNCLEtoRAM     EQU ((MADAM+&0550))
EXTERNALtoRAM  EQU ((MADAM+&0560))
RAMtofrPLAYER  EQU ((MADAM+&0570))
CLUTMIDctl     EQU ((MADAM+&0580))
CLUTMIDvideo   EQU ((MADAM+&0584))
CLUTMIDmidline EQU ((MADAM+&0588))
VIDMIDprev     EQU ((MADAM+&0590))
VIDMIDcur      EQU ((MADAM+&0594))
VIDMIDprevmid  EQU ((MADAM+&0598))
VIDMIDcurmid   EQU ((MADAM+&059C))
CelCtl0        EQU ((MADAM+&05A0))
NEXTPTR        EQU ((MADAM+&05A4))
CelCtlPLUT     EQU ((MADAM+&05A8))
CelCtlData     EQU ((MADAM+&05AC))
CelAddrA       EQU ((MADAM+&05B0))
CelLenA        EQU ((MADAM+&05B4))
CelAddrB       EQU ((MADAM+&05B8))
CelLenB        EQU ((MADAM+&05BC))
CommandGrabber EQU ((MADAM+&05C0))
FrameGrabber   EQU ((MADAM+&05D0))

;  === MATH registers and definitions ===   
MATH_STACK         EQU ((MADAM+&0600))

MATH_CONTROLSET    EQU ((MADAM+&07F0))
MATH_CONTROLCLEAR  EQU ((MADAM+&07F4))
MATH_ADELAY_MASK  EQU &00000003
MATH_EARLYTERM    EQU &00000004

MATH_STATUS        EQU ((MADAM+&07F8))
MATH_DONE         EQU &00000001
MATH_BANK         EQU &00000002
MATH_MACON        EQU &00000004
MATH_PREMIP       EQU &00000008

MATH_START         EQU ((MADAM+&07FC))
MATH_4X4         EQU &00000001
MATH_3X3         EQU &00000002
MATH_3X3_SCALED  EQU &00000004
MATH_CCB         EQU &00000008
MATH_CCB_FAST    EQU &00000010
MATH_BIGDIV      EQU &00000020



;  ===  ==============  =================================================   
;  ===                  =================================================   
;  ===  CLIO Registers  =================================================   
;  ===                  =================================================   
;  ===  ==============  =================================================   

CLIOREV   EQU ((CLIO+&0000))
CSYSBits  EQU ((CLIO+&0004))
Vint0     EQU ((CLIO+&0008))
Vint1     EQU ((CLIO+&000C))

AUDIN      EQU ((CLIO+&0020))
AUDOUT     EQU ((CLIO+&0024))
CSTATBits  EQU ((CLIO+&0028))
WDOG       EQU ((CLIO+&002C))
HCNT       EQU ((CLIO+&0030))
VCNT       EQU ((CLIO+&0034))
SEED       EQU ((CLIO+&0038))
RAND       EQU ((CLIO+&003C))

;  === Interrupt Registers ===   
;  also see these definitions below:
;     - Interrupt Word 0
;     - Interrupt Word 1
;     - BadBits
; 
SetInt0Bits    EQU ((CLIO+&0040))
ClrInt0Bits    EQU ((CLIO+&0044))
SetInt0EnBits  EQU ((CLIO+&0048))
ClrInt0EnBits  EQU ((CLIO+&004C))
SetMode        EQU ((CLIO+&0050))
ClrMode        EQU ((CLIO+&0054))
BadBits        EQU ((CLIO+&0058))

SetInt1Bits    EQU ((CLIO+&0060))
ClrInt1Bits    EQU ((CLIO+&0064))
SetInt1EnBits  EQU ((CLIO+&0068))
ClrInt1EnBits  EQU ((CLIO+&006C))

HDELAY  EQU ((CLIO+&0080))
HSYNC_DELAY_MASK  EQU &000000FF

ADBIO   EQU ((CLIO+&0084))
CLIO_RWPADS_MASK  EQU &0000000F
CLIO_RWDIR_MASK   EQU &000000F0

ADBCTL  EQU ((CLIO+&0088))
ADB_12P  EQU &00000000
ADB_24B  EQU &00000001

;  This structure is used when working with the timers   
	BEGINSTRUCT	 HardwareTimer 
		ULONG	timer_Register
		ULONG	timer_Backup
	ENDSTRUCT	;  The address is defined in an assembly file   

Timer0       EQU ((CLIO+&0100))
Timer0Back   EQU ((CLIO+&0104))
Timer1       EQU ((CLIO+&0108))
Timer1Back   EQU ((CLIO+&010C))
Timer2       EQU ((CLIO+&0110))
Timer2Back   EQU ((CLIO+&0114))
Timer3       EQU ((CLIO+&0118))
Timer3Back   EQU ((CLIO+&011C))
Timer4       EQU ((CLIO+&0120))
Timer4Back   EQU ((CLIO+&0124))
Timer5       EQU ((CLIO+&0128))
Timer5Back   EQU ((CLIO+&012C))
Timer6       EQU ((CLIO+&0130))
Timer6Back   EQU ((CLIO+&0134))
Timer7       EQU ((CLIO+&0138))
Timer7Back   EQU ((CLIO+&013C))
Timer8       EQU ((CLIO+&0140))
Timer8Back   EQU ((CLIO+&0144))
Timer9       EQU ((CLIO+&0148))
Timer9Back   EQU ((CLIO+&014C))
Timer10      EQU ((CLIO+&0150))
Timer10Back  EQU ((CLIO+&0154))
Timer11      EQU ((CLIO+&0158))
Timer11Back  EQU ((CLIO+&015C))
Timer12      EQU ((CLIO+&0160))
Timer12Back  EQU ((CLIO+&0164))
Timer13      EQU ((CLIO+&0168))
Timer13Back  EQU ((CLIO+&016C))
Timer14      EQU ((CLIO+&0170))
Timer14Back  EQU ((CLIO+&0174))
Timer15      EQU ((CLIO+&0178))
Timer15Back  EQU ((CLIO+&017C))

SetTm0  EQU ((CLIO+&0200))
ClrTm0  EQU ((CLIO+&0204))
SetTm1  EQU ((CLIO+&0208))
ClrTm1  EQU ((CLIO+&020C))

Slack  EQU ((CLIO+&0220))

FIFOINIT   EQU ((CLIO+&0300))
;  FIFO init group masks   
I_DMAtoDSPP_MASK  EQU &00001FFF
I_AUDtoDSPP_MASK  EQU &00006000
I_DSPPtoDMA_MASK  EQU &000F0000
I_DMAtoDSPP_SHIFT  EQU 0
I_AUDtoDSPP_SHIFT  EQU 13
I_DSPPtoDMA_SHIFT  EQU 16

;  also see DMA Enable Flags definitions below   
DMAREQEN   EQU ((CLIO+&0304))
DMAREQDIS  EQU ((CLIO+&0308))

FIFOstatus  EQU ((CLIO+&0380))

DMAtoDSPP0  EQU ((CLIO+&0380))
DMAtoDSPP1  EQU ((CLIO+&0384))
DMAtoDSPP2  EQU ((CLIO+&0388))
DMAtoDSPP3  EQU ((CLIO+&038C))
DMAtoDSPP4  EQU ((CLIO+&0390))
DMAtoDSPP5  EQU ((CLIO+&0394))
DMAtoDSPP6  EQU ((CLIO+&0398))
DMAtoDSPP7  EQU ((CLIO+&039C))
DMAtoDSPP8  EQU ((CLIO+&03A0))
DMAtoDSPP9  EQU ((CLIO+&03A4))
DMAtoDSPP10 EQU ((CLIO+&03A8))
DMAtoDSPP11 EQU ((CLIO+&03AC))
DMAtoDSPP12 EQU ((CLIO+&03B0))
AUDtoDSPPL  EQU ((CLIO+&03B4))
AUDtoDSPPR  EQU ((CLIO+&03B8))

DSPPtoDMA0  EQU ((CLIO+&03C0))
DSPPtoDMA1  EQU ((CLIO+&03C4))
DSPPtoDMA2  EQU ((CLIO+&03C8))
DSPPtoDMA3  EQU ((CLIO+&03CC))
DSPPtoDMA_COUNT_MASK  EQU &0000000F
DSPPtoDMA_UFLO_OFLO   EQU &00000010
DSPPtoDMA_EOD_FLUSH   EQU &00000020


;  === Expansion Registers ===   
;  Expansion control registers   
SetExpCtl  EQU ((CLIO+&0400))
ClrExpCtl  EQU ((CLIO+&0404))
ARMCTLRQ  EQU &00000040
ARMCTL    EQU &00000080
DMAWR     EQU &00000200
DMAACT    EQU &00000400
DMAON     EQU &00000800
DMARST    EQU &00004000
EXPRST    EQU &00008000

;  Expansion Types control registers   
EXP_TYPES  EQU ((CLIO+&0408))
TYPE4_MASK  EQU &000000FF
TYPE3_MASK  EQU &0000FF00
TYPE2_MASK  EQU &00FF0000
TYPE1_MASK  EQU &FF000000
TYPE4_CTLHOLD   EQU &00000003
TYPE4_STRBLOW   EQU &0000001C
TYPE4_CTLSETUP  EQU &000000E0
TYPE3_CTLHOLD   EQU &00000300
TYPE3_STRBLOW   EQU &00001C00
TYPE3_CTLSETUP  EQU &0000E000
TYPE2_CTLHOLD   EQU &00030000
TYPE2_STRBLOW   EQU &001C0000
TYPE2_CTLSETUP  EQU &00E00000
TYPE1_CTLHOLD   EQU &03000000
TYPE1_STRBLOW   EQU &1C000000
TYPE1_CTLSETUP  EQU &E0000000

EXP_XFRCNT  EQU ((CLIO+&0408))
DIPIR       EQU ((CLIO+&0410))
DPR2DEV_MASK  EQU &000000FF
OLDDPR2       EQU &00004000
DPR2          EQU &00008000
DPR1DEV_MASK  EQU &00FF0000
OLDDPR1       EQU &40000000
DPR1          EQU &80000000
DPR2DEV_SHIFT  EQU 0
DPR1DEV_SHIFT  EQU 16

EXP_SEL      EQU ((CLIO+&0500))
EXP_POLL     EQU ((CLIO+&0540))
EXP_CMDSTAT  EQU ((CLIO+&0580))
EXP_DATA     EQU ((CLIO+&05C0))

SEMAPHORE  EQU ((CLIO+&17D0))
SEMAACK    EQU ((CLIO+&17D4))

DSPPDMA   EQU ((CLIO+&17E0))
DSPPRST0  EQU ((CLIO+&17E4))
DSPPRST1  EQU ((CLIO+&17E8))

NOISE  EQU ((CLIO+&17F4))
DSPPPC  EQU ((CLIO+&17F4))
DSPPNR  EQU ((CLIO+&17F8))
DSPPGW  EQU ((CLIO+&17FC))

DSPPN32   EQU ((CLIO+&1800))
DSPPN16   EQU ((CLIO+&2000))
DSPPEI32  EQU ((CLIO+&3000))
DSPPEI16  EQU ((CLIO+&3400))
DSPPEO32  EQU ((CLIO+&3800))
DSPPEO16  EQU ((CLIO+&3C00))



;  ===  =================  ==============================================   
;  ===                     ==============================================   
;  ===  Other-1 registers  ==============================================   
;  ===                     ==============================================   
;  ===  =================  ==============================================   

Other1REV   EQU ((Other1+&0000))
Other1Bits  EQU ((Other1+&0004))



;  ===  =================  ==============================================   
;  ===                     ==============================================   
;  ===  Other-2 registers  ==============================================   
;  ===                     ==============================================   
;  ===  =================  ==============================================   

Other2REV   EQU ((Other2+&0000))
Other2Bits  EQU ((Other2+&0004))



;  ===  ===============  ================================================   
;  ===                   ================================================   
;  ===  UNCLE registers  ================================================   
;  ===                   ================================================   
;  ===  ===============  ================================================   

UNCLEREV  EQU ((UNCLE+&0000))
USYSBits  EQU ((UNCLE+&0004))



;  ===  ===============  ================================================   
;  ===                   ================================================   
;  ===  Trace Registers  ================================================   
;  ===                   ================================================   
;  ===  ===============  ================================================   

SRAM       EQU ((TRACE+&100000))
LINK_DATA  EQU ((TRACE+&1FFF00))
LINK_ADDR  EQU ((TRACE+&1FFF04))
LINK_FIFO  EQU ((TRACE+&1FFF08))
JOYSTICKS  EQU ((TRACE+&1FFF0C))



;  ===  =====================================  ==========================   
;  ===                                         ==========================   
;  ===  Bit definitions for various registers  ==========================   
;  ===                                         ==========================   
;  ===  =====================================  ==========================   

HardID_MASK  EQU &FF000000
MADAM_ID     EQU &01000000
CLIO_ID      EQU &02000000
UNCLE_ID     EQU &03000000

HardID_SHIFT  EQU 24

;  for all software rev. registers   
SoftRev_MASK    EQU &FF000000

SoftRev_SHIFT  EQU 24

;  RED RED RED from here to below   
;  === MSYSBits flag definitions ===   
BIST_MASK  EQU &00000E00
PBTEST     EQU &00001000

BIST_SHIFT  EQU 9

;  === MCTL flag definitions ===   
VRAMSIZE_MASK  EQU &00000007
DRAMSIZE_MASK  EQU &00000078
MCTL_unused0   EQU &00001F80
CLUTXEN        EQU &00002000
VSCTXEN        EQU &00004000
PLAYXEN        EQU &00008000
FENCEEN        EQU &00010000
SLIPXEN        EQU &00020000
MCTL_unused1   EQU &00040000
FENCOP         EQU &00080000
MCTL_unused2   EQU &00100000
CPUVEN         EQU &00200000

DRAMSIZE_SHIFT     EQU 3
VRAMSIZE_SHIFT     EQU 0

VRAMSIZE_SET1EMPTY   EQU &00000001
VRAMSIZE_SET1FULL    EQU &00000002

DRAMSIZE_SET0ONLY    EQU &00000008
DRAMSIZE_SET0SET1    EQU &00000010
DRAMSIZE_1MEG        EQU &00000000
DRAMSIZE_4MEG        EQU &00000020
DRAMSIZE_16MEG       EQU &00000040


;  RED RED RED from above to here   
;  BLUE BLUE BLUE from here to below   
;  === MSYSBits ===   
DRAMSETZ_MASK     EQU &00000078
SYSRAMSIZE_MASK   EQU &00000380
DISPMOD_MASK      EQU &00001C00
BIST_MASK_BLUE    EQU &000E0000

BIST_SHIFT_BLUE   EQU 17
DISPMOD_SHIFT     EQU 10
SYSRAMSIZE_SHIFT   EQU 7
DRAMSETZ_SHIFT     EQU 3

DRAMSETZ_1MEG        EQU &00000000
DRAMSETZ_4MEG        EQU &00000020
DRAMSETZ_16MEG       EQU &00000040

DISPMOD_320   EQU &00000000
DISPMOD_384   EQU &00000400
DISPMOD_512   EQU &00000800
DISPMOD_1024  EQU &00000C00
;  BLUE BLUE BLUE from above to here   


;  === AbortBits ===   
ABT_ROMF     EQU &00000001
ABT_ROMW     EQU &00000002
ABT_CLIOT    EQU &00000004
ABT_HARDU    EQU &00000008
;  RED RED RED from here to below   
ABT_unused4  EQU &00000010
;  RED RED RED from above to here   
;  BLUE BLUE BLUE from here to below   
ABT_SYSRAMU  EQU &00000010
;  BLUE BLUE BLUE from above to here   
ABT_FENCEV   EQU &00000020
ABT_VPR      EQU &00000040
ABT_R26E     EQU &00000080
ABT_SPSC     EQU &00000100
ABT_BITE     EQU &00000200
ABT_BADDEC   EQU &00000400
ABT_ARPS     EQU &00000800
ABT_BWACC    EQU &00001000


;  === PrivBits ===   
;  RED RED RED from here to below   
PRIV_unused0         EQU &00000001
PRIV_unused1         EQU &00000002
PRIV_unused2         EQU &00000004
;  RED RED RED from above to here   
;  BLUE BLUE BLUE from here to below   
PRIV_DMAtoSYSRAM     EQU &00000001
PRIV_SPORTtoSYSRAM   EQU &00000002
PRIV_REGIStoSYSRAM   EQU &00000004
;  BLUE BLUE BLUE from above to here   
PRIV_DMA_VRAMSIZE    EQU &00000008
PRIV_SPORT_VRAMSIZE  EQU &00000010
PRIV_REGIS_VRAMSIZE  EQU &00000020
PRIV_REGIS_MATH      EQU &00000040


;  === STATBits ===   
DIAGRESTART     EQU &00000001
DIPIRRESTART    EQU &00000002
BIST_STAT_MASK  EQU &0000000C
SPRON           EQU &00000010
SPRPAU          EQU &00000020
SPREND          EQU &00000040
SPRPRQ          EQU &00000080

BIST_STAT_SHIFT    EQU 2


;  === REGCTL0 ===   
G1_RMOD_MASK  EQU &0000000F
G2_RMOD_MASK  EQU &000000F0
G1_WMOD_MASK  EQU &00000F00
G2_WMOD_MASK  EQU &0000F000
RMOD_MASK     EQU (G1_RMOD_MASK+G2_RMOD_MASK)
WMOD_MASK     EQU (G1_WMOD_MASK+G2_WMOD_MASK)
RMOD_SHIFT    EQU 0
WMOD_SHIFT    EQU 8

;??? G1_RMOD32    EQU &00000001
;??? G1_RMOD256   EQU &00000004
;??? G1_RMOD1024  EQU &00000008
;??? G2_RMOD64    EQU &00000010
;??? G2_RMOD128   EQU &00000020
;??? G2_RMOD256   EQU &00000040
;??? G1_WMOD32    EQU &00000100
;??? G1_WMOD256   EQU &00000400
;??? G1_WMOD1024  EQU &00000800
;??? G2_WMOD64    EQU &00001000
;??? G2_WMOD128   EQU &00002000
;??? G2_WMOD256   EQU &00004000

G1_RMOD32    EQU &00000001
G1_RMOD512   EQU &00000002
G1_RMOD256   EQU &00000004
G1_RMOD1024  EQU &00000008
G2_RMOD64    EQU &00000010
G2_RMOD128   EQU &00000020
G2_RMODu6    EQU &00000040
G2_RMOD1024  EQU &00000080
G1_WMOD32    EQU &00000100
G1_WMOD512   EQU &00000200
G1_WMOD256   EQU &00000400
G1_WMOD1024  EQU &00000800
G2_WMOD64    EQU &00001000
G2_WMOD128   EQU &00002000
G2_WMODuE    EQU &00004000
G2_WMOD1024  EQU &00008000

RMOD_32    EQU (G1_RMOD32)
RMOD_64    EQU (G2_RMOD64)
RMOD_96    EQU (G2_RMOD64 + G1_RMOD32)
RMOD_128   EQU (G2_RMOD128)
RMOD_160   EQU (G2_RMOD128 + G1_RMOD32)
RMOD_256   EQU (G1_RMOD256)
RMOD_320   EQU (G1_RMOD256 + G2_RMOD64)
RMOD_384   EQU (G1_RMOD256 + G2_RMOD128)
RMOD_512   EQU (G1_RMOD512)
RMOD_576   EQU (G1_RMOD512 + G2_RMOD64)
RMOD_640   EQU (G1_RMOD512 + G2_RMOD128)
RMOD_1024  EQU (G1_RMOD1024)
RMOD_1056  EQU (G2_RMOD1024 + G1_RMOD32)
RMOD_1088  EQU (G1_RMOD1024 + G2_RMOD64)
RMOD_1152  EQU (G1_RMOD1024 + G2_RMOD128)
RMOD_1280  EQU (G2_RMOD1024 + G1_RMOD256)
RMOD_1536  EQU (G2_RMOD1024 + G1_RMOD512)
RMOD_2048  EQU (G1_RMOD1024 + G2_RMOD1024)

WMOD_32    EQU (G1_WMOD32)
WMOD_64    EQU (G2_WMOD64)
WMOD_96    EQU (G2_WMOD64 + G1_WMOD32)
WMOD_128   EQU (G2_WMOD128)
WMOD_160   EQU (G2_WMOD128 + G1_WMOD32)
WMOD_256   EQU (G1_WMOD256)
WMOD_320   EQU (G1_WMOD256 + G2_WMOD64)
WMOD_384   EQU (G1_WMOD256 + G2_WMOD128)
WMOD_512   EQU (G1_WMOD512)
WMOD_576   EQU (G1_WMOD512 + G2_WMOD64)
WMOD_640   EQU (G1_WMOD512 + G2_WMOD128)
WMOD_1024  EQU (G1_WMOD1024)
WMOD_1056  EQU (G2_WMOD1024 + G1_WMOD32)
WMOD_1088  EQU (G1_WMOD1024 + G2_WMOD64)
WMOD_1152  EQU (G1_WMOD1024 + G2_WMOD128)
WMOD_1280  EQU (G2_WMOD1024 + G1_WMOD256)
WMOD_1536  EQU (G2_WMOD1024 + G1_WMOD512)
WMOD_2048  EQU (G1_WMOD1024 + G2_WMOD1024)


;  === REGCTL1 ===   
REG_XCLIP_MASK  EQU &000007FF
REG_YCLIP_MASK  EQU &07FF0000

REG_XCLIP_SHIFT  EQU 0
REG_YCLIP_SHIFT  EQU 16


;  === AUDIN ===   
AUDIN_FORMAT    EQU &00000001
AUDIN_ENABLE    EQU &00000002


;  === AUDOUT ===   
AUDOUT_ENABLE        EQU &80000000
AUDOUT_LEFTHIGH      EQU &40000000
AUDOUT_POSHIGH       EQU &20000000
AUDOUT_BITCLKR_MASK  EQU &000F0000
AUDOUT_WRDSTRT_MASK  EQU &0000FF00
AUDOUT_WRDREL_MASK   EQU &000000FF

AUDOUT_BITCLKR_SHIFT  EQU 16
AUDOUT_WRDSTRT_SHIFT  EQU 8
AUDOUT_WRDREL_SHIFT   EQU 0


;  === CSTATbits ===   
CSTAT_PON        EQU &00000001
CSTAT_ROLLOVER   EQU &00000002
CSTAT_WDOGpin    EQU &00000004
CSTAT_WDOGcntr   EQU &00000008
CSTAT_BIST_MASK  EQU &00000E00

CSTAT_BIST_SHIFT  EQU 9


;  === VCNT ===   
VCNT_MASK   EQU &000007FF
VCNT_FIELD  EQU &00000800

VCNT_SHIFT        EQU 0
VCNT_FIELD_SHIFT  EQU 11


;  === SEED ===   
SEED_MSB_MASK  EQU &0FFF0000
SEED_LSB_MASK  EQU &00000FFF

SEED_MSB_SHIFT  EQU 16
SEED_LSB_SHIFT  EQU 0

;  === Interrupt word 0 ===   
INT0_VINT0     EQU &00000001
INT0_VINT1     EQU &00000002
INT0_EXINT     EQU &00000004
INT0_TIMINT15  EQU &00000008
INT0_TIMINT13  EQU &00000010
INT0_TIMINT11  EQU &00000020
INT0_TIMINT9   EQU &00000040
INT0_TIMINT7   EQU &00000080
INT0_TIMINT5   EQU &00000100
INT0_TIMINT3   EQU &00000200
INT0_TIMINT1   EQU &00000400
INT0_DSPPINT   EQU &00000800
INT0_DDRINT0   EQU &00001000
INT0_DDRINT1   EQU &00002000
INT0_DDRINT2   EQU &00004000
INT0_DDRINT3   EQU &00008000
INT0_DRDINT0   EQU &00010000
INT0_DRDINT1   EQU &00020000
INT0_DRDINT2   EQU &00040000
INT0_DRDINT3   EQU &00080000
INT0_DRDINT4   EQU &00100000
INT0_DRDINT5   EQU &00200000
INT0_DRDINT6   EQU &00400000
INT0_DRDINT7   EQU &00800000
INT0_DRDINT8   EQU &01000000
INT0_DRDINT9   EQU &02000000
INT0_DRDINT10  EQU &04000000
INT0_DRDINT11  EQU &08000000
INT0_DRDINT12  EQU &10000000
INT0_DEXINT    EQU &20000000
INT0_SOFTWARE  EQU &40000000
INT0_SCNDPINT  EQU &80000000


;  === Interrupt word 1 ===   
INT1_PLYINT         EQU &00000001
INT1_DIPIR          EQU &00000002
INT1_PDINT          EQU &00000004
INT1_RAMtoDSPPN     EQU &00000008
INT1_DMAtoUNCLE     EQU &00000010
INT1_DMAfrUNCLE     EQU &00000020
INT1_DMAtoEXTERNAL  EQU &00000040
INT1_DMAfrEXTERNAL  EQU &00000080
INT1_BadBits        EQU &00000100
INT1_DSPPUNDER      EQU &00000200
INT1_DSPPOVER       EQU &00000400


;  === BadBits ===   
BB_UFLO_MASK     EQU &00007FFF
BB_OFLO_MASK     EQU &000F0000
BB_PRIV          EQU &00100000
BB_DMANFW        EQU &00200000
BB_DMANFW_MASK   EQU &07C00000
BB_DMANFW_DIR    EQU &08000000

BB_UFLO_SHIFT    EQU 0
BB_OFLO_SHIFT    EQU 16
BB_DMANFW_SHIFT  EQU 22


;  === FIFO status ===   
FIFO_Flush      EQU &00000020
FIFO_FullEmpty  EQU &00000010
FIFO_OverUnder  EQU &00000008
FIFO_Count2     EQU &00000004
FIFO_Count1     EQU &00000002
FIFO_Count0     EQU &00000001


;  == DMA Enable Flags ===   
EN_DMAtoDSPP_MASK  EQU &00001FFF
EN_DMAtoUNCLE      EQU &00002000
EN_DMAtoEXTERNAL   EQU &00004000
EN_DMAtoDSPP_PROG  EQU &00008000
EN_DSPPtoDMA_MASK  EQU &000F0000
EN_DMAtofrEXP      EQU &00100000
EN_UNCLEtoDMA      EQU &20000000
EN_EXTERNALtoDMA   EQU &40000000


	ENDIF	  ; of #ifdef __INTHARD_H   


	END

