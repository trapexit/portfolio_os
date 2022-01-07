/* $Id: madam.h,v 1.5 1994/11/01 17:31:59 bungee Exp $ */
#ifndef __MADAM_H
#define __MADAM_H

/*
 * Copyright (C) 1994, The 3DO Company, Inc.
 * All Rights Reserved
 * Confidential and Proprietary
 */

/* NOTE NOTE: ASIC_ANVIL is TEMPORARY - do not depend on it! */

#pragma force_top_level
#pragma include_only_once

#include "types.h"

/* structures defining Madam hardware registers */

typedef struct Madam
{
/* 0000: Basic Control Words */
	vuint32			MadamRev;	/* 0000: Madam revision */
	vuint32			MSysBits;	/* 0004: system control bits */
	vuint32			MemCtl;		/* 0008: memory control bits */
	vuint32			SLTime;		/* 000c: time expanders */
	vuint32			MultiChip[4];	/* 0010: multi-chip */
	vuint32			Abortbits;	/* 0020: abort bits */
	vuint32			Privbits;	/* 0024: privilege bits */
	vuint32			StatBits;	/* 0028: status bits */
	vuint32			Reserved0x2c;	/* 002c: reserved for priority decode */
	char			Reserved0x30[0x40-0x30];
	vuint32			Diag;		/* 0040: H count */
#ifndef ASIC_ANVIL
	vuint32			Spare0x44;	/* 0044: */
#else /*ASIC_ANVIL*/
	vuint32			AnvilFeature;	/* 0044: */
#endif /*ASIC_ANVIL*/
	char			Reserved0x48[0x80-0x48];
	char			Reserved0x80[0x100-0x80];
/* 0100: CEL Stuff */
	vuint32			CELStart;	/* 0100: Start CEL Engine */
	vuint32			CELStop;	/* 0104: Stop CEL Engine */
	vuint32			CELContinue;	/* 0108: Continue CEL Engine */
	vuint32			CELPause;	/* 010c: Pause CEL Engine */
	vuint32			CCBCtl0;	/* 0110: CCB control word */
	char			Reserved0x114[0x120 - 0x114];
	vuint32			CCB_PIXC;	/* 0120: CCB control word */
	char			Reserved0x124[0x130 - 0x124];
/* 0130: Regis Control */
	vuint32			RegisCtl0;	/* 0130: Regis Control Word */
	vuint32			RegisCtl1;	/* 0134: X and Y clip values */
	vuint32			RegisCtl2;	/* 0138: Read Base address */
	vuint32			RegisCtl3;	/* 013c: Write Base address */
/* 0140: Regis Shape */
	vuint32			XYPosH;		/* 0140: */
	vuint32			XYPosL;		/* 0144: */
	vuint32			Line_dXYH;	/* 0148: */
	vuint32			Line_dXYL;	/* 014c: */
	vuint32			dXYH;		/* 0150: */
	vuint32			dXYL;		/* 0154: */
	vuint32			ddXYH;		/* 0158: */
	vuint32			ddXYL;		/* 015c: */
	char			Reserved0x160[0x180 - 0x160];
/* 0180: Palette Look-up Table (aka Pen Index Palette) */
	/*
	 * This is a union of 0x180 - 0x1ff in the read space,
	 * and 0x180 - 0x1bf in the write space. Thus the
	 * plut_write array is half the length of the plut_read array.
	 * (See the Opera Hardware Address spec for details.)
	 * 
	 * To access elements of this struct, use the #defines from below thus:
	 *   madam.plut0L to read the left half of PLUT 0 (at 0x180),
	 *   madam.plut2 to write PLUT 2 (at 0x188) and so forth.
	 */
	union
	    {
	    struct				/* 0180 - 01ff */
		{
		vuint32 left, right;
		} plut_read[16];
	    vuint32 plut_write[16];		/* 0180 - 01bf */
	    } plut_union;
/* 0200: Fence FIFO */
	/*
	 * The Fence FIFO is more sparse than the PLUT (see HW spec).
	 * To access elements of this array, use the #defines from below thus:
	 *   madam.Fence1L to read the left half of fence 0 (at 0x238),
	 *   madam.Fence1 to write fence 1 (at 0x21c) and so forth.
	 */
	vuint32			fence_stack[0x20]; /* 0200 - 0280 */
	char			Reserved0x280[0x300 - 0x280];
/* 0300: MMU */
	char			Reserved0x300[0x400 - 0x300];
/* 0400: DMA Register Stack */
	/* See #defines below for of addressing DMA stack elements */
	struct
	    {
	    vuint32 Address, Length, NextAddress, NextLength;
	    } DMAStack[0x17];			/* 0400 - 056f */
	struct
	    {
	    vuint32 ToRam, Length, FromRam, Refresh;
	    } ControlPort;			/* 0570 */
	struct
	    {
	    vuint32 Control, Video, MidLine, Reserved;
	    } CLUT_MID;				/* 0580 */
	struct
	    {
	    vuint32 Previous, Current, PreviousMid, CurrentMid;
	    } Video_MID;			/* 0590 */
	struct
	    {
	    vuint32 Control, FirstCCB, PLUT, DataStart;
	    } CELControl;			/* 05a0 */
	struct
	    {
	    vuint32 AddressA, LengthA, AddressB, LengthB;
	    } CELData;				/* 05b0 */
	/* See more #defines below for of addressing DMA stack2 elements */
	struct
	    {
	    vuint32 Address, Length, NextAddress, NextLength;
	    } DMAStack2[4];			/* 05c0 - 05ff */
/* 0600: Hardware Multiplier Math Stack */
	vuint32			Matrix[0x10];		/* 0600 - 063f */
	vuint32			B0_B1[0x28 - 0x10];	/* 0640 - 069c */
	char			Reserved0x6a0[0x700 - 0x6a0];
	char			Reserved0x700[0x7f0 - 0x700];
	vuint32			MathControlSet;		/* 07f0: */
	vuint32			MathControlClear;	/* 07f4: */
	vuint32			MathStatus;		/* 07f8: */
	vuint32			MathStartProcess;	/* 07fc: */
/* 0800: Not Assigned */
	char			Reserved0x800[0x1000 - 0x800];
/* 2000: Rest of MADAM Address Space */
	char			Reserved0x1000[0x2000 - 0x1000];
	char			Reserved0x10000[0x10000 - 0x2000];
} Madam;

/* Madam->MadamRev definitions */
#define	MADAM_CHIPID_MASK	0xff000000	/* Mask for Chip Id */
#define	MADAM_CHIPID_SHIFT	24
#define	MADAM_ARM_INTERNAL	0x00800000	/* Chip has internal ARM */
#define	MADAM_REV_MASK		0x007f0000	/* Mask for chip rev number */
#define	MADAM_REV_SHIFT		16
#define	MADAM_MANU_MASK		0x0000FF00	/* Mask for chip manufacturer */
#define	MADAM_MANU_SHIFT	8
#define MADAM_CHIP_WIREWRAP	0x00000001	/* wirewrap system */

#define	MAKE_MADAMREV(chipid,rev,manu) \
	(((chipid) << MADAM_CHIPID_SHIFT) | \
	 ((rev)    << MADAM_REV_SHIFT) | \
	 ((manu)   << MADAM_MANU_SHIFT))

/* Values for MADAM_CHIPID */
#define	MADAM_CHIPID_DIMADAM	0x01		/* Discrete Madam Chip Id */
#ifdef ASIC_ANVIL
#define MADAM_CHIPID_ANVIL	0x04		/* Anvil Chip Id */
#endif /*ASIC_ANVIL*/

/* Values for MADAM_REV */
#define	MADAM_DIMADAM_REV_RED	0x01		/* Preen rev Red */
#define	MADAM_DIMADAM_REV_GREEN	0x02		/* Madam rev Green */
#define	MADAM_DIMADAM_REV_PREEN	0x02		/* Madam rev Preen */
#ifdef ASIC_ANVIL
#define	MADAM_ANVIL_REV_1	0x00		/* Anvil rev 1,2 */
#endif /*ASIC_ANVIL*/

/* Values for MADAM_MANU */
#define	CHIP_MANU_TOSHIBA	0x01
#define	CHIP_MANU_MEC		0x02
#define	CHIP_MANU_SAMSUNG	0x03
#define	CHIP_MANU_FUJITSU	0x04
#define	CHIP_MANU_TI		0x05
#define	CHIP_MANU_ROHM		0x06
#define	CHIP_MANU_CHIPEXPRESS	0x07
#define	CHIP_MANU_YAMAHA	0x08
#define	CHIP_MANU_SANYO		0x09
#define	CHIP_MANU_IBM		0x0a
#define	CHIP_MANU_GOLDSTAR	0x0b
#define	CHIP_MANU_NEC		0x0c
#define	CHIP_MANU_MOTOROLA	0x0d
#define	CHIP_MANU_ATT		0x0e
#define	CHIP_MANU_VLSI		0x0f
/*
 * The manufacturer IDs have been standardized post-Anvil as above,
 * but Anvil and pre-Anvil chips used these (inconsistent) IDs.
 */
#define	CHIP_MANU_UNK_OLD	0x00	/* AT&T DIMADAM-Green, MEC Anvil */
#define	CHIP_MANU_PREEN_OLD	0x20	/* Toshiba and MEC DIMADAM-Preen */

/* Some MadamRev values */
#ifdef ASIC_ANVIL
#define	MADAM_ANVIL		MAKE_MADAMREV(MADAM_CHIPID_ANVIL, \
						MADAM_ANVIL_REV_1, \
						CHIP_MANU_UNK_OLD)
#endif /*ASIC_ANVIL*/
#define MADAM_PREEN		MAKE_MADAMREV(MADAM_CHIPID_DIMADAM, \
						MADAM_DIMADAM_REV_PREEN, \
						CHIP_MANU_PREEN_OLD)
#define MADAM_GREEN     	MAKE_MADAMREV(MADAM_CHIPID_DIMADAM, \
						MADAM_DIMADAM_REV_GREEN, \
						CHIP_MANU_UNK_OLD)
/* Some obsolete MadamRev values */
#define MADAM_GREENPLUS		(MADAM_GREEN | 0x1000)
#define MADAM_GREENWW   	(MADAM_GREEN | MADAM_CHIP_WIREWRAP)
#define MADAM_RED       	MAKE_MADAMREV(MADAM_CHIPID_DIMADAM, \
						MADAM_DIMADAM_REV_RED, \
						CHIP_MANU_UNK_OLD)
#define MADAM_REDBLUE   	(MADAM_RED | MADAM_CHIP_WIREWRAP)
#define MADAM_BLUE      	0x00000044
#define MADAM_BROWN     	0x00000040

/* Madam->MSysBits definitions */
#ifdef ASIC_ANVIL
#define	MADAM_VRAMSizeMask	0x00000007	/* VRAM size field */
						/* 01: 1MB, 10: 2MB */

#define	MADAM_DRAMBank0Mask	0x00000018	/* DRAM Bank 0 */
						/* 01: 1MB, 10: 4MB */
#define	MADAM_DRAMBank1Mask	0x00000060	/* DRAM Bank 1 */
						/* 01: 1MB, 10: 4MB */
#define MADAM_DRAMSizeMask	(MADAM_DRAMBank1Mask | MADAM_DRAMBank0Mask)
#define MADAM_DRAMByEight	0x00002000	/* DRAM x8 parts */
#endif /*ASIC_ANVIL*/

/* Madam->MemCtl definitions */
#define VRAMSIZE_MASK  0x00000007		/* RED ONLY */
#define DRAMSIZE_MASK  0x00000078		/* RED ONLY */
#define MCTL_unused0   0x00001F80		/* RED ONLY */
#define CLUTXEN        0x00002000		/* RED ONLY */
#define VSCTXEN        0x00004000		/* RED ONLY */
#define PLAYXEN        0x00008000		/* RED ONLY */
#define FENCEEN        0x00010000		/* RED ONLY */
#define SLIPXEN        0x00020000		/* RED ONLY */
#define MCTL_unused1   0x00040000		/* RED ONLY */
#define FENCOP         0x00080000		/* RED ONLY */
#define MCTL_unused2   0x00100000		/* RED ONLY */
#define CPUVEN         0x00200000		/* RED ONLY */
#define DRAMSIZE_SHIFT     3			/* RED ONLY */
#define VRAMSIZE_SHIFT     0			/* RED ONLY */
#define VRAMSIZE_SET1EMPTY   0x00000001		/* RED ONLY */
#define VRAMSIZE_SET1FULL    0x00000002		/* RED ONLY */
#define DRAMSIZE_SETMASK	3		/* RED ONLY */
#define DRAMSIZE_1MEG		0x000000001	/* RED ONLY */
#define DRAMSIZE_4MEG		0x000000002	/* RED ONLY */
#define DRAMSIZE_16MEG		0x000000003	/* RED ONLY */
#define DRAMSIZE_SET1SHIFT	3		/* RED ONLY */
#define DRAMSIZE_SET0SHIFT	(DRAMSIZE_SET1SHIFT+2)		/* RED ONLY */
#define	DRAMSIZE_ALLSHIFT	(DRAMSIZE_SET1SHIFT)		/* RED ONLY */
#define	DRAMSIZE_ALLMASK	(15)		/* RED ONLY */

#define DRAMSETZ_MASK     0x00000078		/* BLUE ONLY */
#define SYSRAMSIZE_MASK   0x00000380		/* BLUE ONLY */
#define DISPMOD_MASK      0x00001C00		/* BLUE ONLY */
#define BIST_MASK_BLUE    0x000E0000		/* BLUE ONLY */
#define BIST_SHIFT_BLUE   17			/* BLUE ONLY */
#define DISPMOD_SHIFT     10			/* BLUE ONLY */
#define SYSRAMSIZE_SHIFT   7			/* BLUE ONLY */
#define DRAMSETZ_SHIFT     3			/* BLUE ONLY */
#define DRAMSETZ_1MEG        0x00000000		/* BLUE ONLY */
#define DRAMSETZ_4MEG        0x00000020		/* BLUE ONLY */
#define DRAMSETZ_16MEG       0x00000040		/* BLUE ONLY */
#define DISPMOD_320   0x00000000		/* BLUE ONLY */
#define DISPMOD_384   0x00000400		/* BLUE ONLY */
#define DISPMOD_512   0x00000800		/* BLUE ONLY */
#define DISPMOD_1024  0x00000C00		/* BLUE ONLY */
#ifdef ASIC_ANVIL
#define	LONG_SC_TRANSFER	0x00100000	/* Enable Long (384) SC Xfers */
#endif /*ASIC_ANVIL*/

/* Madam->Abortbits definitions */
#define ABT_ROMF     0x00000001
#define ABT_ROMW     0x00000002
#define ABT_CLIOT    0x00000004
#define ABT_HARDU    0x00000008
#define ABT_unused4  0x00000010			/* RED ONLY */
#define ABT_SYSRAMU  0x00000010			/* BLUE ONLY */
#define ABT_FENCEV   0x00000020
#define ABT_VPR      0x00000040
#define ABT_R26E     0x00000080
#define ABT_SPSC     0x00000100
#define ABT_BITE     0x00000200
#define ABT_BADDEC   0x00000400
#define ABT_ARPS     0x00000800
#define ABT_BWACC    0x00001000

/* Madam->Privbits definitions */
#define PRIV_unused0         0x00000001		/* RED ONLY */
#define PRIV_unused1         0x00000002		/* RED ONLY */
#define PRIV_unused2         0x00000004		/* RED ONLY */
#define PRIV_DMAtoSYSRAM     0x00000001		/* BLUE ONLY */
#define PRIV_SPORTtoSYSRAM   0x00000002		/* BLUE ONLY */
#define PRIV_REGIStoSYSRAM   0x00000004		/* BLUE ONLY */
#define PRIV_DMA_VRAMSIZE    0x00000008
#define PRIV_SPORT_VRAMSIZE  0x00000010
#define PRIV_REGIS_VRAMSIZE  0x00000020
#define PRIV_REGIS_MATH      0x00000040

/* Madam->StatBits definitions */
#define DIAGRESTART     0x00000001
#define DIPIRRESTART    0x00000002
#define BIST_STAT_MASK  0x0000000C
#define SPRON           0x00000010
#define SPRPAU          0x00000020
#define SPREND          0x00000040
#define SPRPRQ          0x00000080
#define BIST_STAT_SHIFT    2

/* Madam->AnvilFeature definitions */
#ifdef ASIC_ANVIL
#define	MADAM_HiRes		0x00000001	/* Enable 640x480, cable only */
#define	MADAM_EnhanceFMVDMA	0x00000002	/* Enhanced FMV DMA */
#define	MADAM_AnvilWatchDog	0x00000004	/* Enable Anvil watchdog */
#define	MADAM_CDDMA		0x00000008	/* Enable 2 Low-cost CD chans */
#define	MADAM_SuperClipFix	0x00000010	/* Enable Anvil Superclip fix */
#define	MADAM_AddedROM		0x00000020	/* Enable additional ROM */
#define	MADAM_MathFix		0x00000040	/* Enable Anvil Math fixes  */
#define	MADAM_CornerEngIntr	0x00000080	/* Corner Engine Interrupt */
#define	MADAM_AnvilXbusEnhance	0x00000100	/* Enable Anvil Xbus enhance */
#endif /*ASIC_ANVIL*/

/* Madam->RegisCtl0 definitions */
#define G1_RMOD_MASK  0x0000000F
#define G2_RMOD_MASK  0x000000F0
#define G1_WMOD_MASK  0x00000F00
#define G2_WMOD_MASK  0x0000F000
#define RMOD_MASK     (G1_RMOD_MASK|G2_RMOD_MASK)
#define WMOD_MASK     (G1_WMOD_MASK|G2_WMOD_MASK)
#define RMOD_SHIFT    0
#define WMOD_SHIFT    8

#define G1_RMOD32    0x00000001
#define G1_RMOD512   0x00000002
#define G1_RMOD256   0x00000004
#define G1_RMOD1024  0x00000008
#define G2_RMOD64    0x00000010
#define G2_RMOD128   0x00000020
#define G2_RMODu6    0x00000040
#define G2_RMOD1024  0x00000080
#define G1_WMOD32    0x00000100
#define G1_WMOD512   0x00000200
#define G1_WMOD256   0x00000400
#define G1_WMOD1024  0x00000800
#define G2_WMOD64    0x00001000
#define G2_WMOD128   0x00002000
#define G2_WMODuE    0x00004000
#define G2_WMOD1024  0x00008000

#define RMOD_32    (G1_RMOD32)
#define RMOD_64    (G2_RMOD64)
#define RMOD_96    (G2_RMOD64 | G1_RMOD32)
#define RMOD_128   (G2_RMOD128)
#define RMOD_160   (G2_RMOD128 | G1_RMOD32)
#define RMOD_256   (G1_RMOD256)
#define RMOD_320   (G1_RMOD256 | G2_RMOD64)
#define RMOD_384   (G1_RMOD256 | G2_RMOD128)
#define RMOD_512   (G1_RMOD512)
#define RMOD_576   (G1_RMOD512 | G2_RMOD64)
#define RMOD_640   (G1_RMOD512 | G2_RMOD128)
#define RMOD_1024  (G1_RMOD1024)
#define RMOD_1056  (G2_RMOD1024 | G1_RMOD32)
#define RMOD_1088  (G1_RMOD1024 | G2_RMOD64)
#define RMOD_1152  (G1_RMOD1024 | G2_RMOD128)
#define RMOD_1280  (G2_RMOD1024 | G1_RMOD256)
#define RMOD_1536  (G2_RMOD1024 | G1_RMOD512)
#define RMOD_2048  (G1_RMOD1024 | G2_RMOD1024)

#define WMOD_32    (G1_WMOD32)
#define WMOD_64    (G2_WMOD64)
#define WMOD_96    (G2_WMOD64 | G1_WMOD32)
#define WMOD_128   (G2_WMOD128)
#define WMOD_160   (G2_WMOD128 | G1_WMOD32)
#define WMOD_256   (G1_WMOD256)
#define WMOD_320   (G1_WMOD256 | G2_WMOD64)
#define WMOD_384   (G1_WMOD256 | G2_WMOD128)
#define WMOD_512   (G1_WMOD512)
#define WMOD_576   (G1_WMOD512 | G2_WMOD64)
#define WMOD_640   (G1_WMOD512 | G2_WMOD128)
#define WMOD_1024  (G1_WMOD1024)
#define WMOD_1056  (G2_WMOD1024 | G1_WMOD32)
#define WMOD_1088  (G1_WMOD1024 | G2_WMOD64)
#define WMOD_1152  (G1_WMOD1024 | G2_WMOD128)
#define WMOD_1280  (G2_WMOD1024 | G1_WMOD256)
#define WMOD_1536  (G2_WMOD1024 | G1_WMOD512)
#define WMOD_2048  (G1_WMOD1024 | G2_WMOD1024)

/* Madam->RegisCtl1 definitions */
#define REG_XCLIP_MASK  0x000007FF
#define REG_YCLIP_MASK  0x07FF0000
#define REG_XCLIP_SHIFT  0
#define REG_YCLIP_SHIFT  16

/* Madam->MathControlClear definitions */
#define MATH_ADELAY_MASK  0x00000003	/* Accumulator delay mask */
#define MATH_EARLYTERM    0x00000004	/* Early termination select */

/* Madam->MathStatus definitions */
#define MATH_DONE         0x00000001
#define MATH_BANK         0x00000002
#define MATH_MACON        0x00000004
#define MATH_PREMIP       0x00000008

/* Madam->MathStartProcess definitions */
#define MATH_4X4         0x00000001	/* 4x4 Multiply/Accum */
#define MATH_3X3         0x00000002	/* 3x3 Multiply/Accum */
#define MATH_3X3_SCALED  0x00000004
#define MATH_CCB         0x00000008	/* CCB conversion */
#define MATH_CCB_FAST    0x00000010	/* CCB conv, pre-div values */
#define MATH_BIGDIV      0x00000020	/* Big divide - NOT IMPLEMENTED */

/* Sub-element Convenience Definitions */

/* 
 * Defines for sub-elements of Palette Lookup Table
 * (aka Pen Index Palette) struct, read domain
 * Note that only the lower 16-bits are valid - you must mask.
 */
#define	plut0L plut_union.plut_read[0].left	/* 0180 */
#define	plut0R plut_union.plut_read[0].right	/* 0184 */
#define	plut1L plut_union.plut_read[1].left	/* 0188 */
#define	plut1R plut_union.plut_read[1].right	/* 018c */
#define	plut2L plut_union.plut_read[2].left	/* 0190 */
#define	plut2R plut_union.plut_read[2].right	/* 0194 */
#define	plut3L plut_union.plut_read[3].left	/* 0198 */
#define	plut3R plut_union.plut_read[3].right	/* 019c */
#define	plut4L plut_union.plut_read[4].left	/* 01a0 */
#define	plut4R plut_union.plut_read[4].right	/* 01a4 */
#define	plut5L plut_union.plut_read[5].left	/* 01a8 */
#define	plut5R plut_union.plut_read[5].right	/* 01ac */
#define	plut6L plut_union.plut_read[6].left	/* 01b0 */
#define	plut6R plut_union.plut_read[6].right	/* 01b4 */
#define	plut7L plut_union.plut_read[7].left	/* 01b8 */
#define	plut7R plut_union.plut_read[7].right	/* 01bc */

#define	plut8L plut_union.plut_read[8].left	/* 01c0 */
#define	plut8R plut_union.plut_read[8].right	/* 01c4 */
#define	plut9L plut_union.plut_read[9].left	/* 01c8 */
#define	plut9R plut_union.plut_read[9].right	/* 01cc */
#define	plutAL plut_union.plut_read[0xa].left	/* 01d0 */
#define	plutAR plut_union.plut_read[0xa].right	/* 01d4 */
#define	plutBL plut_union.plut_read[0xb].left	/* 01d8 */
#define	plutBR plut_union.plut_read[0xb].right	/* 01dc */
#define	plutCL plut_union.plut_read[0xc].left	/* 01e0 */
#define	plutCR plut_union.plut_read[0xc].right	/* 01e4 */
#define	plutDL plut_union.plut_read[0xd].left	/* 01e8 */
#define	plutDR plut_union.plut_read[0xd].right	/* 01ec */
#define	plutEL plut_union.plut_read[0xe].left	/* 01f0 */
#define	plutER plut_union.plut_read[0xe].right	/* 01f4 */
#define	plutFL plut_union.plut_read[0xf].left	/* 01f8 */
#define	plutFR plut_union.plut_read[0xf].right	/* 01fc */

/* Defines for sub-elements of PLUT struct, write domain */
#define	plut0 plut_union.plut_write[0]		/* 0180 */
#define	plut1 plut_union.plut_write[1]		/* 0184 */
#define	plut2 plut_union.plut_write[2]		/* 0188 */
#define	plut3 plut_union.plut_write[3]		/* 018c */
#define	plut4 plut_union.plut_write[4]		/* 0190 */
#define	plut5 plut_union.plut_write[5]		/* 0194 */
#define	plut6 plut_union.plut_write[6]		/* 0198 */
#define	plut7 plut_union.plut_write[7]		/* 019c */

#define	plut8 plut_union.plut_write[8]		/* 01a0 */
#define	plut9 plut_union.plut_write[9]		/* 01a4 */
#define	plutA plut_union.plut_write[0xa]	/* 01a8 */
#define	plutB plut_union.plut_write[0xb]	/* 01ac */
#define	plutC plut_union.plut_write[0xc]	/* 01b0 */
#define	plutD plut_union.plut_write[0xd]	/* 01b4 */
#define	plutE plut_union.plut_write[0xe]	/* 01b8 */
#define	plutF plut_union.plut_write[0xf]	/* 01bc */

/* Defines for the sub-elements of the Fence Fifo array, read domain */
#define	Fence0L fence_stack[0xc]		/* 0230 */
#define	Fence0R fence_stack[0xd]		/* 0234 */
#define	Fence1L fence_stack[0xe]		/* 0238 */
#define	Fence1R fence_stack[0xf]		/* 023c */

#define	Fence2L fence_stack[0x1c]		/* 0270 */
#define	Fence2R fence_stack[0x1d]		/* 0274 */
#define	Fence3L fence_stack[0x1e]		/* 0278 */
#define	Fence3R fence_stack[0x1f]		/* 027c */

/* Defines for the sub-elements of the Fence Fifo array, read domain */
#define	Fence0 fence_stack[6]			/* 0218 */
#define	Fence1 fence_stack[7]			/* 021c */
#define	Fence2 fence_stack[e]			/* 0238 */
#define	Fence3 fence_stack[f]			/* 023c */

/* Defines for the sub-elements of the Fence Fifo array, read domain */
#define	RamToDSPP0	DMAStack[0]		/* 0400 */
#define	RamToDSPP1	DMAStack[1]		/* 0410 */
#define	RamToDSPP2	DMAStack[2]		/* 0420 */
#define	RamToDSPP3	DMAStack[3]		/* 0430 */
#define	RamToDSPP4	DMAStack[4]		/* 0440 */
#define	RamToDSPP5	DMAStack[5]		/* 0450 */
#define	RamToDSPP6	DMAStack[6]		/* 0460 */
#define	RamToDSPP7	DMAStack[7]		/* 0470 */
#define	RamToDSPP8	DMAStack[8]		/* 0480 */
#define	RamToDSPP9	DMAStack[9]		/* 0490 */
#define	RamToDSPP10	DMAStack[10]		/* 04a0 */
#define	RamToDSPP11	DMAStack[11]		/* 04b0 */
#define	RamToDSPP12	DMAStack[12]		/* 04c0 */

#define	RamToUncle	DMAStack[0x0d]		/* 04d0 */
#define	RamToExternal	DMAStack[0x0e]		/* 04e0 */
#define	RamToDSPPNStack	DMAStack[0x0f]		/* 04f0 */

#define	DSPPToRam0	DMAStack[0x10]		/* 0500 */
#define	DSPPToRam1	DMAStack[0x11]		/* 0510 */
#define	DSPPToRam2	DMAStack[0x12]		/* 0520 */
#define	DSPPToRam3	DMAStack[0x13]		/* 0530 */

#define	DMAExpo		DMAStack[0x14]		/* 0540 */
#define	UncleToRam	DMAStack[0x15]		/* 0550 */
#define	ExternalToRam	DMAStack[0x16]		/* 0560 */

#define	Commandgrabber	DMAStack2[0]		/* 05c0 */
#define	Framegrabber	DMAStack2[1]		/* 05d0 */
#ifdef ASIC_ANVIL
#define	CD0ToRam	DMAStack2[2]		/* 05e0 */
#define	CD1ToRam	DMAStack2[3]		/* 05f0 */
#endif /*ASIC_ANVIL*/

/*
 * FOR COMPATABILITY ONLY
 *
 * These are the old names from inthard.h redefined
 * to use the structure definitions.
 * These are only for backward source compatability -
 * Please do not use in new code.
 */

#define MADAMREV	(&(((Madam *)MADAM)->MadamRev))
#define MSYSBits  	(&(((Madam *)MADAM)->MSysBits))
#define MCTL      	(&(((Madam *)MADAM)->MemCtl))
#define SLTIME    	(&(((Madam *)MADAM)->SLTime))
#define MultiCHIP	(&(((Madam *)MADAM)->MultiChip[0]))
#define AbortBits	(&(((Madam *)MADAM)->Abortbits))
#define PrivBits	(&(((Madam *)MADAM)->Privbits))
#define STATBits	(&(((Madam *)MADAM)->StatBits))
#define PRIDEC		(&(((Madam *)MADAM)->Reserved0x2c))
#define MADAMDIAG0	(&(((Madam *)MADAM)->Diag))
#define MADAMDIAG1	(&(((Madam *)MADAM)->Spare0x44))
#define SPRSTRT		(&(((Madam *)MADAM)->CELStart))
#define SPRSTOP		(&(((Madam *)MADAM)->CELStop))
#define SPRCNTU		(&(((Madam *)MADAM)->CELContinue))
#define SPRPAUS		(&(((Madam *)MADAM)->CELPause))
#define CCBCTL0		(&(((Madam *)MADAM)->CCBCtl0))
#define PPMPC		(&(((Madam *)MADAM)->CCB_PIXC))
#define REGCTL0		(&(((Madam *)MADAM)->RegisCtl0))
#define REGCTL1		(&(((Madam *)MADAM)->RegisCtl1))
#define REGCTL2		(&(((Madam *)MADAM)->RegisCtl2))
#define REGCTL3		(&(((Madam *)MADAM)->RegisCtl3))
#define XYPOSH		(&(((Madam *)MADAM)->XYPosH))
#define XYPOSL		(&(((Madam *)MADAM)->XYPosL))
#define LINEDXYH	(&(((Madam *)MADAM)->Line_dXYH))
#define LINEDXYL	(&(((Madam *)MADAM)->Line_dXYL))
#define DXYH		(&(((Madam *)MADAM)->dXYH))
#define DXYL		(&(((Madam *)MADAM)->dXYL))
#define DDXYH		(&(((Madam *)MADAM)->ddXYH))
#define DDXYL		(&(((Madam *)MADAM)->ddXYL))

#define PLUTSTACK	(&(((Madam *)MADAM)->plut0L))
#define PLUT0L		(&(((Madam *)MADAM)->plut0L))
#define PLUT0R		(&(((Madam *)MADAM)->plut0R))
#define PLUT1L		(&(((Madam *)MADAM)->plut1L))
#define PLUT1R		(&(((Madam *)MADAM)->plut1R))
#define PLUT2L		(&(((Madam *)MADAM)->plut2L))
#define PLUT2R		(&(((Madam *)MADAM)->plut2R))
#define PLUT3L		(&(((Madam *)MADAM)->plut3L))
#define PLUT3R		(&(((Madam *)MADAM)->plut3R))
#define PLUT4L		(&(((Madam *)MADAM)->plut4L))
#define PLUT4R		(&(((Madam *)MADAM)->plut4R))
#define PLUT5L		(&(((Madam *)MADAM)->plut5L))
#define PLUT5R		(&(((Madam *)MADAM)->plut5R))
#define PLUT6L		(&(((Madam *)MADAM)->plut6L))
#define PLUT6R		(&(((Madam *)MADAM)->plut6R))
#define PLUT7L		(&(((Madam *)MADAM)->plut7L))
#define PLUT7R		(&(((Madam *)MADAM)->plut7R))
#define PLUT8L		(&(((Madam *)MADAM)->plut8L))
#define PLUT8R		(&(((Madam *)MADAM)->plut8R))
#define PLUT9L		(&(((Madam *)MADAM)->plut9L))
#define PLUT9R		(&(((Madam *)MADAM)->plut9R))
#define PLUTAL		(&(((Madam *)MADAM)->plutAL))
#define PLUTAR		(&(((Madam *)MADAM)->plutAR))
#define PLUTBL		(&(((Madam *)MADAM)->plutBL))
#define PLUTBR		(&(((Madam *)MADAM)->plutBR))
#define PLUTCL		(&(((Madam *)MADAM)->plutCL))
#define PLUTCR		(&(((Madam *)MADAM)->plutCR))
#define PLUTDL		(&(((Madam *)MADAM)->plutDL))
#define PLUTDR		(&(((Madam *)MADAM)->plutDR))
#define PLUTEL		(&(((Madam *)MADAM)->plutEL))
#define PLUTER		(&(((Madam *)MADAM)->plutER))
#define PLUTFL		(&(((Madam *)MADAM)->plutFL))
#define PLUTFR		(&(((Madam *)MADAM)->plutFR))

#define PLUT0		(&(((Madam *)MADAM)->plut0))
#define PLUT1		(&(((Madam *)MADAM)->plut1))
#define PLUT2		(&(((Madam *)MADAM)->plut2))
#define PLUT3		(&(((Madam *)MADAM)->plut3))
#define PLUT4		(&(((Madam *)MADAM)->plut4))
#define PLUT5		(&(((Madam *)MADAM)->plut5))
#define PLUT6		(&(((Madam *)MADAM)->plut6))
#define PLUT7		(&(((Madam *)MADAM)->plut7))
#define PLUT8		(&(((Madam *)MADAM)->plut8))
#define PLUT9		(&(((Madam *)MADAM)->plut9))
#define PLUTA		(&(((Madam *)MADAM)->plutA))
#define PLUTB		(&(((Madam *)MADAM)->plutB))
#define PLUTC		(&(((Madam *)MADAM)->plutC))
#define PLUTD		(&(((Madam *)MADAM)->plutD))
#define PLUTE		(&(((Madam *)MADAM)->plutE))
#define PLUTF		(&(((Madam *)MADAM)->plutF))

#define FENCESTACK	(&(((Madam *)MADAM)->fence_stack[0]))
#define FENCE0L		(&(((Madam *)MADAM)->Fence0L))
#define FENCE0R		(&(((Madam *)MADAM)->Fence0R))
#define FENCE1L		(&(((Madam *)MADAM)->Fence1L))
#define FENCE1R		(&(((Madam *)MADAM)->Fence1R))
#define FENCE2L		(&(((Madam *)MADAM)->Fence2L))
#define FENCE2R		(&(((Madam *)MADAM)->Fence2R))
#define FENCE3L		(&(((Madam *)MADAM)->Fence3L))
#define FENCE3R		(&(((Madam *)MADAM)->Fence3R))
#define FENCE0		(&(((Madam *)MADAM)->Fence0))
#define FENCE1		(&(((Madam *)MADAM)->Fence1))
#define FENCE2		(&(((Madam *)MADAM)->Fence2))
#define FENCE3		(&(((Madam *)MADAM)->Fence3))

#define MMU		(&(((Madam *)MADAM)->Reserved0x300[0]))
#define DMASTACK	(&(((Madam *)MADAM)->DMAStack[0]))
#define DMAAddress      0
#define DMALength       1
#define DMANextAddress  2
#define DMANextLength   3

#define RAMtoDSPP0	(&(((Madam *)MADAM)->RamToDSPP0.Address))
#define RAMtoDSPP1	(&(((Madam *)MADAM)->RamToDSPP1.Address))
#define RAMtoDSPP2	(&(((Madam *)MADAM)->RamToDSPP2.Address))
#define RAMtoDSPP3	(&(((Madam *)MADAM)->RamToDSPP3.Address))
#define RAMtoDSPP4	(&(((Madam *)MADAM)->RamToDSPP4.Address))
#define RAMtoDSPP5	(&(((Madam *)MADAM)->RamToDSPP5.Address))
#define RAMtoDSPP6	(&(((Madam *)MADAM)->RamToDSPP6.Address))
#define RAMtoDSPP7	(&(((Madam *)MADAM)->RamToDSPP7.Address))
#define RAMtoDSPP8	(&(((Madam *)MADAM)->RamToDSPP8.Address))
#define RAMtoDSPP9	(&(((Madam *)MADAM)->RamToDSPP9.Address))
#define RAMtoDSPP10	(&(((Madam *)MADAM)->RamToDSPP10.Address))
#define RAMtoDSPP11	(&(((Madam *)MADAM)->RamToDSPP11.Address))
#define RAMtoDSPP12	(&(((Madam *)MADAM)->RamToDSPP12.Address))
#define RAMtoUNCLE	(&(((Madam *)MADAM)->RamToUncle.Address))
#define RAMtoEXTERNAL	(&(((Madam *)MADAM)->RamToExternal.Address))
#define RAMtoDSPPN	(&(((Madam *)MADAM)->RamToDSPPNStack.Address))
#define DSPPtoRAM0	(&(((Madam *)MADAM)->DSPPToRam0.Address))
#define DSPPtoRAM1	(&(((Madam *)MADAM)->DSPPToRam1.Address))
#define DSPPtoRAM2	(&(((Madam *)MADAM)->DSPPToRam2.Address))
#define DSPPtoRAM3	(&(((Madam *)MADAM)->DSPPToRam3.Address))
#define DMAtofrEXP0	(&(((Madam *)MADAM)->DMAExpo.Address))
#define UNCLEtoRAM	(&(((Madam *)MADAM)->UncleToRam.Address))
#define EXTERNALtoRAM	(&(((Madam *)MADAM)->ExternalToRam.Address))
#ifdef ASIC_ANVIL
#define CD0toRAM	(&(((Madam *)MADAM)->CD0ToRam.Address))
#define CD1toRAM	(&(((Madam *)MADAM)->CD1ToRam.Address))
#endif /*ASIC_ANVIL*/
#define RAMtofrPLAYER	(&(((Madam *)MADAM)->ControlPort.ToRam))
#define CLUTMIDctl	(&(((Madam *)MADAM)->CLUT_MID.Control))
#define CLUTMIDvideo	(&(((Madam *)MADAM)->CLUT_MID.Video))
#define CLUTMIDmidline	(&(((Madam *)MADAM)->CLUT_MID.MidLine))
#define VIDMIDprev	(&(((Madam *)MADAM)->Video_MID.Previous))
#define VIDMIDcur	(&(((Madam *)MADAM)->Video_MID.Current))
#define VIDMIDprevmid	(&(((Madam *)MADAM)->Video_MID.PreviousMid))
#define VIDMIDcurmid	(&(((Madam *)MADAM)->Video_MID.CurrentMid))
#define CelCtl0		(&(((Madam *)MADAM)->CELControl.Control))
#define NEXTPTR		(&(((Madam *)MADAM)->CELControl.FirstCCB))
#define CelCtlPLUT	(&(((Madam *)MADAM)->CELControl.PLUT))
#define CelCtlData	(&(((Madam *)MADAM)->CELControl.DataStart))
#define CelAddrA	(&(((Madam *)MADAM)->CELData.AddressA))
#define CelLenA		(&(((Madam *)MADAM)->CELData.LengthA))
#define CelAddrB	(&(((Madam *)MADAM)->CELData.AddressB))
#define CelLenB		(&(((Madam *)MADAM)->CELData.LengthB))
#define CommandGrabber	(&(((Madam *)MADAM)->Commandgrabber.Address))
#define FrameGrabber	(&(((Madam *)MADAM)->Framegrabber.Address))

#define MATH_STACK	(&(((Madam *)MADAM)->Matrix[0]))

#define MATH_CONTROLSET		(&(((Madam *)MADAM)->MathControlSet))
#define MATH_CONTROLCLEAR	(&(((Madam *)MADAM)->MathControlClear))
#define MATH_STATUS		(&(((Madam *)MADAM)->MathStatus))
#define MATH_START		(&(((Madam *)MADAM)->MathStartProcess))

/* End of FOR COMPATABILITY ONLY #defines */

#endif							/* __MADAM_H */
