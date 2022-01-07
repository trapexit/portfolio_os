/* $Id: clio.h,v 1.21 1994/09/14 01:02:58 deborah Exp $ */
#ifndef __CLIO_H
#define __CLIO_H

/*
	Copyright (C) 1993, The 3DO Company, Inc.
	All Rights Reserved
	Confidential and Proprietary
*/
/* NOTE NOTE: ASIC_ANVIL is TEMPORARY - do not depend on it! */

#pragma force_top_level
#pragma include_only_once

#include "types.h"

/* structures defining Clio hardware registers */

typedef struct ExpansionBus
{
	vuint32			xb_SetExpCtl;	/* +00 */
	vuint32			xb_ClrExpCtl;	/* +04 */
	vuint32			xb_ExpType;	/* +08 */
	vuint32			xb_XferCnt;	/* +0c */
	vuint32			xb_DIPIR;	/* both for red, first for green */
	vuint32			xb_DIPIR2;	/* second for green */
	char                    xb_Reserved0x418[0x500 - 0x418];
	vuint32			xb_Sel[16];	/* +100 - +13f */
	vuint32			xb_Poll[16];	/* +140 - +17f */
	vuint32			xb_CmdStat[16];	/* +180 - +1bf */
	vuint32			xb_Data[16];	/* +1c0 - +1ff */
	char                    xb_Reserved0x600[0xC00 - 0x600];
}                       ExpansionBus;


/* ExpansionBus->xb_SetExpCtl defines */
#define XB_CPUWANTXBUS	(1<<6)		/* R, CPU requests expansion bus */
#define XB_CPUHASXBUS	(1<<7)		/* R/clr CPU has bus */
#define XB_DmadirectION	(1<<9)		/* R/W DMA Direction 1=MEM->EXP */
#define XB_DMAISOUT	(1<<9)		/* memory to exp */
#define XB_DMAISIN	(0)		/* exp to memory */
#define XB_DMAACTIVE	(1<<10)		/* DMA possibly active */
#define XB_DMAON	(1<<11)		/* dma turned on */
#define XB_DMARESET	(1<<14)		/* also does a flush */
#define XB_EXPRESET	(1<<15)		/* reset expansion bus */

/* ExpansionBus->xb_ExpType defines */
#define XB_TYPEMask	0xFF
#define XB_TYPE4Shift	0
#define XB_TYPE3Shift	8
#define XB_TYPE2Shift	16
#define XB_TYPE1Shift	24

/* in each 8bit xb_ExpType group these are the bits */
#define XB_ControlHoldShift	0	/* bits 0,1 set Control Hold */
#define XB_StrobeLowShift	2	/* bits 2,3,4 set Strobe low duration */
#define XB_ControlSetupShift	5	/* bits 5,6,7 set Control setup */

#define XBUSTYPE(a,b,c) ((a<<XB_ControlSetupShift)| \
			(b<<XB_StrobeLowShift)| \
			(c<<XB_ControlHoldShift))

#define XTYPE1(cs,sl,ch)	(XBUSTYPE(cs,sl,ch)<<XB_TYPE1Shift)
#define XTYPE2(cs,sl,ch)	(XBUSTYPE(cs,sl,ch)<<XB_TYPE2Shift)
#define XTYPE3(cs,sl,ch)	(XBUSTYPE(cs,sl,ch)<<XB_TYPE3Shift)
#define XTYPE4(cs,sl,ch)	(XBUSTYPE(cs,sl,ch)<<XB_TYPE4Shift)

/* ExpansionBus->xb_DIPIR defines */
#define XB_DipirMask	0xFF		/* dipir device # */
#define XB_DipirNOSR	0x4000		/* Dipir occured b4 soft reset */
#define XB_DipirCurrent	0x8000

#define XB_Dipir1Shift	16
#define XB_Dipir2Shift	0

/* ExpansionBus->xb_Sel defines (bits in select register) */
#define XBUS_EXTERNAL	0x80

/* ExpansionBus->xb_Poll defines */
#define XBUS_POLL_STAT_INT_EN	1			/* r/w */
#define XBUS_POLL_READ_INT_EN	2			/* r/w */
#define XBUS_POLL_WRIT_INT_EN	4			/* r/w */
#define XBUS_POLL_RESET		8			/* r/w */
#define XBUS_POLL_STAT_VALID	0x10			/* ro */
#define XBUS_POLL_READ_VALID	0x20			/* ro */
#define XBUS_POLL_WRIT_VALID	0x40			/* ro */
#define XBUS_POLL_MEDIA_ACCESS	0x80			/* r/clear */

#define XBUS_INTEN_BITS	(XBUS_POLL_STAT_INT_EN \
			|XBUS_POLL_READ_INT_EN|XBUS_POLL_WRIT_INT_EN)
#define XBUS_INTRQ_BITS	(XBUS_POLL_STAT_VALID \
			|XBUS_POLL_READ_VALID|XBUS_POLL_WRIT_VALID)

#define XBUS_POLL_DEV_OVERFLOW	0x80		/* too many devices */
#define XBUS_POLL_NEWDEV	0x10		/* new device plugged in */
#define XBUS_POLL_NEWDEV_INT_EN	0x01


typedef struct OldWoodyRegs
{
	vuint32			fmv_FMVID;
	vuint32			fmv_AddressReg;
	vuint32			fmv_ROMReg;
	vuint32			fmv_ControlSet;
	vuint32			fmv_ControlClr;
	vuint32			fmv_Status;
	vuint32			fmv_AudioReg;
	vuint32			fmv_VideoReg;
	vuint32			fmv_SizeReg;
	vuint32			fmv_VidControl1;
	vuint32			fmv_VidControl2;
	vuint32			fmv_LFSRReg;
}                       OldWoodyRegs;

/* UNCLEID by itself identifies the old fmv card */
/* all other valid uncle cards will have some bits in */
/* the bit8-bit23 range being 1 */

typedef struct UncleRegs
{
	vuint32			unc_IdRevBits;
	vuint32			unc_SysBits;
	vuint32			unc_AddressReg;
	vuint32			unc_DataReg;
}                       UncleRegs;

/* unc_RevBits */
#define UNC_NOROM	0x00800000

/* for controlling the hardware timers */

typedef struct HardTimer
{
	vuint32			ht_cnt;
	vuint32			ht_cntBack;
}                       HardTimer;

typedef struct HardTimerControl
{
	vuint32			htc_Set;
	vuint32			htc_Clr;
}                       HardTimerControl;

typedef struct ClioInterrupts
{
	vuint32			SetIntBits;	/* +00 */
	vuint32			ClrIntBits;	/* +04 */
	vuint32			SetIntEnBits;	/* +08 */
	vuint32			ClrIntEnBits;	/* +0c */
} ClioInterrupts;


typedef struct Clio
{
/* 0000: Basic Control Words */
	vuint32			ClioRev;	/* 0000: Clio revision */
	vuint32			CSysBits;	/* 0004: System control bits */
	vuint32			VInt0;		/* 0008: V0 scan line */
	vuint32			VInt1;		/* 000c: V1 scan line */
#ifndef ASIC_ANVIL
	vuint32			MultiChip[4];	/* 0010 - 001F: Multi-chip */
#else /*ASIC_ANVIL*/
	vuint32			ClioDigVidEnc;	/* 0010: Digital vid encoder */
	vuint32			ClioSbusState;	/* 0014: Sbus state at reset */
	char			Reserved0x18[0x20 - 0x18];
#endif /*ASIC_ANVIL*/
	vuint32			AudioIn;	/* 0020: Aud-in format */
	vuint32			AudioOut;	/* 0024: Aud-out control */
	vuint32			CStatBits;	/* 0028: Clio system bits */
	vuint32			WatchDog;	/* 002c: Watchdog timer */
	vuint32			HCnt;		/* 0030: Horiz count */
	vuint32			VCnt;		/* 0034: Vert count */
	vuint32			RandSeed;	/* 0038: video seed */
	vuint32			RandSample;	/* 003c: video rand value */
/* 0040: Interrupt Stuff */
	ClioInterrupts          ClioInt0;	/* 0040 - 004f */
	vuint32			SetMode;	/* 0050: */
	vuint32			ClrMode;	/* 0054: */
	vuint32			BadBits;	/* 0058: */
	vuint32			Spare;		/* 005c: */
	ClioInterrupts          ClioInt1;	/* 0060 - 006f */
	char                    Reserved0x70[0x80 - 0x70];	/* reserved for ClioInt2 */
	vuint32			HDelay;		/* 0080: */
	vuint32			ADBIOBits;	/* 0084: */
	vuint32			ADBCTLBits;	/* 0088: */
	char                    Reserved0x8C[0x90 - 0x8C];
	char                    Reserved0x90[0x100 - 0x90];
/* 0100: Timers */
	HardTimer               Timers[16];		/* 0100 - 017f */
	char                    Reserved0x180[0x200 - 0x180];
	HardTimerControl        TimerControl[2];	/* 0200 - 020f */
	char                    Reserved0x210[0x220 - 0x210];
	vuint32			TimerSlack;		/* 0220: */
	char                    Reserved0x224[0x230 - 0x224];
	char                    Reserved0x230[0x300 - 0x230];
/* 0300: FIFO Stuff */
	vuint32			FifoInit;		/* 0300: */
	vuint32			SetDMAEnable;		/* 0304: */
	vuint32			ClrDMAEnable;		/* 0308: */
	char                    Reserved0x30C[0x310 - 0x30C];
	char                    Reserved0x310[0x380 - 0x310];
	vuint32			clio_FIFOStatusDMAtoDSPP[16]; /* 0380 */
	vuint32			clio_FIFOStatusDSPPtoDMA[16]; /* 03c0 */
/* 0400: Expansion Bus */
	ExpansionBus            clio_ExpansionBus;	/* 0400 - 0c00 */
	char                    Reserved0xC00[0x1700 - 0xC00];
/* 1700: DSPP */
	char			Reserved0x1700[0x17D0-0x1700];
	vuint32			clio_DSPPSemaphore;	/* 17d0 */
	vuint32			clio_DSPPSemaAck;	/* 17d4 */
	char                    Reserved0x17D8[0x17E0 - 0x17D8];
	vuint32			clio_DSPPDMA;		/* 17e0 */
	vuint32			clio_DSPPTickReset0;	/* 17e4 */
	vuint32			clio_DSPPTickReset1;	/* 17e8 */
	char                    Reserved0x17EC[0x17F0 - 0x17EC];
	vuint32			clio_DSPPNoise;		/* 17f0 */
	vuint32			clio_DSPPPC;		/* 17f4 */
	vuint32			clio_DSPPNR;		/* 17f8 */
	vuint32			clio_DSPPGW;		/* 17fc */
	vuint32			clio_DSPPN32bit[(0x2000 - 0x1800) / sizeof (vuint32)];
	vuint32			clio_DSPPN16bit[(0x3000 - 0x2000) / sizeof (vuint32)];
	vuint32			clio_DSPPEI32bit[(0x3200 - 0x3000) / sizeof (vuint32)];
	char                    Reserved0x3200[0x3400 - 0x3200];
	vuint32			clio_DSPPEI16bit[(0x3800 - 0x3400) / sizeof (vuint32)];
	vuint32			clio_DSPPEO32bit[(0x3A00 - 0x3800) / sizeof (vuint32)];
	char                    Reserved0x3A00[0x3C00 - 0x3A00];
	vuint32			clio_DSPPEO16bit[(0x4000 - 0x3C00) / sizeof (vuint32)];
/* 4000: Other-1 */
	vuint32			clio_Other1Rev;		/* 4000 */
	vuint32			clio_Other1Bits;	/* 4004 */
	char                    Reserved0x4008[0x8000 - 0x4008];
/* 8000: Other-2 */
	vuint32			clio_Other2Rev;		/* 8000 */
	vuint32			clio_Other2Bits;	/* 8004 */
	char                    Reserved0x8008[0xC000 - 0x8008];
/* C000: UNCLE, WOODY */
	vuint32			clio_UncleRev;		/* c000 */
	vuint32			clio_UncleBits;		/* AKA Woody "SoftRev" */
	vuint32			clio_WoodyAddr;		/* c008 */
	vuint32			clio_WoodyRomReg;	/* c00c */
	char                    Reserved0xC010[0x10000 - 0xC010];
} Clio;

/* Clio->ClioRev definitions */
#define CLIO_RED	0x02010000	/* obsolete */
#define CLIO_REDWW	0x02010001	/* obsolete */
#define CLIO_GREEN	0x02020000	/* AT&T Green */
#define CLIO_GREENWW	0x02020001	/* obsolete */
#define CLIO_PREEN	0x02022000	/* Toshiba/MEC Preen */
#ifdef ASIC_ANVIL
#define CLIO_ANVIL	0x04000000	/* MEC Anvil */
#endif /*ASIC_ANVIL*/

/* Clio->ClioDigVidEnc definitions */
#ifdef ASIC_ANVIL
#define	CLIO_RTCMode0		0x00000001	/* Enable RTC Mode */
#define	CLIO_GenLock0		0x00000002	/* Enable GenLock */
#define	CLIO_FilterMode		0x00000004	/* Select enhanced BW filters */

#define	CLIO_PixModeMask	0x00000018	/* Pixel Mode bits */
#define	CLIO_PixModeRGB		0x00000000
#define	CLIO_PixModeYUV		0x00000008
#define	CLIO_PixModeTest	0x00000010
#define	CLIO_PixModeDith	0x00000018

#define	CLIO_VidModePgrsv	0x00000020	/* Progressive/Interlaced Scan */
#define	CLIO_VidModePAL		0x00000040	/* PAL/NTSC */
#define	CLIO_VidModeRGB		0x00000080	/* Set for RGB */
#define	CLIO_VidModeMask (CLIO_VidModeRGB | CLIO_VidModePgrsv | CLIO_VidModePAL)
#define	CLIO_DVEBypass		0x00000300	/* Bypass digital vid encoder */
#define	CLIO_Transparency	0x00000400	/* Enable transparency */
#define	CLIO_SyncDir		0x00000800	/* Amy syncs as outputs */
/*				0x00001000	   Not Used */
#define	CLIO_ResetDVE		0x00002000	/* Reset Dig video encoder */
#define	CLIO_ColorKill		0x00004000	/* Kill color */
#endif /*ASIC_ANVIL*/

#ifdef ASIC_ANVIL
/* Clio->ClioSbusState definitions */
/* Bit layout for Anvil's ClioSbusState register */
/* NOTE: value for initial units is 0xFFFFFFFF */
/* Invert the ClioSbusState before using. */

#define	CLIO_VRAM2ndBankSize	0x00000001	/* VRAM 2nd bank size in MB */
#define	CLIO_DRAMConfig		0x0000000e	/* DRAM configuration bits */
#define	CLIO_VideoMode		0x00000040	/* Video mode, 0=NTSC, 1=PAL */
#define	CLIO_AudioConfig	0x00000300	/* Audio configuration bits */
#define	CLIO_DfltSystemLanguage	0x00007000	/* Default system lang bits */
#define	CLIO_DfltSystemLangShft	12		/* Shift for system lang bits */
#endif /*ASIC_ANVIL*/

/* Clio->CStatBits definitions */
#define CLIO_PwrON	1
#define CSTAT_PON        0x00000001
#define CLIO_RstOD	2
#define CSTAT_ROLLOVER   0x00000002
#define CLIO_WatchDog	4		/* meaningless on Anvil */
#define CSTAT_WDOGpin    0x00000004	/* meaningless on Anvil */
#define CLIO_WatchDogCtr	8	/* meaningless on Anvil */
#define CSTAT_WDOGcntr   0x00000008	/* meaningless on Anvil */
#define CLIO_SoftReset		0x10
#define CLIO_ClearDIPIR		0x20
#define CLIO_DIPIRReset		0x40
#define CSTAT_BIST_MASK  0x00000E00
#define CSTAT_BIST_SHIFT  9

/* Clio->AudioIn definitions */
#define AUDIN_FORMAT    0x00000001
#define AUDIN_ENABLE    0x00000002
#ifdef ASIC_ANVIL
#define	CLIO_AudioInReserved	0x0000ffff
#define	CLIO_AudInStartOffset	0x00ff0000	/* Audio input skipped mask */
#define	CLIO_AudOutSyncEn	0x04000000	/* reset aud-out sync HW */
#define	CLIO_AudInClockPol	0x08000000	/* AUDBCK polarity used */
						/* 0: neg edge, 1: pos edge */
#define	CLIO_AudInLeftFirst	0x10000000	/* Left chan data first */
#define	CLIO_AudInLeftHigh	0x20000000	/* Left chan with high AUDWS */
#define	CLIO_AudInInputEn	0x40000000	/* Enable Audio input */
						/* cleared by reset and dipir */
#define	CLIO_AudInConfigEn	0x80000000	/* Enable writing AudIn bits */
#endif /*ASIC_ANVIL*/

/* Clio->AudioOut definitions */
#define AUDOUT_ENABLE        0x80000000
#define AUDOUT_LEFTHIGH      0x40000000
#define AUDOUT_POSHIGH       0x20000000
#define AUDOUT_BITCLKR_MASK  0x000F0000
#define AUDOUT_WRDSTRT_MASK  0x0000FF00
#define AUDOUT_WRDREL_MASK   0x000000FF
#define AUDOUT_BITCLKR_SHIFT  16
#define AUDOUT_WRDSTRT_SHIFT  8
#define AUDOUT_WRDREL_SHIFT   0

/* Clio->VCnt definitions */
#define VCNT_MASK   0x000007FF
#define VCNT_FIELD  0x00000800
#define VCNT_SHIFT        0
#define VCNT_FIELD_SHIFT  11

/* Clio->RandSeed definitions */
#define SEED_MSB_MASK  0x0FFF0000
#define SEED_LSB_MASK  0x00000FFF
#define SEED_MSB_SHIFT  16
#define SEED_LSB_SHIFT  0

/* Clio->ClioInt0 definitions */
#define INT0_VINT0     0x00000001
#define INT0_VINT1     0x00000002
#define INT0_EXINT     0x00000004
#define INT0_TIMINT15  0x00000008
#define INT0_TIMINT13  0x00000010
#define INT0_TIMINT11  0x00000020
#define INT0_TIMINT9   0x00000040
#define INT0_TIMINT7   0x00000080
#define INT0_TIMINT5   0x00000100
#define INT0_TIMINT3   0x00000200
#define INT0_TIMINT1   0x00000400
#define INT0_DSPPINT   0x00000800
#define INT0_DDRINT0   0x00001000
#define INT0_DDRINT1   0x00002000
#define INT0_DDRINT2   0x00004000
#define INT0_DDRINT3   0x00008000
#define INT0_DRDINT0   0x00010000
#define INT0_DRDINT1   0x00020000
#define INT0_DRDINT2   0x00040000
#define INT0_DRDINT3   0x00080000
#define INT0_DRDINT4   0x00100000
#define INT0_DRDINT5   0x00200000
#define INT0_DRDINT6   0x00400000
#define INT0_DRDINT7   0x00800000
#define INT0_DRDINT8   0x01000000
#define INT0_DRDINT9   0x02000000
#define INT0_DRDINT10  0x04000000
#define INT0_DRDINT11  0x08000000
#define INT0_DRDINT12  0x10000000
#define INT0_DEXINT    0x20000000
#define INT0_SOFTWARE  0x40000000
#define INT0_SCNDPINT  0x80000000

/* Clio->BadBits definitions */
#define BB_UFLO_MASK     0x00007FFF
#define BB_OFLO_MASK     0x000F0000
#define BB_PRIV          0x00100000
#define BB_DMANFW        0x00200000
#define BB_DMANFW_MASK   0x07C00000
#define BB_DMANFW_DIR    0x08000000
#define BB_UFLO_SHIFT    0
#define BB_OFLO_SHIFT    16
#define BB_DMANFW_SHIFT  22

/* Clio->ClioInt1 definitions */
#define INT1_PLYINT		0x00000001
#define INT1_DIPIR		0x00000002
#define INT1_PDINT		0x00000004
#define INT1_RAMtoDSPPN		0x00000008
#define INT1_DMAtoUNCLE		0x00000010
#define INT1_DMAfrUNCLE		0x00000020
#define INT1_DMAtoEXTERNAL	0x00000040
#define INT1_DMAfrEXTERNAL	0x00000080
#define INT1_BadBits		0x00000100
#define INT1_DSPPUNDER		0x00000200
#define INT1_DSPPOVER		0x00000400
#ifdef ASIC_ANVIL
#define	INT1_DMAfrCD0		0x00000800
#define	INT1_DMAfrCD1		0x00001000
#define	INT1_CDOVERFLOW		0x00002000
#endif /*ASIC_ANVIL*/

/* Clio->HDelay definitions */
#define HSYNC_DELAY_MASK  0x000000FF

/*
 * Clio->ADBIOBits Definitions
 * The bits in the lowest nibble control various functions.
 * The enable bits in the next nibble control whether the associated control
 * bits are inputs or outputs. The default (wakeup) condition
 * is all bits as inputs.
 *
 * Current Assignments:
 *
 *	adbio_zero: May be NTSC/PAL in future.
 *	adbio_one: Audio mute output, used in cdipir.c
 *	adbio_two: Alternate ROM bank select, used in ramdevice.c
 *	adbio_three: Watchdog reset output, used in cdipir.c
 */
#define	ADBIO_BIT0		(0x01)			/* UNUSED */
#define	ADBIO_BIT0_EN		(ADBIO_BIT0 << 4)	/* UNUSED */
#define	ADBIO_AUDIO_MUTE	(0x02)
#define	ADBIO_AUDIO_MUTE_EN	(ADBIO_AUDIO_MUTE << 4)
#define	ADBIO_OTHERROM		(0x04)
#define	ADBIO_OTHERROM_EN	(ADBIO_OTHERROM << 4)
#define	ADBIO_WATCHDOG		(0x08)
#define	ADBIO_WATCHDOG_EN	(ADBIO_WATCHDOG << 4)
#define CLIO_RWPADS_MASK  0x0000000F
#define CLIO_RWDIR_MASK   0x000000F0

/* Clio->ADBCTLBits definitions */
#define ADB_12P  0x00000000
#define ADB_24B  0x00000001

/* Clio->FifoInit definitions */
/* FIFO init group masks */
#define I_DMAtoDSPP_MASK  0x00001FFF	/* DMA->DSPP[0:12] */
#define I_AUDtoDSPP_MASK  0x00006000	/* AudIn->DSPP[L:R] */
#define I_DSPPtoDMA_MASK  0x000F0000	/* DSPP->DMA[0:3] */
#define I_DMAtoDSPP_SHIFT  0
#define I_AUDtoDSPP_SHIFT  13
#define I_DSPPtoDMA_SHIFT  16

/* Clio->SetDMAEnable defintions */
#define EN_DMAtoDSPP_MASK  0x00001FFF
#define EN_DMAtoUNCLE      0x00002000
#define EN_DMAtoEXTERNAL   0x00004000
#define EN_DMAtoDSPP_PROG  0x00008000
#define EN_DSPPtoDMA_MASK  0x000F0000
#define EN_DMAtofrEXP      0x00100000
#define EN_UNCLEtoDMA      0x20000000
#define EN_EXTERNALtoDMA   0x40000000

/* Clio->ClrDMAEnable defintions */
#ifdef ASIC_ANVIL
#define EN_CD0toDMA	   0x00200000
#define EN_CD1toDMA	   0x00400000
#endif /*ASIC_ANVIL*/

/*
 * Clio->clio_FIFOStatusDMAtoDSPP and
 * Clio->clio_FIFOStatusDSPPtoDMA definitions
 */
#define DSPPtoDMA_COUNT_MASK  0x0000000F
#define FIFO_Flush      0x00000020
#define DSPPtoDMA_EOD_FLUSH   0x00000020
#define FIFO_FullEmpty  0x00000010
#define DSPPtoDMA_UFLO_OFLO   0x00000010
#define FIFO_OverUnder  0x00000008
#define FIFO_Count2     0x00000004
#define FIFO_Count1     0x00000002
#define FIFO_Count0     0x00000001

/* Clio->clio_Other1Bits definitions */
#define ARMCTLRQ  0x00000040
#define ARMCTL    0x00000080
#define DMAWR     0x00000200
#define DMAACT    0x00000400
#define DMAON     0x00000800
#define DMARST    0x00004000
#define EXPRST    0x00008000

/* Clio->clio_XbusTypes definitions */
#define TYPE4_MASK  0x000000FF
#define TYPE3_MASK  0x0000FF00
#define TYPE2_MASK  0x00FF0000
#define TYPE1_MASK  0xFF000000
#define TYPE4_CTLHOLD   0x00000003
#define TYPE4_STRBLOW   0x0000001C
#define TYPE4_CTLSETUP  0x000000E0
#define TYPE3_CTLHOLD   0x00000300
#define TYPE3_STRBLOW   0x00001C00
#define TYPE3_CTLSETUP  0x0000E000
#define TYPE2_CTLHOLD   0x00030000
#define TYPE2_STRBLOW   0x001C0000
#define TYPE2_CTLSETUP  0x00E00000
#define TYPE1_CTLHOLD   0x03000000
#define TYPE1_STRBLOW   0x1C000000
#define TYPE1_CTLSETUP  0xE0000000

/* Clio->clio_DipirInfo defines */
#define DPR2DEV_MASK  0x000000FF
#define OLDDPR2       0x00004000
#define DPR2          0x00008000
#define DPR1DEV_MASK  0x00FF0000
#define OLDDPR1       0x40000000
#define DPR1          0x80000000
#define DPR2DEV_SHIFT  0
#define DPR1DEV_SHIFT  16

/* Clio->clio_UncleRev defines */
#define clio_Woody	clio_UncleRev;
#define FMVID		0
#define WOODY_ID	0x03000001
#define UNCLE_ID_MASK	0x00FF0000
#define UNCLE_REV_MASK	0x0000FF00

/* Sub-element Convenience Definitions */

/* Elements of Clio->clio_FIFOStatusDMAtoDSPP */
#define DMAtoDSPP0	Clio->clio_FIFOStatusDMAtoDSPP[0]
#define DMAtoDSPP1	Clio->clio_FIFOStatusDMAtoDSPP[1]
#define DMAtoDSPP2	Clio->clio_FIFOStatusDMAtoDSPP[2]
#define DMAtoDSPP3	Clio->clio_FIFOStatusDMAtoDSPP[3]
#define DMAtoDSPP4	Clio->clio_FIFOStatusDMAtoDSPP[4]
#define DMAtoDSPP5	Clio->clio_FIFOStatusDMAtoDSPP[5]
#define DMAtoDSPP6	Clio->clio_FIFOStatusDMAtoDSPP[6]
#define DMAtoDSPP7	Clio->clio_FIFOStatusDMAtoDSPP[7]
#define DMAtoDSPP8	Clio->clio_FIFOStatusDMAtoDSPP[8]
#define DMAtoDSPP9	Clio->clio_FIFOStatusDMAtoDSPP[9]
#define DMAtoDSPP10	Clio->clio_FIFOStatusDMAtoDSPP[10]
#define DMAtoDSPP11	Clio->clio_FIFOStatusDMAtoDSPP[11]
#define DMAtoDSPP12	Clio->clio_FIFOStatusDMAtoDSPP[12]
#define AUDtoDSPPL	Clio->clio_FIFOStatusDMAtoDSPP[13]
#define AUDtoDSPPR	Clio->clio_FIFOStatusDMAtoDSPP[14]

/* Elements of Clio->clio_FIFOStatusDSPPtoDMA */
#define DSPPtoDMA0	Clio->clio_FIFOStatusDSPPtoDMA[0]
#define DSPPtoDMA1	Clio->clio_FIFOStatusDSPPtoDMA[1]
#define DSPPtoDMA2	Clio->clio_FIFOStatusDSPPtoDMA[2]
#define DSPPtoDMA3	Clio->clio_FIFOStatusDSPPtoDMA[3]

/*
 * FOR COMPATABILITY ONLY
 *
 * These are the old names from inthard.h redefined
 * to use the structure definitions.
 * These are only for backward source compatability -
 * Please do not use in new code.
 */
#define CLIOREV		(&(((Clio *)CLIO)->ClioRev))
#define CSYSBits	(&(((Clio *)CLIO)->CSysBits))
#define Vint0		(&(((Clio *)CLIO)->VInt0))
#define Vint1		(&(((Clio *)CLIO)->VInt1))
#define AUDIN		(&(((Clio *)CLIO)->AudioIn))
#define AUDOUT		(&(((Clio *)CLIO)->AudioOut))
#define CSTATBits	(&(((Clio *)CLIO)->CStatBits))
#define WDOG		(&(((Clio *)CLIO)->WatchDog))
#define HCNT		(&(((Clio *)CLIO)->HCnt))
#define VCNT		(&(((Clio *)CLIO)->VCnt))
#define SEED		(&(((Clio *)CLIO)->RandSeed))
#define RAND		(&(((Clio *)CLIO)->RandSample))
#define SetInt0Bits	(&(((Clio *)CLIO)->ClioInt0.SetIntBits))
#define ClrInt0Bits	(&(((Clio *)CLIO)->ClioInt0.ClrIntBits))
#define SetInt0EnBits	(&(((Clio *)CLIO)->ClioInt0.SetIntEnBits))
#define ClrInt0EnBits	(&(((Clio *)CLIO)->ClioInt0.ClrIntEnBits))
#define SETMODE		(&(((Clio *)CLIO)->SetMode))
#define CLRMODE		(&(((Clio *)CLIO)->ClrMode))
#define BADBITS		(&(((Clio *)CLIO)->BadBits))
#define SetInt1Bits	(&(((Clio *)CLIO)->ClioInt1.SetIntBits))
#define ClrInt1Bits	(&(((Clio *)CLIO)->ClioInt1.ClrIntBits))
#define SetInt1EnBits	(&(((Clio *)CLIO)->ClioInt1.SetIntEnBits))
#define ClrInt1EnBits	(&(((Clio *)CLIO)->ClioInt1.ClrIntEnBits))
#define HDELAY		(&(((Clio *)CLIO)->HDelay))
#define ADBIO		(&(((Clio *)CLIO)->ADBIOBits))
#define ADBCTL		(&(((Clio *)CLIO)->ADBCTLBits))

#define Timer0		(&(((Clio *)CLIO)->Timers[0].ht_cnt))
#define Timer0Back	(&(((Clio *)CLIO)->Timers[0].ht_cntBack))
#define Timer1		(&(((Clio *)CLIO)->Timers[1].ht_cnt))
#define Timer1Back	(&(((Clio *)CLIO)->Timers[1].ht_cntBack))
#define Timer2		(&(((Clio *)CLIO)->Timers[2].ht_cnt))
#define Timer2Back	(&(((Clio *)CLIO)->Timers[2].ht_cntBack))
#define Timer3		(&(((Clio *)CLIO)->Timers[3].ht_cnt))
#define Timer3Back	(&(((Clio *)CLIO)->Timers[3].ht_cntBack))
#define Timer4		(&(((Clio *)CLIO)->Timers[4].ht_cnt))
#define Timer4Back	(&(((Clio *)CLIO)->Timers[4].ht_cntBack))
#define Timer5		(&(((Clio *)CLIO)->Timers[5].ht_cnt))
#define Timer5Back	(&(((Clio *)CLIO)->Timers[5].ht_cntBack))
#define Timer6		(&(((Clio *)CLIO)->Timers[6].ht_cnt))
#define Timer6Back	(&(((Clio *)CLIO)->Timers[6].ht_cntBack))
#define Timer7		(&(((Clio *)CLIO)->Timers[7].ht_cnt))
#define Timer7Back	(&(((Clio *)CLIO)->Timers[7].ht_cntBack))
#define Timer8		(&(((Clio *)CLIO)->Timers[8].ht_cnt))
#define Timer8Back	(&(((Clio *)CLIO)->Timers[8].ht_cntBack))
#define Timer9		(&(((Clio *)CLIO)->Timers[9].ht_cnt))
#define Timer9Back	(&(((Clio *)CLIO)->Timers[9].ht_cntBack))
#define Timer10		(&(((Clio *)CLIO)->Timers[10].ht_cnt))
#define Timer10Back	(&(((Clio *)CLIO)->Timers[10].ht_cntBack))
#define Timer11		(&(((Clio *)CLIO)->Timers[11].ht_cnt))
#define Timer11Back	(&(((Clio *)CLIO)->Timers[11].ht_cntBack))
#define Timer12		(&(((Clio *)CLIO)->Timers[12].ht_cnt))
#define Timer12Back	(&(((Clio *)CLIO)->Timers[12].ht_cntBack))
#define Timer13		(&(((Clio *)CLIO)->Timers[12].ht_cnt))
#define Timer13Back	(&(((Clio *)CLIO)->Timers[12].ht_cntBack))
#define Timer14		(&(((Clio *)CLIO)->Timers[14].ht_cnt))
#define Timer14Back	(&(((Clio *)CLIO)->Timers[14].ht_cntBack))
#define Timer15		(&(((Clio *)CLIO)->Timers[15].ht_cnt))
#define Timer15Back	(&(((Clio *)CLIO)->Timers[15].ht_cntBack))
#define SetTm0		(&(((Clio *)CLIO)->TimerControl[0].htc_Set))
#define ClrTm0		(&(((Clio *)CLIO)->TimerControl[0].htc_Clr))
#define SetTm1		(&(((Clio *)CLIO)->TimerControl[1].htc_Set))
#define ClrTm1		(&(((Clio *)CLIO)->TimerControl[1].htc_Clr))
#define Slack		(&(((Clio *)CLIO)->TimerSlack))
#define FIFOINIT	(&(((Clio *)CLIO)->FifoInit))
#define DMAREQEN	(&(((Clio *)CLIO)->SetDMAEnable))
#define DMAREQDIS	(&(((Clio *)CLIO)->ClrDMAEnable))
#define FIFOstatus	(&(((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[0]))

#define DMATODSPP0	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[0]))
#define DMATODSPP1	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[1]))
#define DMATODSPP2	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[2]))
#define DMATODSPP3	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[3]))
#define DMATODSPP4	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[4]))
#define DMATODSPP5	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[5]))
#define DMATODSPP6	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[6]))
#define DMATODSPP7	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[7]))
#define DMATODSPP8	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[8]))
#define DMATODSPP9	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[9]))
#define DMATODSPP10	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[10]))
#define DMATODSPP11	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[11]))
#define DMATODSPP12	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[12]))
#define AUDTODSPPL	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[13]))
#define AUDTODSPPR	((((Clio *)CLIO)->clio_FIFOStatusDMAtoDSPP[14]))
#define DSPPTODMA0	(&(((Clio *)CLIO)->clio_FIFOStatusDSPPtoDMA[0]))
#define DSPPTODMA1	(&(((Clio *)CLIO)->clio_FIFOStatusDSPPtoDMA[1]))
#define DSPPTODMA2	(&(((Clio *)CLIO)->clio_FIFOStatusDSPPtoDMA[2]))
#define DSPPTODMA3	(&(((Clio *)CLIO)->clio_FIFOStatusDSPPtoDMA[3]))
#define SetExpCtl	(&(((Clio *)CLIO)->clio_ExpansionBus.xb_SetExpCtl))
#define XBUS_ADDR	(&(((Clio *)CLIO)->clio_ExpansionBus))
#define ClrExpCtl	(&(((Clio *)CLIO)->clio_ExpansionBus.xb_ClrExpCtl))
#define EXP_TYPES	(&(((Clio *)CLIO)->clio_ExpansionBus.xb_ExpType))
/* NOTE: The EXP_XFRCNT #define was at the WRONG offset in inthard.h. */
#define EXP_XFRCNT	(&(((Clio *)CLIO)->clio_ExpansionBus.xb_XferCnt))
#define DIPIR		(&(((Clio *)CLIO)->clio_ExpansionBus.xb_DIPIR))
#define EXP_SEL		(&(((Clio *)CLIO)->clio_ExpansionBus.xb_Sel[0]))
#define EXP_POLL	(&(((Clio *)CLIO)->clio_ExpansionBus.xb_Poll[0]))
#define EXP_CMDSTAT	(&(((Clio *)CLIO)->clio_ExpansionBus.xb_CmdStat[0]))
#define EXP_DATA	(&(((Clio *)CLIO)->clio_ExpansionBus.xb_Data[0]))

#define SEMAPHORE	(&(((Clio *)CLIO)->clio_DSPPSemaphore))
#define SEMAACK		(&(((Clio *)CLIO)->clio_DSPPSemaAck))
#define DSPPDMA		(&(((Clio *)CLIO)->clio_DSPPDMA))
#define DSPPRST0	(&(((Clio *)CLIO)->clio_DSPPTickReset0))
#define DSPPRST1	(&(((Clio *)CLIO)->clio_DSPPTickReset1))
/* NOTE: The NOISE #define was at the WRONG offset in inthard.h. */
#define NOISE		(&(((Clio *)CLIO)->clio_DSPPNoise))
#define DSPPPC		(&(((Clio *)CLIO)->clio_DSPPPC))
#define DSPPNR		(&(((Clio *)CLIO)->clio_DSPPNR))
#define DSPPGW		(&(((Clio *)CLIO)->clio_DSPPGW))
#define DSPPN32		(&(((Clio *)CLIO)->clio_DSPPN32bit[0]))
#define DSPPN16		(&(((Clio *)CLIO)->clio_DSPPN16bit[0]))
#define DSPPEI32	(&(((Clio *)CLIO)->clio_DSPPEI32bit[0]))
#define DSPPEI16	(&(((Clio *)CLIO)->clio_DSPPEI16bit[0]))
#define DSPPEO32	(&(((Clio *)CLIO)->clio_DSPPEO32bit[0]))
#define DSPPEO16	(&(((Clio *)CLIO)->clio_DSPPEO16bit[0]))

#define Other1REV	(&(((Clio *)CLIO)->clio_Other1Rev))
#define Other1Bits	(&(((Clio *)CLIO)->clio_Other1Bits))
#define Other2REV	(&(((Clio *)CLIO)->clio_Other2Rev))
#define Other2Bits	(&(((Clio *)CLIO)->clio_Other2Bits))
#define UNCLEREV	(&(((Clio *)CLIO)->clio_UncleRev))
#define WOODYADDR	(&(((Clio *)CLIO)->clio_UncleRev))
#define UNCLEADDR	WOODYADDR
#define USYSBits	(&(((Clio *)CLIO)->clio_UncleBits))

/* End of FOR COMPATABILITY ONLY #defines */

#endif							/* __CLIO_H */
