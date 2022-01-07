#ifndef __SYSINFO_H
#define __SYSINFO_H

/* $Id: sysinfo.h,v 1.8 1994/12/06 02:23:52 sdas Exp $ */

/*
    Copyright (C) 1993, 1994 The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

/* TAG defintions for the SuperQuerySysInfo */

#define	SYSINFO_TAG_MATHDIVOUTERR	0x50001	/* Incorrect divide output if */
						/* one of inputs is large     */
#define	SYSINFO_MATHDIVOUTERR_TRUE	1
#define	SYSINFO_MATHDIVOUTERR_FALSE	0

#define	SYSINFO_TAG_MATHDIVOVERLAP	0x50002	/* Incorrect divide with      */
						/* overlap operations         */
#define	SYSINFO_MATHDIVOVERLAP_TRUE	1
#define	SYSINFO_MATHDIVOVERLAP_FALSE	0

#define	SYSINFO_TAG_MATHSWAPDONE	0x50003	/* SWAP sets incorrect status */
					/* of operation-in-progress bit       */
#define	SYSINFO_MATHSWAPDONE_TRUE	1
#define	SYSINFO_MATHSWAPDONE_FALSE	0

#define	SYSINFO_TAG_AUDOUTPRESENT	0x40001	/* AUDOUT control             */
#define	SYSINFO_AUDOUT_PRESENT		1
#define	SYSINFO_AUDOUT_ABSENT		0
/* AUDOUT control value passed in info. buffer */

#define	SYSINFO_TAG_AUDINPRESENT	0x40002	/* Presence of audio input    */
#define	SYSINFO_AUDIN_PRESENT		1
#define	SYSINFO_AUDIN_ABSENT		0
/* AUDIN control value passed in info. buffer */

#define	SYSINFO_TAG_AUDDSPPCLOCK	0x40003	/* DSPP clock rate            */
#define	SYSINFO_AUDDSPP_SUCCESS		1
#define	SYSINFO_AUDDSPP_FAILURE		0
/* DSPP clock rate passed in info. buffer */

#define	SYSINFO_TAG_FIELDFREQ		0x10001	/* Field frequency in HZ      */
#define	SYSINFO_FREQ_50HZ		50
#define	SYSINFO_FREQ_60HZ		60

#define	SYSINFO_TAG_PLATFORMID		0x10002	/* Platform id- MEC/GOLDSTAR  */
						/* Green/Anvil..              */
/* NOTE: This query is ONLY for diagnostics/debug/display purposes. */
/*       This should not be used to determine any feature of the IM */

typedef struct PlatformID {
	unsigned char	mfgr;
	unsigned char	chip;
	unsigned char	ver;
	unsigned char	rev;
} PlatformID;

/* Values for the mfgr/product field */
#define	SYSINFO_MFGR_MEC		0x01	/* FZ-1 and compatible system */
#define	SYSINFO_MFGR_SANYO		0x02	/* FZ-1 compatible system     */
#define	SYSINFO_MFGR_ATT		0x03	/*                            */
#define	SYSINFO_MFGR_GOLDSTAR		0x04	/* FZ-1 compatible system     */
#define	SYSINFO_MFGR_SAMSUNG		0x05	/*                            */
#define	SYSINFO_MFGR_CREATIVE		0x06	/* 3DO Blaster 1.0            */
#define	SYSINFO_MFGR_SA			0x07	/* Cable trial system         */
#define	SYSINFO_MFGR_TOSHIBA		0x08	/* Naviken system             */

/* Values for the chip field */
#define	SYSINFO_CHIP_RED		0x01
#define	SYSINFO_CHIP_GREEN		0x02
#define	SYSINFO_CHIP_ANVIL		0x03

#define	SYSINFO_TAG_MADAMID		0x10003	/* MADAM id- Green, Preen,... */
#define	SYSINFO_TAG_CLIOID		0x10004	/* CLIO id - Green, Preen,... */
#define	SYSINFO_TAG_UNCLEID		0x10005	/* UNCLE id                   */

#define	SYSINFO_TAG_CURROMBANK		0x10006	/* 2nd (Kanji Font) ROM switch*/
#define	SYSINFO_ROMBANK1		0	/* Main(boot) ROM Bank        */
#define	SYSINFO_ROMBANK2		1	/* 2nd (Kanji) ROM Bank       */

#define	SYSINFO_TAG_ROM2BASE		0x10007	/* Base of 2nd(Kanji Font) ROM*/
						/* returns addr in info param */
#define	SYSINFO_ROM2FOUND		0	/* found the base addr        */
#define	SYSINFO_ROM2NOTFOUND		1	/* Could not find 2nd rom bank*/

#define	SYSINFO_TAG_BOOTDISCTYPE	0x10008	/* Boot disc type             */
#define SYSINFO_DISCTYPE_NOTSET		0	/* default state              */
#define SYSINFO_DISCTYPE_NODISC		1	/* no disc in drive           */
#define SYSINFO_DISCTYPE_UNKNOWN	2	/* unknown disc type          */
#define SYSINFO_DISCTYPE_3DO		3	/* 3DO application CD         */
#define SYSINFO_DISCTYPE_AUDIO		4	/* audio CD                   */
#define SYSINFO_DISCTYPE_PHOTO		5	/* photo CD                   */
#define SYSINFO_DISCTYPE_VIDEO		6	/* video CD                   */
#define SYSINFO_DISCTYPE_NAVIKEN	7	/* naviken CD                 */

#define	SYSINFO_TAG_SPLITNVRAM		0x10009	/* NVRAM split into 4 2M units*/
#define	SYSINFO_SPLITNVRAM_TRUE		1	/* NVRAM is split into 4 units*/
#define	SYSINFO_SPLITNVRAM_FALSE	0	/* NVRAM is one 8M unit       */

#define	SYSINFO_TAG_CUREMSBANK		0x1000a	/* selected/active Expanded-  */
						/* Memory-System bank         */
						/* returns bank# in info param*/
#define	SYSINFO_EMSBANKS_SUPPORTED	1
#define	SYSINFO_EMSBANKS_NOSUPPORT	0

#define	SYSINFO_TAG_GRAPHDISPWDTH	0x20001	/* Display width              */
#define	SYSINFO_DISPWIDTH320		320
#define	SYSINFO_DISPWIDTH384		384

#define	SYSINFO_TAG_GRAPHDISPHGT	0x20002	/* Display height             */
#define	SYSINFO_DISPHEITH240		240
#define	SYSINFO_DISPHEITH288		288


#define	SYSINFO_TAG_GRAPHVBLINES	0x20003	/* Total lines of VBlank      */
#define	SYSINFO_DISPVLINES21		21

#define	SYSINFO_TAG_GRAPHDISPSUPP	0x20004	/* Display mode supports      */
#define	SYSINFO_NTSC_SUPPORTED		0x00000001
#define	SYSINFO_PAL_SUPPORTED		0x00000002
#define	SYSINFO_NTSC_DFLT		0x00000100
#define	SYSINFO_PAL_DFLT		0x00000200
#define	SYSINFO_NTSC_CURDISP		0x00010000
#define	SYSINFO_PAL_CURDISP		0x00020000


#define	SYSINFO_TAG_GRAPHSUPPINTLC	0x20005	/* Interlace mode support     */
#define	SYSINFO_INTLC_SUPPORTED	1
#define	SYSINFO_INTLC_NOSUPPORT	0

#define	SYSINFO_TAG_GRAPHSUPPNINTLC	0x20006	/* Non-interlace mode support */
#define	SYSINFO_NINTLC_SUPPORTED	1
#define	SYSINFO_NINTLC_NOSUPPORT	0

#define	SYSINFO_TAG_GRAPHSUPPDBLHRZ	0x20007	/* Double horizontl resolution*/
#define	SYSINFO_DBLHORZ_SUPPORTED	1
#define	SYSINFO_DBLHORZ_NOSUPPORT	0

#define	SYSINFO_TAG_GRAPHSUPPCC		0x20008	/* Close-caption support      */
#define	SYSINFO_CC_SUPPORTED	1
#define	SYSINFO_CC_NOSUPPORT	0

#define	SYSINFO_TAG_GRAPH_HDELAY	0x20009	/* Support for reading HDELAY */
#define	SYSINFO_HDELAY_SUPPORTED	1
#define	SYSINFO_HDELAY_NOSUPPORT	0

#define	SYSINFO_TAG_CDROMSUPPORT	0x30001	/* Type of CD-ROM supported   */
#define	SYSINFO_MKE_DRIVE_SUPPORTED	0x00000001
#define	SYSINFO_SB_DRIVE_SUPPORTED	0x00000002
#define	SYSINFO_LCCD_DRIVE_SUPPORTED	0x00000004

#define	SYSINFO_TAG_ROMFSADDR		0x30002	/* Start addr of /rom filesys */
#define	SYSINFO_ROMFS_SUPPORTED	1
#define	SYSINFO_ROMFS_NOSUPPORT	0

#define	SYSINFO_TAG_INTLDFLTLANG	0x90001	/* Dflt international lang set*/
/* This tag not supported romcores built after 1.4.1 release */
/* For newer romcores, use SYSINFO_TAG_INTLLANGCNTRY instead */
#define	SYSINFO_INTLLANG_ENGLISH	0	/* default language           */
#define	SYSINFO_INTLLANG_GERMAN		1
#define	SYSINFO_INTLLANG_JAPANESE	2
#define	SYSINFO_INTLLANG_SPANISH	3
#define	SYSINFO_INTLLANG_ITALIAN	4
#define	SYSINFO_INTLLANG_CHINESE	5
#define	SYSINFO_INTLLANG_KOREAN		6
#define	SYSINFO_INTLLANG_FRENCH		7

#define	SYSINFO_TAG_INTLLANGCNTRY	0x90002	/* Dflt lang/country for systm*/
#define	SYSINFO_LANGCNTRY_USA		0	/* US/English, default        */
#define	SYSINFO_LANGCNTRY_GERMANY	1	/* Germany/German             */
#define	SYSINFO_LANGCNTRY_JAPAN		2	/* Japan/Japanese             */
#define	SYSINFO_LANGCNTRY_SPAIN		3	/* Spain/Spanish              */
#define	SYSINFO_LANGCNTRY_ITALY		4	/* Italy/Italian              */
#define	SYSINFO_LANGCNTRY_CHINA		5	/* China/Chinese              */
#define	SYSINFO_LANGCNTRY_KOREA		6	/* Korea/Korean               */
#define	SYSINFO_LANGCNTRY_FRANCE	7	/* France/French              */
#define	SYSINFO_LANGCNTRY_UK		8	/* UK/English                 */
#define	SYSINFO_LANGCNTRY_AUSTRALIA	9	/* Australia/English          */
#define	SYSINFO_LANGCNTRY_MEXICO	10	/* Mexico/Spanish             */
#define	SYSINFO_LANGCNTRY_CANADA	11	/* Canada/English             */


#define	SYSINFO_UNSUPPORTEDTAG		0xfffffffe
#define	SYSINFO_BADTAG			0xffffffff


/* TAG defintions for the SuperSetSysInfo */

#define	SYSINFO_TAG_SETROMBANK		0x11006	/* 2nd (Kanji Font) ROM switch*/
	/* Tag values are the same as for SYSINFO_TAG_CURROMBANK */
#define	SYSINFO_ROMBANK1		0	/* Main(boot) ROM Bank        */
#define	SYSINFO_ROMBANK2		1	/* 2nd (Kanji) ROM Bank       */

#define	SYSINFO_TAG_WATCHDOG		0x11007	/* Enable Watchdog timer      */
#define	SYSINFO_WDOGENABLE		1

#define	SYSINFO_TAG_SETBOOTDISCTYPE	0x11008	/* Set boot disc type         */
	/* Tag values are the same as for SYSINFO_TAG_BOOTDISCTYPE */

#define	SYSINFO_TAG_SETEMSBANK		0x1100a	/* select a Expanded-Memory-  */
						/* System bank                */
						/* bank# is the info param    */

#define	SYSINFO_TAG_SETINTERLACE	0x21005	/* Set interlace mode         */
#define	SYSINFO_TAG_SETNONINTERLACE	0x21006	/* Set non-interlace mode     */

#define	SYSINFO_TAG_SETAUDINSTATE	0x41002	/* Enable/disable audio input */
/* info values */
#define	SYSINFO_AUDIN_ENABLE		1	/* Enable audio input         */
#define	SYSINFO_AUDIN_DISABLE		0	/* disable audio input        */

#define	SYSINFO_TAG_MATHDIVOUTFIX	0x51001	/* Incorrect divide output if */
						/* one of inputs is large     */
#define	SYSINFO_TAG_MATHDIVOVERLAPFIX 	0x51002	/* Incorrect divide with      */
						/* overlap operations         */
#define	SYSINFO_TAG_MATHSWAPFIX		0x51003	/* SWAP sets incorrect status */
						/* of operation-in-progres bit*/

#define	SYSINFO_SUCCESS			0
#define	SYSINFO_FAILURE			0xfffffffd
#define	SYSINFO_NOLICENSE		0xfffffffc

#endif /* __SYSINFO_H */
