/* $Id: romsysinfo.c,v 1.11 1994/12/16 02:16:09 sdas Exp $ *
**
**	ROM code for Opera System Information
**
**	Copyright 1993, 1994 by The 3DO Company Inc.
*/

#include "types.h"
#include "sysinfo.h"
#include "inthard.h"
#include "madam.h"
#include "clio.h"
#include "rom.h"
#include "aif.h"
#include "dipir.h"
#include "kernel.h"
#include "setjmp.h"


/* The following constants are currently not used by the OS. If and    */
/* when they are used by the os, they need to be moved to RomTag table */
#define SYSINFO_AUDIN_ANVIL	0xF4000000
#define SYSINFO_AUDOUT_ANVIL	0x60011f1f
#define	SYSINFO_AUDOUT_CABLE	0xc0030f0f

#define	SYSINFO_HDELAY_CABLE	0x0000007a

#ifdef	NOTYET

#define	SHERRY_ADDR		0x00000028
#define	ROMTAG_ADDR		0x0000002c
#define	NON_ANVIL_DISPSUPPADDR	0x00000038
#define	DISP_PAL		1

/* The following values need to go into a .h file */
#define	SLOW1_PAGEREG_ADDR	0x31c00c0

#define	DISCTYPE_REG_ADDR	0x31c0080
#define	DISCTYPE_3DO		0x01
#define	DISCTYPE_AUDIO		0x02
#define	DISCTYPE_PHOTO		0x04
#define	DISCTYPE_VIDEO		0x10
#define	DISCTYPE_NAVIKEN	0x20

static uint8 disc_types[8] = { 0, 0, 0, DISCTYPE_3DO, DISCTYPE_AUDIO,
			       DISCTYPE_PHOTO, DISCTYPE_VIDEO, DISCTYPE_NAVIKEN
			     };


static PlatformID PfID = { 0, };

static int32
GetRT(int subtype, int type, RomTag *rt)
{
	uint32	*rtdata = *(uint32 **)ROMTAG_ADDR;
	uint32	*info = (uint32 *)rt;
	uint32	RTInROM;
	uint32	readbytes;
	Clio	*clio = (Clio *)CLIO;
	jmp_buf	*old_co;
	uint32	old_quiet;
	uint32	bank = 0;
	uint32	ret;
	struct KernelBase *KernelBase= *(struct KernelBase **)
	   (*(uint32 *)SHERRY_ADDR + sizeof(AIFHeader) + sizeof(_3DOBinHeader));


	RTInROM =  ((uint32)rtdata >= KernelBase->kb_MemEnd) ? 1 : 0;

	if (RTInROM)
	{
	    /*
	     * Assumption: First time this routine gets called
	     *             rombank0 is the current rombank
	     */
	    if (PfID.chip && (PfID.chip != SYSINFO_CHIP_ANVIL))
	    {
		if (bank = (clio->ADBIOBits & ADBIO_OTHERROM))
		    clio->ADBIOBits &= ~ADBIO_OTHERROM;
	    }
	    old_co = KernelBase->kb_CatchDataAborts;
	    old_quiet = KernelBase->kb_QuietAborts;
	}

	readbytes = sizeof(RomTag);
	while (readbytes)
	{
	    jmp_buf	jb;
	    uint32	tbuf;

	    if (RTInROM)
	    {
		do
		{
		    KernelBase->kb_CatchDataAborts = &jb;
		    KernelBase->kb_QuietAborts = ABT_ROMF;
		}
		while (setjmp(jb));
	    }

	    tbuf = *rtdata;

	    if (readbytes == sizeof(RomTag))
	    {
		if (!tbuf)
		{
		    /* End of RomTag Table */
		    ret = -1;
		    goto over;
		}
		/*
		 * Assumption: rt_SubSysType and rt_Type are in the
		 *             first four bytes of RomTag structure
		 */
		if ((((RomTag *)&tbuf)->rt_SubSysType != subtype) ||
		    (((RomTag *)&tbuf)->rt_Type != type))
		{
		    rtdata += sizeof(RomTag)/sizeof(uint32);
		    continue;
		}
	    }
	    *info++ = tbuf;
	    rtdata++;
	    readbytes -= 4;
	}
	ret = 0;
over:
	if (RTInROM)
	{
	    if (bank)
		clio->ADBIOBits |= ADBIO_OTHERROM;
	    KernelBase->kb_CatchDataAborts = old_co;
	    KernelBase->kb_QuietAborts = old_quiet;
	}
	return ret;
}

uint32 QueryROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	Madam	*madam = (Madam *)MADAM;
	Clio	*clio = (Clio *)CLIO;
	RomTag	rt;


	if (PfID.chip == 0)
	{
	    if (GetRT(RT_SUBSYS_ROM, ROM_PLATFORM_ID, &rt) < 0)
		return SYSINFO_UNSUPPORTEDTAG;
	    PfID.mfgr = rt.rt_Flags;
	    PfID.chip = rt.rt_TypeSpecific;
	    PfID.ver = rt.rt_Version;
	    PfID.rev = rt.rt_Revision;
	}

	switch (tag)
	{
	    /* An identifier for the platform */
	    case SYSINFO_TAG_PLATFORMID:
		if (info && (size >= sizeof(uint32)))
		    *(PlatformID *)info = PfID;
		return SYSINFO_SUCCESS;

	    case SYSINFO_TAG_CURROMBANK:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    return (madam->AnvilFeature & MADAM_AddedROM) ? 
			   SYSINFO_ROMBANK2 : SYSINFO_ROMBANK1;
		}
		break;

	    case SYSINFO_TAG_ROM2BASE:
		if (info && (size >= sizeof(uint32)))
		{
		    if (GetRT(RT_SUBSYS_ROM, ROM_ROM2_BASE, &rt) < 0)
			return SYSINFO_UNSUPPORTEDTAG;
		    *(uint32 *)info = rt.rt_Offset;
		    if (size >= 2*sizeof(uint32))
			*((uint32 *)info+1) = rt.rt_Size;
		}
		return SYSINFO_SUCCESS;
	    case SYSINFO_TAG_BOOTDISCTYPE:
		if (PfID.mfgr == SYSINFO_MFGR_TOSHIBA)
		{
		    uint8 dregval = *(uint8 *)(DISCTYPE_REG_ADDR);
		    uint32 i = 7;

		    while (i)
			if (dregval == disc_types[i]) break;
		    return i;
		}
		break;
	    case SYSINFO_TAG_SPLITNVRAM:
		if (PfID.mfgr == SYSINFO_MFGR_TOSHIBA)
		    return SYSINFO_SPLITNVRAM_TRUE;
		break;
	    case SYSINFO_TAG_CUREMSBANK:
		if (PfID.mfgr == SYSINFO_MFGR_TOSHIBA)
		{
		    if (info && (size >= sizeof(uint8)))
			*(uint8 *)info= (uint8)(*(uint32 *)(SLOW1_PAGEREG_ADDR));
		    return SYSINFO_EMSBANKS_SUPPORTED;
		}
		break;

	    case SYSINFO_TAG_GRAPHDISPSUPP:
		{
		    uint32 isPAL;
		    uint32 ret = (SYSINFO_NTSC_SUPPORTED|SYSINFO_PAL_SUPPORTED);

		    if (PfID.chip == SYSINFO_CHIP_ANVIL)
			isPAL = ~clio->ClioSbusState & CLIO_VideoMode;
		    else
			isPAL = *(uint32 *)NON_ANVIL_DISPSUPPADDR & DISP_PAL;

		    ret |= (isPAL) ? (SYSINFO_PAL_DFLT|SYSINFO_PAL_CURDISP) :
				     (SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP);

		    return ret;
		}

	    case SYSINFO_TAG_GRAPHSUPPDBLHRZ:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    /* 
		       return (madam->AnvilFeature & MADAM_HiRes) ?
			      SYSINFO_DBLHORZ_SUPPORTED : SYSINFO_DBLHORZ_NOSUPPORT;
		     */
		    return (madam->AnvilFeature & MADAM_HiRes);
		}
		break;

	    case SYSINFO_TAG_GRAPH_HDELAY:
		if (PfID.mfgr == SYSINFO_MFGR_SA)
		{
		    if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_HDELAY_CABLE;
		    return SYSINFO_HDELAY_SUPPORTED;
		}
		break;

	    case SYSINFO_TAG_CDROMSUPPORT:
		if (PfID.mfgr == SYSINFO_MFGR_CREATIVE)
		    return SYSINFO_SB_DRIVE_SUPPORTED;
		break;

	    case SYSINFO_TAG_ROMFSADDR:
		if (info && (size >= sizeof(uint32)))
		{
		    uint32 rtbase = *(uint32 *)ROMTAG_ADDR;

		    if (GetRT(RT_SUBSYS_ROM, ROM_FS_IMAGE, &rt) < 0)
			return SYSINFO_ROMFS_NOSUPPORT;
		    *(uint32 *)info = rtbase + rt.rt_Offset;
		    if (size >= 2*sizeof(uint32))
			*((uint32 *)info+1) = rt.rt_Size;
		}
		return SYSINFO_ROMFS_SUPPORTED;

	    case SYSINFO_TAG_AUDINPRESENT:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_AUDIN_ANVIL;
		    return SYSINFO_AUDIN_PRESENT;
		}
		break;

	    case SYSINFO_TAG_AUDOUTPRESENT:
		if (PfID.mfgr == SYSINFO_MFGR_SA)
		{
		    if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_AUDOUT_CABLE;
		    return SYSINFO_AUDOUT_PRESENT;
		}
		else if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_AUDOUT_ANVIL;
		    return SYSINFO_AUDOUT_PRESENT;
		}
		break;

	    case SYSINFO_TAG_MATHDIVOUTERR:
	    case SYSINFO_TAG_MATHDIVOVERLAP:
	    case SYSINFO_TAG_MATHSWAPDONE:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    return (madam->AnvilFeature & MADAM_MathFix) ?
			   SYSINFO_MATHDIVOUTERR_TRUE : SYSINFO_MATHDIVOUTERR_FALSE;
		}
		break;

	    case SYSINFO_TAG_INTLLANGCNTRY:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    return (~clio->ClioSbusState & CLIO_DfltSystemLanguage) >>
			    CLIO_DfltSystemLangShft;
		}
		break;

	    default:
		break;
	}
	return SYSINFO_BADTAG;
}

uint32 SetROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	Madam	*madam = (Madam *)MADAM;
	Clio	*clio = (Clio *)CLIO;
	RomTag	rt;


	if (PfID.chip == 0)
	{
	    if (GetRT(RT_SUBSYS_ROM, ROM_PLATFORM_ID, &rt) < 0)
		return SYSINFO_UNSUPPORTEDTAG;
	    PfID.mfgr = rt.rt_Flags;
	    PfID.chip = rt.rt_TypeSpecific;
	    PfID.ver = rt.rt_Version;
	    PfID.rev = rt.rt_Revision;
	}

	switch (tag)
	{
	    case SYSINFO_TAG_SETROMBANK:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    if ((uint32)info == SYSINFO_ROMBANK1)
			madam->AnvilFeature &= ~(MADAM_AddedROM);
		    else
			madam->AnvilFeature |= (MADAM_AddedROM);
		    return SYSINFO_SUCCESS;
		}
		break;

	    case SYSINFO_TAG_WATCHDOG:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    if ((uint32)info == SYSINFO_WDOGENABLE)
		    {
			clio->WatchDog = 11;
			madam->AnvilFeature |= MADAM_AnvilWatchDog;

			return SYSINFO_SUCCESS;
		    }
		}
		break;
	    case SYSINFO_TAG_SETBOOTDISCTYPE:
		if (PfID.mfgr == SYSINFO_MFGR_TOSHIBA)
		{
		    uint32 i = (uint32)info;

		    if ((i < 3) || (i > 7)) return SYSINFO_FAILURE;
		    *(uint8 *)(DISCTYPE_REG_ADDR) = disc_types[i];
		    return SYSINFO_SUCCESS;
		}
		break;
	    case SYSINFO_TAG_SETEMSBANK:
		if (PfID.mfgr == SYSINFO_MFGR_TOSHIBA)
		{
		    if ((uint32)info > 255) return SYSINFO_FAILURE;
		    *(uint32 *)(SLOW1_PAGEREG_ADDR) = (uint32)info;
		    return SYSINFO_SUCCESS;
		}
		break;

	    case SYSINFO_TAG_SETINTERLACE:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    clio->ClioDigVidEnc &= ~CLIO_VidModePgrsv;
		    return SYSINFO_SUCCESS;
		}
		break;

	    case SYSINFO_TAG_SETNONINTERLACE:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    clio->ClioDigVidEnc |= CLIO_VidModePgrsv;
		    return SYSINFO_SUCCESS;
		}
		break;

	    case SYSINFO_TAG_SETAUDINSTATE:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    uint32 regval;

		    regval = (*AUDIN) & 0x3FFFFFFF;
		    if ((uint32)info)
			regval |= 0x40000000;
		    else
			regval &= ~0x40000000;
		    *AUDIN = 0x80000000;	/* set reg access enable bit */
		    *AUDIN = regval;		/* set en(dis)able bit */
		    return SYSINFO_SUCCESS;
		}
		break;

	    case SYSINFO_TAG_MATHDIVOUTFIX:
	    case SYSINFO_TAG_MATHDIVOVERLAPFIX:
	    case SYSINFO_TAG_MATHSWAPFIX:
		if (PfID.chip == SYSINFO_CHIP_ANVIL)
		{
		    madam->AnvilFeature |= (MADAM_MathFix);
		    return SYSINFO_SUCCESS;
		}
		break;

	    default :
		break;
	}
	return SYSINFO_BADTAG;
}
#else	/* NOTYET */

#ifdef	ANVIL

#define	SYSINFO_CLOCK_25MHZ		25000000
#define	SYSINFO_ROM2_BASE		0x03800000
#define	SYSINFO_ROMFS_BASE		0x03040000

/* Support for ANVIL */
uint32 QueryROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	Madam	*madam = (Madam *)MADAM;
	Clio	*clio = (Clio *)CLIO;
	uint32 i;

	switch (tag)
	{
	    case SYSINFO_TAG_FIELDFREQ:
		return SYSINFO_UNSUPPORTEDTAG;

	    /* An identifier for the platform */
	    case SYSINFO_TAG_PLATFORMID:
		if (info && (size >= sizeof(PlatformID)))
		{
		    ((PlatformID *)info)->mfgr = SYSINFO_MFGR_MEC;
		    ((PlatformID *)info)->chip = SYSINFO_CHIP_ANVIL;
		    ((PlatformID *)info)->ver = 0;
		    ((PlatformID *)info)->rev = 0;
		}
		break;

	    /* MADAM id for the platform */
	    case SYSINFO_TAG_MADAMID:
		return *(vuint32 *)MADAM;

	    /* CLIO id for the platform */
	    case SYSINFO_TAG_CLIOID:
		return *(uint32 *)CLIO;

	    /* UNCLE id for the platform */
	    case SYSINFO_TAG_UNCLEID:
		return *(uint32 *)UNCLE;

	    case SYSINFO_TAG_CURROMBANK:
		return (madam->AnvilFeature & MADAM_AddedROM) ? 
			SYSINFO_ROMBANK2 : SYSINFO_ROMBANK1;

	    case SYSINFO_TAG_ROM2BASE:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = (uint32)SYSINFO_ROM2_BASE;
		break;

	    case SYSINFO_TAG_GRAPHDISPWDTH:
	    case SYSINFO_TAG_GRAPHDISPHGT:
	    case SYSINFO_TAG_GRAPHVBLINES:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_GRAPHDISPSUPP:
		i = (~clio->ClioSbusState & CLIO_VideoMode) ?
			(SYSINFO_PAL_DFLT|SYSINFO_PAL_CURDISP) :
			(SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP);
		return (SYSINFO_NTSC_SUPPORTED|SYSINFO_PAL_SUPPORTED|i);

	    case SYSINFO_TAG_GRAPHSUPPINTLC:
	    case SYSINFO_TAG_GRAPHSUPPNINTLC:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_GRAPHSUPPDBLHRZ:
		/* 
		return (madam->AnvilFeature & MADAM_HiRes) ?
			SYSINFO_DBLHORZ_SUPPORTED : SYSINFO_DBLHORZ_NOSUPPORT;
		 */
		return (madam->AnvilFeature & MADAM_HiRes);

	    case SYSINFO_TAG_GRAPHSUPPCC:
		return SYSINFO_CC_NOSUPPORT;

	    case SYSINFO_TAG_GRAPH_HDELAY:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_CDROMSUPPORT:
		return SYSINFO_MKE_DRIVE_SUPPORTED;

	    case SYSINFO_TAG_ROMFSADDR:
		if (info && (size >= sizeof(uint32)))
		    *(uint32 *)info = SYSINFO_ROMFS_BASE;
		return SYSINFO_ROMFS_SUPPORTED;

	    case SYSINFO_TAG_AUDINPRESENT:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_AUDIN_ANVIL;
		return SYSINFO_AUDIN_PRESENT;

	    case SYSINFO_TAG_AUDOUTPRESENT:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_AUDOUT_ANVIL;
		return SYSINFO_AUDOUT_PRESENT;

	    case SYSINFO_TAG_AUDDSPPCLOCK:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_CLOCK_25MHZ;
		return SYSINFO_AUDDSPP_SUCCESS;

	    case SYSINFO_TAG_MATHDIVOUTERR:
	    case SYSINFO_TAG_MATHDIVOVERLAP:
	    case SYSINFO_TAG_MATHSWAPDONE:
		return (madam->AnvilFeature & MADAM_MathFix) ?
			SYSINFO_MATHDIVOUTERR_TRUE : SYSINFO_MATHDIVOUTERR_FALSE;

	    case SYSINFO_TAG_INTLLANGCNTRY:
		return (~clio->ClioSbusState & CLIO_DfltSystemLanguage) >>
			CLIO_DfltSystemLangShft;

	    default:
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

uint32 SetROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	Madam	*madam = (Madam *)MADAM;
	Clio	*clio = (Clio *)CLIO;

	switch (tag)
	{
	    case SYSINFO_TAG_SETROMBANK:
		if ((uint32)info == SYSINFO_ROMBANK1)
			madam->AnvilFeature &= ~(MADAM_AddedROM);
		else
			madam->AnvilFeature |= (MADAM_AddedROM);
		break;

	    case SYSINFO_TAG_WATCHDOG:
		if ((uint32)info == SYSINFO_WDOGENABLE)
		{
			clio->WatchDog = 11;
			madam->AnvilFeature |= MADAM_AnvilWatchDog;
		}
		break;

	    case SYSINFO_TAG_SETINTERLACE:
		clio->ClioDigVidEnc &= ~CLIO_VidModePgrsv;
		break;
	    case SYSINFO_TAG_SETNONINTERLACE:
		clio->ClioDigVidEnc |= CLIO_VidModePgrsv;
		break;

	    case SYSINFO_TAG_SETAUDINSTATE:
		{
		    uint32 regval;

		    regval = (*AUDIN) & 0x3FFFFFFF;
		    if ((uint32)info)
			regval |= 0x40000000;
		    else
			regval &= ~0x40000000;
		    *AUDIN = 0x80000000;	/* set reg access enable bit */
		    *AUDIN = regval;		/* set en(dis)able bit */
		    return SYSINFO_SUCCESS;
		}
		break;

	    case SYSINFO_TAG_MATHDIVOUTFIX:
	    case SYSINFO_TAG_MATHDIVOVERLAPFIX:
	    case SYSINFO_TAG_MATHSWAPFIX:
		madam->AnvilFeature |= (MADAM_MathFix);
		break;

	    default :
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

#else	/* ANVIL */
#ifdef	CREATIVE

#define	SYSINFO_ROMFS_BASE		0x03040000

/* Support for CREATIVE */
uint32 QueryROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	uint32 i;

	switch (tag)
	{
	    case SYSINFO_TAG_FIELDFREQ:
		return SYSINFO_UNSUPPORTEDTAG;

	    /* An identifier for the platform */
	    case SYSINFO_TAG_PLATFORMID:
		if (info && (size >= sizeof(PlatformID)))
		{
		    ((PlatformID *)info)->mfgr = SYSINFO_MFGR_CREATIVE;
		    ((PlatformID *)info)->chip = SYSINFO_CHIP_GREEN;
		    ((PlatformID *)info)->ver = 0;
		    ((PlatformID *)info)->rev = 0;
		}
		break;

	    case SYSINFO_TAG_MADAMID:
	    case SYSINFO_TAG_CLIOID:
	    case SYSINFO_TAG_UNCLEID:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_CURROMBANK:
	    case SYSINFO_TAG_ROM2BASE:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_GRAPHDISPWDTH:
	    case SYSINFO_TAG_GRAPHDISPHGT:
	    case SYSINFO_TAG_GRAPHVBLINES:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_GRAPHDISPSUPP:
		{
		    uint32	dispsupp = *(uint32 *)0x38;

		    if (dispsupp & 1)
			i = (SYSINFO_PAL_DFLT|SYSINFO_PAL_CURDISP);
		    else
			i = (SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP);
		    return (SYSINFO_NTSC_SUPPORTED|SYSINFO_PAL_SUPPORTED|i);
		}

	    case SYSINFO_TAG_GRAPHSUPPINTLC:
	    case SYSINFO_TAG_GRAPHSUPPNINTLC:
	    case SYSINFO_TAG_GRAPHSUPPDBLHRZ:
	    case SYSINFO_TAG_GRAPHSUPPCC:
	    case SYSINFO_TAG_GRAPH_HDELAY:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_CDROMSUPPORT:
		return (SYSINFO_MKE_DRIVE_SUPPORTED|SYSINFO_SB_DRIVE_SUPPORTED);

	    case SYSINFO_TAG_ROMFSADDR:
		if (info && (size >= sizeof(uint32)))
		    *(uint32 *)info = SYSINFO_ROMFS_BASE;
		return SYSINFO_ROMFS_SUPPORTED;
		break;

	    case SYSINFO_TAG_AUDINPRESENT:
	    case SYSINFO_TAG_AUDOUTPRESENT:
	    case SYSINFO_TAG_AUDDSPPCLOCK:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_MATHDIVOUTERR:
	    case SYSINFO_TAG_MATHDIVOVERLAP:
	    case SYSINFO_TAG_MATHSWAPDONE:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_INTLLANGCNTRY:
		return SYSINFO_UNSUPPORTEDTAG;

	    default:
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

uint32 SetROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	switch (tag)
	{
	    case SYSINFO_TAG_SETROMBANK:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_WATCHDOG:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_SETINTERLACE:
	    case SYSINFO_TAG_SETNONINTERLACE:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_MATHDIVOUTFIX:
	    case SYSINFO_TAG_MATHDIVOVERLAPFIX:
	    case SYSINFO_TAG_MATHSWAPFIX:
		return SYSINFO_UNSUPPORTEDTAG;

	    default :
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

#else	/* CREATIVE */
#ifdef	CABLE

#define	SYSINFO_ROMFS_BASE		0x03048000

/* Support for CABLE */
uint32 QueryROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	uint32 i;

	switch (tag)
	{
	    case SYSINFO_TAG_FIELDFREQ:
		return SYSINFO_UNSUPPORTEDTAG;

	    /* An identifier for the platform */
	    case SYSINFO_TAG_PLATFORMID:
		if (info && (size >= sizeof(PlatformID)))
		{
		    ((PlatformID *)info)->mfgr = SYSINFO_MFGR_SA;
		    ((PlatformID *)info)->chip = SYSINFO_CHIP_GREEN;
		    ((PlatformID *)info)->ver = 0;
		    ((PlatformID *)info)->rev = 0;
		}
		break;

	    case SYSINFO_TAG_MADAMID:
	    case SYSINFO_TAG_CLIOID:
	    case SYSINFO_TAG_UNCLEID:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_CURROMBANK:
	    case SYSINFO_TAG_ROM2BASE:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_GRAPHDISPWDTH:
	    case SYSINFO_TAG_GRAPHDISPHGT:
	    case SYSINFO_TAG_GRAPHVBLINES:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_GRAPHDISPSUPP:
		{
		    uint32	dispsupp = *(uint32 *)0x38;

		    if (dispsupp & 1)
			i = (SYSINFO_PAL_DFLT|SYSINFO_PAL_CURDISP);
		    else
			i = (SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP);
		    return (SYSINFO_NTSC_SUPPORTED|SYSINFO_PAL_SUPPORTED|i);
		}

	    case SYSINFO_TAG_GRAPHSUPPINTLC:
	    case SYSINFO_TAG_GRAPHSUPPNINTLC:
	    case SYSINFO_TAG_GRAPHSUPPDBLHRZ:
	    case SYSINFO_TAG_GRAPHSUPPCC:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_GRAPH_HDELAY:
		if (info && (size >= sizeof(uint32)))
		    *(uint32 *)info = SYSINFO_HDELAY_CABLE;
		return SYSINFO_HDELAY_SUPPORTED;

	    case SYSINFO_TAG_CDROMSUPPORT:
		return SYSINFO_MKE_DRIVE_SUPPORTED;

	    case SYSINFO_TAG_ROMFSADDR:
		if (info && (size >= sizeof(uint32)))
		    *(uint32 *)info = SYSINFO_ROMFS_BASE;
		return SYSINFO_ROMFS_SUPPORTED;

	    case SYSINFO_TAG_AUDINPRESENT:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_AUDOUTPRESENT:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = SYSINFO_AUDOUT_CABLE;
		return SYSINFO_AUDOUT_PRESENT;

	    case SYSINFO_TAG_AUDDSPPCLOCK:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_MATHDIVOUTERR:
	    case SYSINFO_TAG_MATHDIVOVERLAP:
	    case SYSINFO_TAG_MATHSWAPDONE:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_INTLLANGCNTRY:
		return SYSINFO_UNSUPPORTEDTAG;

	    default:
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

uint32 SetROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	switch (tag)
	{
	    case SYSINFO_TAG_SETROMBANK:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_WATCHDOG:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_SETINTERLACE:
	    case SYSINFO_TAG_SETNONINTERLACE:
		return SYSINFO_UNSUPPORTEDTAG;

	    case SYSINFO_TAG_MATHDIVOUTFIX:
	    case SYSINFO_TAG_MATHDIVOVERLAPFIX:
	    case SYSINFO_TAG_MATHSWAPFIX:
		return SYSINFO_UNSUPPORTEDTAG;

	    default :
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

#else	/* CABLE */
#ifdef	NAVIKEN

#define	SLOW1_PAGEREG_ADDR	0x31c00c0

#define	DISCTYPE_REG_ADDR	0x31c0080
#define	DISCTYPE_3DO		0x01
#define	DISCTYPE_AUDIO		0x02
#define	DISCTYPE_PHOTO		0x04
#define	DISCTYPE_VIDEO		0x10
#define	DISCTYPE_NAVIKEN	0x20

uint8 disc_types[8] =	{ 0, 0, 0, DISCTYPE_3DO, DISCTYPE_AUDIO,
			  DISCTYPE_PHOTO, DISCTYPE_VIDEO, DISCTYPE_NAVIKEN
			};

/* Support for Naviken */
uint32 QueryROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
uint32 size;
{
	switch (tag)
	{
	    /* An identifier for the platform */
	    case SYSINFO_TAG_PLATFORMID:
		if (info && (size >= sizeof(PlatformID)))
		{
		    ((PlatformID *)info)->mfgr = SYSINFO_MFGR_TOSHIBA;
		    ((PlatformID *)info)->chip = SYSINFO_CHIP_GREEN;
		    ((PlatformID *)info)->ver = 0;
		    ((PlatformID *)info)->rev = 0;
		}
		break;

	    case SYSINFO_TAG_GRAPHDISPSUPP:
		{
		    uint32	i;
		    uint32	dispsupp = *(uint32 *)0x38;

		    if (dispsupp & 1)
			i = (SYSINFO_PAL_DFLT|SYSINFO_PAL_CURDISP);
		    else
			i = (SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP);
		    return (SYSINFO_NTSC_SUPPORTED|SYSINFO_PAL_SUPPORTED|i);
		}
		break;
	    case SYSINFO_TAG_BOOTDISCTYPE:
		{
		    uint8 dregval = *(uint8 *)(DISCTYPE_REG_ADDR);
		    uint32 i = 7;

		    while (i)
			if (dregval == disc_types[i]) break;
		    return i;
		}
		break;
	    case SYSINFO_TAG_SPLITNVRAM:
		return SYSINFO_SPLITNVRAM_TRUE;
	    case SYSINFO_TAG_CUREMSBANK:
		if (info && (size >= sizeof(uint8)))
			*(uint8 *)info= (uint8)(*(uint32 *)(SLOW1_PAGEREG_ADDR));
		return SYSINFO_EMSBANKS_SUPPORTED;
	    default:
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

uint32 SetROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
uint32 size;
{
	switch (tag)
	{
	    case SYSINFO_TAG_SETBOOTDISCTYPE:
		{
		    uint32 i = (uint32)info;

		    if ((i < 3) || (i > 7)) return SYSINFO_FAILURE;
		    *(uint8 *)(DISCTYPE_REG_ADDR) = disc_types[i];
		}
		break;
	    case SYSINFO_TAG_SETEMSBANK:
		if ((uint32)info > 255) return SYSINFO_FAILURE;
		*(uint32 *)(SLOW1_PAGEREG_ADDR) = (uint32)info;
		break;
	    default:
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

#else	/* NAVIKEN */

/* Support for Opera */
#define	SYSINFO_ROMFS_BASE		0x03040000

uint32 QueryROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
uint32 size;
{
	switch (tag)
	{
	    case SYSINFO_TAG_FIELDFREQ:
	    case SYSINFO_TAG_PLATFORMID:
	    case SYSINFO_TAG_MADAMID:
	    case SYSINFO_TAG_CLIOID:
	    case SYSINFO_TAG_UNCLEID:
	    case SYSINFO_TAG_GRAPHDISPWDTH:
	    case SYSINFO_TAG_GRAPHDISPHGT:
	    case SYSINFO_TAG_GRAPHVBLINES:
		return SYSINFO_UNSUPPORTEDTAG;
	    case SYSINFO_TAG_GRAPHDISPSUPP:
#ifndef	FZ1_UK
		{
		    uint32	i;
		    uint32	dispsupp = *(uint32 *)0x38;

		    if (dispsupp & 1)
			i = (SYSINFO_PAL_DFLT|SYSINFO_PAL_CURDISP);
		    else
			i = (SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP);
		    return (SYSINFO_NTSC_SUPPORTED|SYSINFO_PAL_SUPPORTED|i);
		}

#else	/* FZ1_UK */
		return (SYSINFO_PAL_SUPPORTED|SYSINFO_PAL_DFLT|SYSINFO_PAL_CURDISP);
#endif	/* FZ1_UK */
	    case SYSINFO_TAG_GRAPHSUPPINTLC:
	    case SYSINFO_TAG_GRAPHSUPPNINTLC:
	    case SYSINFO_TAG_GRAPHSUPPDBLHRZ:
	    case SYSINFO_TAG_GRAPHSUPPCC:

	    case SYSINFO_TAG_ROMFSADDR:
		if (info && (size >= sizeof(uint32)))
		    *(uint32 *)info = SYSINFO_ROMFS_BASE;
		return SYSINFO_ROMFS_SUPPORTED;
		break;

	    case SYSINFO_TAG_AUDOUTPRESENT:
	    case SYSINFO_TAG_AUDDSPPCLOCK:
	    case SYSINFO_TAG_MATHDIVOUTERR:
	    case SYSINFO_TAG_MATHDIVOVERLAP:
	    case SYSINFO_TAG_MATHSWAPDONE:
	    case SYSINFO_TAG_INTLDFLTLANG:
		return SYSINFO_UNSUPPORTEDTAG;
	    default:
		return SYSINFO_BADTAG;
	}
}

uint32 SetROMSysInfo(tag, info, size)
uint32 tag;
void   *info;
uint32 size;
{
	return SYSINFO_UNSUPPORTEDTAG;
}

#endif	/* NAVIKEN */
#endif	/* CABLE */
#endif	/* CREATIVE */
#endif	/* ANVIL */
#endif	/* NOTYET */
