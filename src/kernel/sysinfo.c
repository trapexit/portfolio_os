/* $Id: sysinfo.c,v 1.13.1.1 1995/02/01 22:09:27 sdas Exp $ */

#include "types.h"
#include "sysinfo.h"
#include "inthard.h"
#include "clio.h"
#include "bootdata.h"

/* code for Opera System Information */
#include "debug.h"

#define DBUG(x)	/*printf x*/
#define DBUG1(x)	/*printf x*/

#define	DIPIR_QSI_ADDR		0x0000020c
#define	DIPIR_SSI_ADDR		0x00000210

#define	SYSINFO_AUDOUT_OPERA	0xc0050f0f
#define	SYSINFO_CLOCK_25MHZ	25000000
#define	SYSINFO_ROM2_BASE	0x03000000
#define	SYSINFO_ROMFS_BASE	0x03000000+(160*1024)

uint32 QueryCDSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	Clio	*clio = (Clio *)CLIO;

	switch (tag)
	{
	    case SYSINFO_TAG_FIELDFREQ:		return SYSINFO_FREQ_50HZ;
	    case SYSINFO_TAG_PLATFORMID:
		if (info && (size >= sizeof(PlatformID)))
		{
		    ((PlatformID *)info)->mfgr = SYSINFO_MFGR_MEC;
		    ((PlatformID *)info)->chip = SYSINFO_CHIP_GREEN;
		    ((PlatformID *)info)->ver = 0;
		    ((PlatformID *)info)->rev = 0;
		}
		break;
	    case SYSINFO_TAG_MADAMID:		return *(vuint32 *)MADAM;
	    case SYSINFO_TAG_CLIOID:		return *(vuint32 *)CLIO;
	    case SYSINFO_TAG_UNCLEID:		return *(vuint32 *)UNCLE;
	    case SYSINFO_TAG_CURROMBANK:
		return (clio->ADBIOBits & ADBIO_OTHERROM) ?
			SYSINFO_ROMBANK2 : SYSINFO_ROMBANK1;
		break;
	    case SYSINFO_TAG_ROM2BASE:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = (uint32)SYSINFO_ROM2_BASE;
		break;
	    case SYSINFO_TAG_BOOTDISCTYPE:	return SYSINFO_DISCTYPE_NOTSET;
	    case SYSINFO_TAG_SPLITNVRAM:
		return SYSINFO_SPLITNVRAM_FALSE;
	    case SYSINFO_TAG_CUREMSBANK:
		return SYSINFO_EMSBANKS_NOSUPPORT;
	    case SYSINFO_TAG_GRAPHDISPWDTH:
	    case SYSINFO_TAG_GRAPHDISPHGT:	return SYSINFO_UNSUPPORTEDTAG;
	    case SYSINFO_TAG_GRAPHVBLINES:	return SYSINFO_DISPVLINES21;
	    case SYSINFO_TAG_GRAPHDISPSUPP:
		return (SYSINFO_NTSC_SUPPORTED|SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP);
	    case SYSINFO_TAG_GRAPHSUPPINTLC:	return SYSINFO_INTLC_SUPPORTED;
	    case SYSINFO_TAG_GRAPHSUPPNINTLC:	return SYSINFO_NINTLC_NOSUPPORT;
	    case SYSINFO_TAG_GRAPHSUPPDBLHRZ:	return SYSINFO_DBLHORZ_NOSUPPORT;
	    case SYSINFO_TAG_GRAPHSUPPCC:	return SYSINFO_CC_NOSUPPORT;
	    case SYSINFO_TAG_GRAPH_HDELAY:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = (uint32)*HDELAY;
		return SYSINFO_HDELAY_SUPPORTED;
	    case SYSINFO_TAG_CDROMSUPPORT:	return SYSINFO_MKE_DRIVE_SUPPORTED;
	    case SYSINFO_TAG_ROMFSADDR:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = (uint32)SYSINFO_ROMFS_BASE;
		return SYSINFO_ROMFS_SUPPORTED;
	    case SYSINFO_TAG_AUDINPRESENT:	return SYSINFO_AUDIN_ABSENT;
	    case SYSINFO_TAG_AUDOUTPRESENT:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = (uint32)SYSINFO_AUDOUT_OPERA;
		return SYSINFO_AUDOUT_PRESENT;
	    case SYSINFO_TAG_AUDDSPPCLOCK:
		if (info && (size >= sizeof(uint32)))
			*(uint32 *)info = (uint32)SYSINFO_CLOCK_25MHZ;
		return SYSINFO_AUDDSPP_SUCCESS;
	    case SYSINFO_TAG_MATHDIVOUTERR:	return SYSINFO_MATHDIVOUTERR_FALSE;
	    case SYSINFO_TAG_MATHDIVOVERLAP:	return SYSINFO_MATHDIVOVERLAP_FALSE;
	    case SYSINFO_TAG_MATHSWAPDONE:	return SYSINFO_MATHSWAPDONE_FALSE;
	    case SYSINFO_TAG_INTLDFLTLANG:	return SYSINFO_UNSUPPORTEDTAG;
	    case SYSINFO_TAG_INTLLANGCNTRY:	return SYSINFO_UNSUPPORTEDTAG;
	    default:			 	return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}

uint32 SetCDSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	Clio	*clio = (Clio *)CLIO;

	switch (tag)
	{
	    case SYSINFO_TAG_SETROMBANK:
		if ((uint32)info == SYSINFO_ROMBANK1)
			clio->ADBIOBits &= ~(ADBIO_OTHERROM);
		else
			clio->ADBIOBits |= (ADBIO_OTHERROM | ADBIO_OTHERROM_EN);
		break;

	    case SYSINFO_TAG_WATCHDOG:
		if ((uint32)info == SYSINFO_WDOGENABLE)
			clio->WatchDog = 11;
		break;
	    case SYSINFO_TAG_SETINTERLACE:
	    case SYSINFO_TAG_SETNONINTERLACE:
		{
			uint32	adbioval = clio->ADBIOBits;

			adbioval |= ADBIO_BIT0_EN;
			adbioval ^= ADBIO_BIT0;
			clio->ADBIOBits = adbioval;
		}
		break;
	    default:
		return SYSINFO_FAILURE;
	}
	return SYSINFO_SUCCESS;
}

/**
|||	AUTODOC PRIVATE spg/kernel/SuperQuerySysInfo
|||	SuperQuerySysInfo - Query system information
|||
|||	  Synopsis
|||
|||	    uint32 SuperQuerySysInfo(uint32 tag, void *info, size_t size)
|||
|||	  Description
|||
|||	    SuperQuerySysInfo returns the current system information
|||	    for the parameter tag.
|||
|||	  Arguments
|||
|||	    tag                         The system information to query. It
|||	                                must be one of the SuperQuerySysInfo
|||	                                tags defined in sysinfo.h.
|||
|||	    info                        The pointer to the optional buffer
|||	                                area where to return additional
|||	                                information. If no additional
|||	                                information is available for the
|||	                                tag or no additional information
|||	                                is desired, this parameter should
|||	                                be set to NULL.
|||
|||	    size                        The size of the info buffer in bytes.
|||	                                The value of this parameter is
|||	                                ignored if info is set to NULL.
|||
|||	  Return Value
|||
|||	    The procedure returns a system information for the tag.
|||	    If the possible values are predefined for the tag, the
|||	    appropriate value is returned; else SYSINFO_SUCCESS is returned
|||	    and the information for the tag is returned through info buffer.
|||	    SYSINFO_BADTAG is returned in case of an invalid tag.
|||	    SYSINFO_UNSUPPORTEDDTAG is returned in case the tag is unsupported
|||	    in the release.
|||
|||	  Implementation
|||
|||	    Supervisor-mode folio call implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	    super.h                     ANSI C Prototype
|||	    sysinfo.h                   Tags Definition
|||
|||	  Caveats
|||
|||	    SuperQuerySysInfo relies on the system ROM to provide a
|||	    system information query procedure to which the query is to be
|||	    passed on. If the system ROM does not provide the information
|||	    query procedure, a default information appropriate for the
|||	    Opera hardware is instead returned by SuperQuerySysInfo.
|||
|||	  See Also
|||
|||	    SuperSetSysInfo()
**/

uint32
SuperQuerySysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	uint32 ret = SYSINFO_BADTAG;
	uifunc_t dipir_ROMQSI = (uifunc_t)DIPIR_QSI_ADDR;
	uint32 ROMQSIval = *(uint32 *)dipir_ROMQSI;

	DBUG(("SuperQuerySysInfo(%lx,%lx,%lx)\n",tag,info,size));

	if (ROMQSIval)
		ret = (uint32)(*dipir_ROMQSI)(tag, info, size);

	if ((ret == SYSINFO_BADTAG) || (ret == SYSINFO_UNSUPPORTEDTAG))
		ret = QueryCDSysInfo(tag, info, size);

	DBUG(("SuperQuerySysInfo result = %lx\n", ret));
	return ret;
}

uint32
CheckDevicePermission(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	extern uint32	DevicePermissions;

	switch (tag)
	{
	    case SYSINFO_TAG_SETAUDINSTATE:
		if ((DevicePermissions & BOOTDATA_DEVICEPERM_AUDIOIN) == 0)
		    return SYSINFO_NOLICENSE;
		break;
	    default:
		break;
	}
	return 0;
}

/**
|||	AUTODOC PRIVATE spg/kernel/SuperSetSysInfo
|||	SuperSetSysInfo - Set system information
|||
|||	  Synopsis
|||
|||	    uint32 SuperSetSysInfo(uint32 tag, void *info, size_t size)
|||
|||	  Description
|||
|||	    SuperSetSysInfo sets the system information requested through tag.
|||
|||	  Arguments
|||
|||	    tag                         The system information to set. It
|||	                                must be one of the SuperSetSysInfo
|||	                                tags defined in sysinfo.h.
|||
|||	    info                        This parameter type is tag specific.
|||	                                If a tag requires upto two additional
|||	                                informations, the first additional
|||	                                information is passed through info.
|||	                                If a tag requires three or more
|||	                                additional informations, the info
|||	                                points to a buffer containing the
|||	                                informations. The specifics are
|||	                                detailed with every SuperSetSysInfo
|||	                                tag in sysinfo.h.
|||
|||	    size                        This parameter type is tag specific.
|||	                                If info points to a buffer, then
|||	                                size specifies the size of the
|||	                                buffer in bytes. Otherwise, it may
|||	                                be used to pass additional informations
|||	                                if required for the tag. The specifics
|||	                                are detailed with every SuperSetSysInfo
|||	                                tag in sysinfo.h.
|||
|||	  Return Value
|||
|||	    SYSINFO_SUCCESS is returned if information could be successfully
|||	    set, else SYSINFO_FAILURE is returned.
|||	    SYSINFO_BADTAG is returned in case of an invalid tag.
|||	    SYSINFO_UNSUPPORTEDDTAG is returned in case the tag is unsupported
|||	    in the release.
|||
|||	  Implementation
|||
|||	    Supervisor-mode folio call implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	    super.h                     ANSI C Prototype
|||	    sysinfo.h                   Tags Definition
|||
|||	  Caveats
|||
|||	    SuperSetSysInfo relies on the system ROM to provide a procedure
|||	    for setting system information to which the request is to be
|||	    passed on. If the system ROM does not provide such a procedure,
|||	    the information is set as would be appropriate for the Opera
|||	    hardware.
|||
|||	  See Also
|||
|||	    SuperQuerySysInfo()
**/

uint32
SuperSetSysInfo(tag, info, size)
uint32 tag;
void   *info;
size_t size;
{
	uifunc_t dipir_ROMQSI = (uifunc_t)DIPIR_QSI_ADDR;
	uint32 ROMQSIval = *(uint32 *)dipir_ROMQSI;
	uifunc_t dipir_ROMSSI = (uifunc_t)DIPIR_SSI_ADDR;
	uint32 ROMSSIval = *(uint32 *)dipir_ROMSSI;
	int32 ret;

	DBUG(("SuperSetSysInfo(%lx,%lx,%lx)\n",tag,info,size));

	ret = CheckDevicePermission(tag, info, size);
	if (ret >= 0)
	{
	    ret = SYSINFO_BADTAG;

	    if (ROMSSIval && ROMQSIval)
		ret = (uint32)(*dipir_ROMSSI)(tag, info, size);

	    if ((ret == SYSINFO_BADTAG) || (ret == SYSINFO_UNSUPPORTEDTAG))
		ret = SetCDSysInfo(tag, info, size);
	}

	DBUG(("SuperSetSysInfo result = %lx\n", ret));
	return ret;
}
