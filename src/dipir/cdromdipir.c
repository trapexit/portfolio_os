/*	$Id: cdromdipir.c,v 1.82.1.3 1994/12/21 21:43:02 markn Exp $
**
**	Disk resident security code
**
**	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
*/
#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "setjmp.h"
#include "rom.h"
#include "dipir.h"
#include "aif.h"
#include "bootdata.h"

/* cromdipir.c : c code to handle dipir events */
/* This is cdrom resident and is loaded by the rom resident dipir code */
/* stack has been setup, the DipirEnv * is passed in. */


#define NO_MULTISESSION_DISC	1
#define NO_MIXED_MODE_DISC	1

extern void EnableIrq(void);
extern uint32 CurrentOsVersion(void);
uint8 Current3DOFlags(void);
extern int VerifyDiscDigest(DipirEnv *de);
extern void FindMemSize(uint32 *pDramSize, uint32 *pVramSize);
extern uint32 ModMCTL(uint32 op, uint32 newmctl);
extern int32 DefaultDisplayImage(void *image0, char *pattern);

#if defined(APPSPLASH) && defined(APPDIGEST)
/*
 * This is the case where we're building the cddipir for inclusion
 * in the system ROM (in driver.pak).  In this case we have to be able
 * to deal with old format misc code on some very early discs.
 * cddipirs that are built for inclusion on new discs don't need 
 * to support old misc code because a new disc will have new misc code.
 */
#define SUPPORT_OLD_MISCCODE 1
#endif

#ifdef SUPPORT_OLD_MISCCODE
static void RelocateOldMiscCode(void);
#endif

DipirRoutines *dipr;

#ifdef COMBINE
extern DeviceRoutines *dvr;
#else
DeviceRoutines *dvr;
#endif

#if defined(APPDIGEST) && defined(ENCRYPT)
extern int32 digest_start_block,digest_size_bytes, app_start_block;
extern int MaxPain;
#endif    


struct component {
	uint32 comp_Address;
	uint32 comp_Size;
} Component[MAX_OS_COMPONENTS];
uint32 numComponents = 0;
uint32 componentLoadAddr;
static uint32 dramSize;
static uint32 vramSize;
static uint32 pageSize = 0;

BootData bootData;

#define PHYS_BS	2048

/* A value which, when written into a video pixel, turns it (roughly) gold */
#define	GOLD_COLOR	0x63e0

/* Assume ROUNDUP size is a power of 2 */
#define	ROUNDUP(v,sz)	(((v)&((sz)-1)) ? ((v)+(sz)-((v)&((sz)-1))) : (v))

char Copyright[] = "Copyright 1993 The 3DO Company, All rights reserved";

/* 
 * Load and/or verify OS component.
 * If doload is FALSE, the component is just verified (signature checked).
 * If doload is TRUE, the component is loaded into memory at OS_LOAD_ADDR.
 * It is the caller's responsibility to transfer the component 
 * to its correct address, if necessary, by calling RelocateOsComponent.
 */
#define	LoadOsComponent(de,blk,siz,appkey) \
	ReadOsComponent(de,(char*)OS_LOAD_ADDR,blk,siz,TRUE,appkey)
#define	VerifyOsComponent(de,blk,siz,appkey) \
	ReadOsComponent(de,(char*)0,blk,siz,FALSE,appkey)

static int
ReadOsComponent(DipirEnv *de, char *dst, uint32 startblock, uint32 bytes, 
		bool doload, bool appkey)
{
	int status;
	uint32 os_blocks;
	char *NextBuff;

	PUTS("ReadOsComponent:"); PUTHEX(dst);
	PUTS(" startblock="); PUTHEX(startblock);
	PUTS(" bytes="); PUTHEX(bytes);

	os_blocks = (bytes - SIG_LEN) / PHYS_BS;
	DoubleBuffer(de);
	ASYNCREADBLOCK(startblock);
	startblock += 1;
	os_blocks--;
	bytes -= PHYS_BS;

	DIGESTINIT();
	while (os_blocks)
	{
		if (WAIT()) {
			PUTS("READ ERR");
			return 0;
		}
		SwitchBuffers(de);
		ASYNCREADBLOCK(startblock);
		startblock += 1;
		os_blocks--;
		bytes -= PHYS_BS;
		UPDATEDIGEST(NextBuff, PHYS_BS);
		if (doload) 
		{
			MEM_MOVE((POINTER)dst, (POINTER)NextBuff, PHYS_BS);
			dst += PHYS_BS;
		}
	}
	if (WAIT()) {
		PUTS("READ ERR");
		return 0;
	}
	UPDATEDIGEST(de->CurrentBuff, PHYS_BS);
	if (doload) {
		MEM_MOVE((POINTER)dst,(POINTER)de->CurrentBuff,PHYS_BS);	
		dst += PHYS_BS;
	}

	/* now finish up */
	SingleBuffer(de, de->databuff1);
	READBLOCK(startblock);
	startblock += 1;
	os_blocks--;
	UPDATEDIGEST(de->databuff1, bytes-SIG_LEN);
	if (doload) {
		MEM_MOVE((POINTER)dst,(POINTER)de->databuff1,bytes-SIG_LEN);
	}

	if (bytes <= PHYS_BS)
	{
		/* Signature is completely within this last block. */
		MEM_MOVE((POINTER)de->databuff2, 
			(POINTER)de->databuff1 + bytes - SIG_LEN, SIG_LEN);
	} else
	{
		/* Signature straddles the end of this block. */
		MEM_MOVE(de->databuff2, 
			de->databuff1 + bytes - SIG_LEN,
			PHYS_BS - bytes + SIG_LEN);
		/* now get the last block */
		bytes -= PHYS_BS;
		READ(startblock, bytes);
		MEM_MOVE((POINTER)de->databuff2+SIG_LEN-bytes,
			(POINTER)de->databuff1, bytes);
	}

	FINALDIGEST();
	if (appkey)
		status = RSAFINALAPP((unsigned char *)de->databuff2, SIG_LEN);
	else
		status = RSAFINALTHDO((unsigned char *)de->databuff2, SIG_LEN);
	return status;
}

uint32 *
RelocateOsFile(uint32 *image, uint32 addr, uint32 size, uint32 roundup, bool reserveStack)
{
	uint32 *dst;
	struct AIFHeader *aif;

	if (addr != 0) {
		componentLoadAddr = addr;
	} else if (roundup != 0) {
		componentLoadAddr = ROUNDUP(componentLoadAddr, roundup);
	}

	/* If we need to reserve an external stack,
	 * bump up the load addr to leave room for the stack. */
	if (reserveStack) {
		struct _3DOBinHeader *thdo = (struct _3DOBinHeader *)
			(((uint8*)image) + sizeof(struct AIFHeader));
		componentLoadAddr += thdo->_3DO_Stack;
		PUTS("Reserve stack: bump addr to "); PUTHEX(componentLoadAddr);
	}

	/* Save the load addr for the kernel to see. */
	if (numComponents >= MAX_OS_COMPONENTS) {
		PUTS("Too many OS components");
		RESETDEVANDEXIT();
		return NULL;
	}
	Component[numComponents].comp_Address = componentLoadAddr;
	Component[numComponents].comp_Size = size;
	numComponents++;
	PUTS("Load component at "); PUTHEX(componentLoadAddr);
	PUTS("     size="); PUTHEX(size);

	/* Copy the image to the load address */
	dst = (uint32*) componentLoadAddr;
	for ( ;  size != 0;  size -= sizeof (uint32)) {
		*dst++ = *image++;
	}

	/* Set the load addr to the end of this file,
	 * so the next file starts after this one. */
	aif = (struct AIFHeader *) componentLoadAddr;
	size = aif->aif_ImageROsize +
		aif->aif_ImageRWsize +
		aif->aif_ZeroInitSize;
	componentLoadAddr += size;
	return image;
}

static void
RelocateOsComponent(bool reserveStack)
{
	uint32 *image;
	uint32 addr;
	uint32 size;
	uint32 roundup;
	bool first;

	/* 
	 * The component consists of an 8 byte header followed by a list
	 * of files.  Each file in the component starts with an 8 byte 
	 * header (two words), giving the file's load address and size.
	 */
	image = (uint32*) (OS_LOAD_ADDR + 8);
	/* First file in a component does not need to be page-aligned;
	 * subsequent ones do. */
	first = TRUE;
	while ((addr = *image++) != 0) 
	{
		/* Ignore the loadaddr in the file and pick our own addr */
		size = *image++;
		if (first)
		{
			addr = 0;
			roundup = 0;
		} else
		{
			roundup = pageSize;
		}
		image = RelocateOsFile(image, addr, size, roundup, reserveStack);
		first = FALSE;
		reserveStack = FALSE; // KLUDGE
	}
}

#ifdef ROMBUILD
static int
LoadAndRelocateRomOsComponent(int subsysType, int type, 
			uint32 roundup, bool forceAddr, bool reserveStack)
{
	void *fd;
	uint32 size;
	RomTag *rt;
	uint32 addr;

	fd = OPENROMFILE(subsysType, type);
	if (fd == NULL) {
		PUTS("cannot open ROM comp");
		return 0;
	}
	size = READROMFILE(fd, 0, MAX_OS_SIZE, (uint8*)OS_LOAD_ADDR);
	CLOSEROMFILE(fd);
	if (size <= sizeof(struct AIFHeader)) {
		PUTS("short ROM comp:"); PUTHEX(size);
		return 0;
	}
	if (forceAddr)
	{
		rt = FINDSYSROMTAG(subsysType, type);
		if (rt == NULL) {
			PUTS("no rom tag for ROM comp??");
			return 0;
		}
		addr = rt->rt_Reserved3[0];
	} else
	{
		addr = 0;
	}
PUTS("Reloc ROM comp to "); PUTHEX(addr);
	/* Load address is in rt_Reserved3[0] of RomTag entry */
	RelocateOsFile((uint32*)OS_LOAD_ADDR, addr,
		size, roundup, reserveStack);
	return 1;
}
#endif /* ROMBUILD */

static void
memset(uint8 *p, uint8 v, uint32 size)
{
	while (size-- > 0)
		*p++ = v;
}

static void
ClearGaps(void)
{
	uint32 comp;
	uint32 addr;
	uint32 size;

	for (comp = 0;  comp < numComponents-1;  comp++)
	{
		if (Component[comp].comp_Address >= 
			Component[comp+1].comp_Address)
		{
			/* Components should be sorted by address! */
			PUTS("********** ClearGaps: unsorted! **********\n");
			return;
		}
		addr = Component[comp].comp_Address + Component[comp].comp_Size;
		size = Component[comp+1].comp_Address - addr;
		PUTS("clr gap @"); PUTHEX(addr);
		PUTS(" size="); PUTHEX(size);
		memset((uint8*)addr, 0, size);
	}
}

#ifdef APPSPLASH
#define	VerifyAllOsComponents(de,s1,b1,s2,b2,s3,b3) \
	internalVerifyAllOsComponents(de,s1,b1,s2,b2,s3,b3) 
#else
#define	VerifyAllOsComponents(de,s1,b1,s2,b2,s3,b3) \
	internalVerifyAllOsComponents(de,s1,b1,s2,b2) 
#endif

static int
VerifyAllOsComponents(DipirEnv *de,
			uint32 osstartblock, uint32 osbytes,
			uint32 miscstartblock, uint32 miscbytes,
			uint32 splashstartblock, uint32 splashbytes)
{
	int status;

	if (osstartblock != 0)
	{
		status = VerifyOsComponent(de, osstartblock, osbytes, 0);
		if (status == 0)
			return 0;
	}
	if (miscstartblock != 0)
	{
		status = VerifyOsComponent(de, miscstartblock, miscbytes, 0);
		if (status == 0)
			return 0;
	}
#ifdef APPSPLASH
	if (splashstartblock != 0) 
	{
		status = VerifyOsComponent(de, splashstartblock, splashbytes, 1);
#ifdef ENCRYPT
		/* Don't require splash screen to be signed in unenc version.
		 * This is so developers don't need the signfile tool. */
		if (status == 0)
			return 0;
#endif
	}
#endif /* APPSPLASH */
	return 1;
}

#ifdef SIGN_LAUNCHME
int RSACheckLaunchme(DipirEnv *de,uint32 startblock)
{
    int status=1;
    return status;
#if 0
    PUTS("===RSACheckLaunchme===");

    SingleBuffer(de, de->databuff1);
    PUTS("startblock=");PUTHEX(startblock);

    READBLOCK(startblock);
    
    status = 1; /* RSAFINALTHDO((unsigned char *)p[1], SIG_LEN); */
    return status;
#endif
}
#endif /* SIGN_LAUNCHME */


#ifdef NO_MULTISESSION_DISC
static int MultiSession(DipirEnv *de) 
{
	READSESSIONINFO();
	if (de->statbuff[1] & 0x80) return 1;
	return 0;
}
#endif /* NO_MULTISESSION_DISC */

static int GoldDisc(DipirEnv *de) 
{
	/* hardware doesn't do this now, but someday */
        if (de->dipir_version > 0)
		return (int)CHECKGOLDDISC();
	return 0;	/* assume not a gold disc */
}

static int AuthPlantCheck(DipirEnv *de) 
{
	int32 plant;

	/* hardware doesn't do this now, but someday */
        if (de->dipir_version == 0) {
		/* assume this disc pressed at an authorized plant */
		return 1;
	}
	plant = GETMFGPLANT();
	return (plant == 0);	
}

/*
 * Exit if there are any CDDA tracks on the disc.  (3DO disks
 * have 16 bit audio in CDROM data format, and never have CDDA tracks).
 */
static DIPIR_RETURN
CheckDiscMode(DipirEnv *de) 
{
#ifdef NO_MIXED_MODE_DISC
	int blkcnt;
        volatile TOCInfo *ti;
        DiscInfo *di;
    	int LastTrack;
	int TotalFrames;
    	int i;

        di = (DiscInfo *)de->statbuff;
        if (READDISCINFO() < 0) return DIPIR_RETURN_ROMAPPDISC;
        LastTrack = di->di_LastTrackNumber;

	PUTS("LastTrack=");PUTHEX(LastTrack);
	blkcnt = (int)de->DiscLabel->dl_VolumeBlockCount;
	PUTS("BlockCount=");PUTHEX(blkcnt);
	PUTS("###########");
	TotalFrames = di->di_MSFEndAddr_Frm + 
		di->di_MSFEndAddr_Sec * FRAMEperSEC +
		di->di_MSFEndAddr_Min * FRAMEperSEC * SECperMIN - 
		2 * FRAMEperSEC;
	PUTS("TotalFrames=");PUTHEX(TotalFrames);

	if (LastTrack > 1) return DIPIR_RETURN_ROMAPPDISC;
	if (TotalFrames > blkcnt + 512)	/* allow 1M of slop */
	{
		/* We are unable to verify enough data, kick it out */
		PUTS("Too many extra frames on this disc!!!!!!");
		return DIPIR_RETURN_TROJAN;
	}

        ti = (TOCInfo *)de->statbuff;

    	PUTS("ReadTOCS ");
	/* None of this really matters since we only allow one track per 3do */
	/* disc at this time, and it must be a digital track, as verified in */
	/* cdipir.c */
    	for (i = 1; i <= LastTrack; i++) {
		uint8 AddrCntrl;
		if (READTOC(i) < 0)
			return DIPIR_RETURN_ROMAPPDISC;
		AddrCntrl = ti->toc_AddrCntrl;
		PUTS("AdrCtl="); PUTHEX(AddrCntrl);
		if (AddrCntrl & (ACB_AUDIO_PREEMPH|ACB_FOUR_CHANNEL)) 
			return DIPIR_RETURN_ROMAPPDISC;
		if ((AddrCntrl & ACB_DIGITAL_TRACK) == 0)
			return DIPIR_RETURN_ROMAPPDISC;
        }

        if (i > 2)		/* No need to reread? */
		READTOC( 1 );	/* get this one back */
#endif /* NO_MIXED_MODE_DISC */
	return DIPIR_RETURN_THREE_BUCKS;
}

#if defined(APPDIGEST) && defined(ENCRYPT)
static DIPIR_RETURN
CheckAppDigest(DipirEnv *de) 
{
	RomTag	*rt;
	int status;

	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_SIGNATURE_BLOCK);
	if (rt == NULL) 
	{
		PUTS("NO DIGEST! ");
		return DIPIR_RETURN_TROJAN;
	}
	/* found the digest */
	digest_start_block = rt->rt_Offset + de->RomTagBlock;
	digest_size_bytes  = rt->rt_Size;
	MaxPain = rt->rt_TypeSpecific;

	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_APP);
	if (rt == NULL)
	{
		PUTS("No APP CODE! ");
                app_start_block = 256; /* after the 2nd rt table */
	} else 
	{
		/* Found application area start */
		app_start_block = (rt->rt_Offset + de->RomTagBlock) / 16;
	}

	/* Now RSA check the app digest */
	status = VerifyDiscDigest(de);
	PUTS("cddipir: VerifyDiscDigest ret="); PUTHEX(status);
	if (status == 0) 
		return DIPIR_RETURN_TROJAN;
	return DIPIR_RETURN_THREE_BUCKS;
}
#endif /* APPDIGEST && ENCRYPT */

DIPIR_RETURN 
cddipir(DipirEnv *de) 
{
	RomTag *	rt;
	RomTag *	sysrt;
	int		status;
	int32		discVersion;
	int32		romOsVersion;
	int32		currVersion;
	uint32		entryMctl;
	DIPIR_RETURN	ret;

	uint32		osstartblock;
	uint32		osbytes;
	uint32 *	osaddr;

	uint32		miscstartblock;
	uint32		miscbytes;
	bool		oldmisc;

#ifdef APPSPLASH
	uint32		splashstartblock;
	uint32		splashbytes;
#endif /* APPSPLASH */

#ifdef SIGN_LAUNCHME
	uint32		sign_launchme;
	uint32		sign_launchme_size;
#endif /* SIGN_LAUNCHME */

	dipr = de->DipirRoutines;
#ifndef COMBINE
	dvr = de->dvr;
#endif

	/* Load New OS/RSA/LAUNCH NEW OS */

#ifdef COMBINE
	PUTS("## Entering BUILTIN CDROM dipir program!! ");
#else
	PUTS("## Entering CDROM dipir program!! ");
#endif
#ifndef ALLOW_NONZERO_DEVICE
	/* We need to make sure we are not being called */
	/* for the wrong device! */
	if (de->device != BOOTDEVICEUNIT())
	{
		PUTS("3DO Discs allowed in internal drive only!");
		return DIPIR_RETURN_EXT_EJECT;
	}
#endif

	/* Turn on video */
	PUTS("Turning on video");
	entryMctl = ModMCTL(MCTL_OP_BITSET, VSCTXEN | CLUTXEN);

#ifdef NO_MULTISESSION_DISC
	if (MultiSession(de)) {
		PUTS("***MultiSession***");
		goto Trojan;
	}
#endif /* NO_MULTISESSION_DISC */

	if (GoldDisc(de)) {
		PUTS("***Gold Disc***");
#if defined(ENCRYPT) || !defined(APPSPLASH)
		goto Trojan;
#else
		/* Allow a Gold disc in the unencrypted version, but make
		 * the display "gold" to signal that we've detected it. */
		{
			extern void ClearDisplay(int16 color);
			ClearDisplay(GOLD_COLOR);
		}
#endif
	}

	if (!AuthPlantCheck(de)) {
		PUTS("***BAD PRESSING PLANT***");
		goto Trojan;
	}

	ret = CheckDiscMode(de);
	if (ret != DIPIR_RETURN_THREE_BUCKS)
		goto Exit;

	/* Get memory sizes and page size for roundup calculations. */
	FindMemSize(&dramSize, &vramSize);
	pageSize = dramSize / 64;

	/*
	 * Get version numbers of:
	 *  1. current running OS
	 *  2. OS on disc
	 *  3. OS in ROM
	 * and choose the newest one to run.
	 */
	currVersion = CurrentOsVersion();
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_OS);
	if (rt == NULL) 
		goto Trojan;
	discVersion = MakeInt16(rt->rt_Version, rt->rt_Revision);
	sysrt = FINDSYSROMTAG(RT_SUBSYS_ROM, ROM_KERNEL_CD);
	if (sysrt == NULL) {
		romOsVersion = 0;
	} else {
		romOsVersion = MakeInt16(sysrt->rt_Version, sysrt->rt_Revision);
	}
	PUTS("Curr OS="); PUTHEX(currVersion);
	PUTS("Disk OS="); PUTHEX(discVersion);
	PUTS("ROM  OS="); PUTHEX(romOsVersion);
	osstartblock = rt->rt_Offset + de->RomTagBlock;
	osbytes = rt->rt_Size;

	/* Find the misc code on the disc. */
	oldmisc = FALSE;
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_MISCCODE);
	if (rt == NULL) {
		/* not found, try old misc code romtag */
		PUTS("Old MISC CODE? ");
		oldmisc = TRUE;
		rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_OLD_MISCCODE);
	}
	if (rt == NULL) 
	{
		PUTS("No MISC CODE! ");
		goto Trojan;
	}
	/* Found misc_code */
	miscstartblock = rt->rt_Offset + de->RomTagBlock;
	miscbytes = rt->rt_Size;

#ifdef SIGN_LAUNCHME
        sign_launchme = 0;
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_BLOCKS_ALWAYS);
	if (rt != NULL) 
	{
		sign_launchme = rt->rt_Offset + de->RomTagBlock;
		sign_launchme_size = rt->rt_Size;
	}
#endif /* SIGN_LAUNCHME */

#ifdef APPSPLASH
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_APPSPLASH);
	if (rt != NULL)
	{
		splashstartblock = rt->rt_Offset + de->RomTagBlock;
		splashbytes = rt->rt_Size;
	} else
	{
		splashstartblock = 0;
		splashbytes = 0;
		PUTS("No app splash screen on disc");
	}
#endif /* APPSPLASH */

	bootData.bd_Version = 1;
	bootData.bd_DevicePerms = 0;
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_DEV_PERMS);
	if (rt != NULL)
		bootData.bd_DevicePerms = rt->rt_Reserved3[0];

#ifdef ENCRYPT
#ifdef APPDIGEST
	ret = CheckAppDigest(de);
	if (ret != DIPIR_RETURN_THREE_BUCKS)
		goto Exit;
#else
	if (splashstartblock == 0) 
	{
		/* This is an encrypted cddipir which doesn't check the digest.
		 * We REQUIRE a splash screen. */
		goto Trojan;
	}
#endif /* APPDIGEST */
#endif /* ENCRYPT */

	/* After this point, the RomTags in databuff2 may be gone
	 * (CheckAppDigest reuses databuff2). */

#ifdef SIGN_LAUNCHME
	/* might have to check launchme for a valid signature. */
	if (sign_launchme) 
	{
		if (RSACheckLaunchme(de,sign_launchme) == 0)
			goto Trojan;
	}
#endif /* SIGN_LAUNCHME */

#ifndef ROMBUILD
	/* Development version can't boot OS off disc. */
	discVersion = romOsVersion = 0;
#endif /* ROMBUILD */

/* #ifdef  DATA_DISC */
	/*
	 * OS was already launched when we entered dipir.
	 * The only way to get here with the LUNCH bit set is
	 * if we are dipiring a data disc.
	 */
	if (Current3DOFlags() & _3DO_LUNCH) 
	{
		PUTS("Data disc: verifying OS");
		status = VerifyAllOsComponents(de,
				osstartblock, osbytes,
				miscstartblock, miscbytes,
				splashstartblock, splashbytes);
		if (status == 0)
			goto Trojan;
		PUTS("Good DataDisc");
		ret = DIPIR_RETURN_DATADISC;
		goto Exit;
	}
/* #endif DATA_DISC */

	if (currVersion >= discVersion && currVersion >= romOsVersion)
	{
		/*
		 * Current OS is the newest.
		 * Just return and let the current OS keep running.
		 * (But verify the OS on disc anyway.)
		 */
		status = VerifyAllOsComponents(de,
				osstartblock, osbytes,
				miscstartblock, miscbytes,
				splashstartblock, splashbytes);
		if (status == 0)
			goto Trojan;
		PUTS("Good 3DO, old os");
		ret = DIPIR_RETURN_THREE_BUCKS;
		goto Exit;
	}

#ifdef APPSPLASH
	if (splashstartblock != 0)
	{
		status = LoadOsComponent(de, splashstartblock, splashbytes, 1);
		if (status == 0)
		{
			PUTS("Bad sig on splash screen");
#ifdef ENCRYPT
			/* Don't require splash screen to be signed in 
			 * unenc version. */
			goto Trojan;
#endif
		}
		if (DISPLAYIMAGE((void*)OS_LOAD_ADDR, APP_SPLASH_PATTERN) == 0)
		{
			PUTS("DisplayImage failed");
			goto Trojan;
		}
	}
#endif /* APPSPLASH */

	numComponents = 0;
	componentLoadAddr = DIPIR_ENDADDR();

	/*
	 * Load misc code. This always comes from the disc (for legal reasons).
	 */
	if (oldmisc) 
	{
#ifdef SUPPORT_OLD_MISCCODE
		/* Oh no!  See RelocateOldMiscCode for the gory details. */
		status = LoadOsComponent(de, osstartblock, osbytes, 0);
		if (status == 0)
			goto Trojan;
		RelocateOldMiscCode();
#else
		PUTS("Old misc code not supported!");
		goto Trojan;
#endif
	} else 
	{
		status = LoadOsComponent(de, miscstartblock, miscbytes, 0);
		if (status == 0)
			goto Trojan;
		RelocateOsComponent(FALSE);
	}

	/* Load kernel code.  This comes from either disc or ROM,
	 * whichever is newer. */
	if (discVersion > romOsVersion) {
		/* Disc OS is newer: load OS from disc. */
		PUTS("Load from DISC");
		status = LoadOsComponent(de, osstartblock, osbytes, 0);
		if (status == 0)
			goto Trojan;
		/* Give ROM a chance, else relocate the kernel ourselves */
		osaddr = ROMDOESTRANSFER(((uint32 *)(OS_LOAD_ADDR+8)));
		if (osaddr == 0) {
			componentLoadAddr = ROUNDUP(componentLoadAddr, 1024);
			RelocateOsComponent(TRUE);
			osaddr = (uint32*) Component[1].comp_Address; /* Sherry */
		}
		/* Now reboot to the newly loaded OS. */
		ClearGaps();
		*((uint32**)MAGIC_KERNELADR) = osaddr;
		PUTS("reboot to new OS:"); PUTHEX(osaddr);
		/* ModMCTL(MCTL_OP_SET, entryMctl); */
		bootData.bd_BootVolumeLabel = de->DiscLabel;
		REBOOT(osaddr, 0x101, (int)&bootData,
			(int)Component[0].comp_Address);
	} else {
		/* ROM OS is newer: load OS from ROM. */
		PUTS("Load from ROM");
#ifndef ROMBUILD
		PUTS("cddipir logic error\n");
		goto Trojan;
#else
		status = VerifyAllOsComponents(de,
				osstartblock, osbytes,
				oldmisc ? miscstartblock : 0, miscbytes,
				splashstartblock, splashbytes);
		if (status == 0)
			goto Trojan;

		/* Sherry */
		componentLoadAddr = ROUNDUP(componentLoadAddr, 1024);
		status = LoadAndRelocateRomOsComponent(
				RT_SUBSYS_ROM, ROM_KERNEL_CD, 0, FALSE, TRUE);
		if (status == 0)
			goto Trojan;
		osaddr = (uint32*) Component[1].comp_Address; /* Sherry */

		/* Operator */
		status = LoadAndRelocateRomOsComponent(
				RT_SUBSYS_ROM, ROM_OPERATOR, pageSize, TRUE, FALSE);
		if (status == 0)
			goto Trojan;

		/* File system */
		status = LoadAndRelocateRomOsComponent(
				RT_SUBSYS_ROM, ROM_FS, pageSize, TRUE, FALSE);
		if (status == 0)
			goto Trojan;
#endif /* ROMBUILD */
		/* Now reboot to the newly loaded OS. */
		ClearGaps();
		*((uint32**)MAGIC_KERNELADR) = osaddr;
		PUTS("reboot to new OS:"); PUTHEX(osaddr);
		/* ModMCTL(MCTL_OP_SET, entryMctl); */
		bootData.bd_BootVolumeLabel = de->DiscLabel;
		REBOOT(osaddr, 0x401, (int)&bootData, 
			(int)Component[0].comp_Address);
	}

	/* Couldn't reboot. */
Trojan:
	ret = DIPIR_RETURN_TROJAN;
Exit:
	/* Restore video */
	/* ModMCTL(MCTL_OP_SET, entryMctl); */
	return ret;
}


/*

Note on "shareable" disks.  Our elaborate scheme for "sharable" 3DO
disks can be implemented here, with no change to Dipir:

Disk Type	App Type	RC	Action
----------------+-----------------------+------------------------------------
Normal		Normal		--	Boot'NLoad
Shareable	Normal		--	Boot app's "Sorry, wrong disk" prog.
Normal		Shareable	--	Boot'NLoad new app
Shareable	Shareable	02	Return to APP, initiate dischange

3DO Pretender	XXXXXXXXX	-1	Eject 'Em

** A shareable disk inserted to a normal app must boot the new app.
CDRomDipir will need to somehow get the information that an app is
shareable.  Perhaps the app will make a system call that leaves 
a special signed something somewhere in dipir memory.  Ugh!

If the disk does not verify, return -1 (with prejudice). 
This function should never return 0.  You *must* check the 3DO intellectual
property.

*/


#ifdef SUPPORT_OLD_MISCCODE
/*
 * This is some really horrible code to make the Crash&Burn disc boot.
 * This disc has a romtag RSA_OLD_MISCCODE, but due to some human error,
 * the file it points to is NOT misc code, but junk.
 * So, to load misc code from the C&B disc, we find all the pieces of
 * misc code in the RSA_OS file instead, pull them out, construct a
 * buffer that looks like misc code is supposed to look, and use that.
 */

/*
 * Table of offsets of the various misc code pieces in the C&B RSA_OS file.
 */
struct functable {
	uint32 offset;
	uint32 size;
	uint8 index;
	uint8 reloc;
};
typedef void vfunc(void);

static void
RelocateOldMiscCode(void)
{
	uint32 miscaddr;
	uint32 osaddr;
	uint32 dst;
	uint32 *vtable;
	struct functable *p;
	/* The order of the entries in this table is important! */
	static struct functable functable[] = {
		{ 0x130c, 0x488,  0, 0 },	/* memcpy */
		{ 0x1794,  0xbc,  1, 0 },	/* memset */
		{  0x7c4,  0x1c,  6, 0 },	/* AddHead */
		{  0x7e0,  0x2c,  7, 0 },	/* RemHead */
		{  0x80c,  0x28,  5, 0 },	/* RemTail */
		{  0x834,  0x24,  2, 0 },	/* RemNode */
		{  0x858,  0x1c,  4, 0 },	/* AddTail */
		{  0x874,  0x54,  3, 0 },	/* InsertNodeFromTail */
		{  0x8c8,  0x68,  8, 0 },	/* InsertNodeFromHead */
		{ 0x1cd4,  0x48,  9, 1 },	/* InitList */
		{  0xf3c, 0x1a0, 10, 0 },	/* __rt_sdiv */
		{ 0x110c, 0x184, 11, 0 },	/* __rt_udiv */
		{ 0x1290,  0x48, 12, 0 },	/* __rt_sdiv10 */
		{ 0x12d8,  0x34, 13, 0 },	/* __rt_udiv10 */
		{ 0 }
	};

	/*
	 * Save the address of the misccode buffer.
	 * Allow 3K for misc code.
	 */
	miscaddr = Component[numComponents].comp_Address = componentLoadAddr;
	Component[numComponents].comp_Size = 3*1024;
	numComponents++;
	componentLoadAddr += 3*1024;
	/*
	 * The RSA_OS file is at OS_LOAD_ADDR.
	 * The first 8 bytes are a header. 
	 * The next 8 bytes are the loadaddr and size of the sherry.
	 * The sherry itself starts at offset 16.
	 */
	osaddr = OS_LOAD_ADDR + 16;

	/*
	 * The RSA_OS file is compressed, so we have to decompress it.
	 * But we can't just jump to the decompress entrypoint, because
	 * it will never return.
	 * So we patch it to look like this:
	 *	STMFD	SP!,{r0-r14}	; save all registers on the stack
	 *	STR	SP,xxx		; save the SP
	 *	BL	decompress	; (this is at osaddr)
	 *	LDR	SP,xxx		; restore the SP
	 *	LDMFD	SP!,{r0-r13,r15}; restore all registers from the stack
	 * xxx	DCD	?
	 * Note that we corrupt two words before osaddr, but that's ok
	 * (its the loadaddr+size header).  We also corrupt three words 
	 * after osaddr, but that's ok too (its part of the AIF header).
	 */
	*((uint32*)(osaddr-8)) = 0xE92D7FFF;
	*((uint32*)(osaddr-4)) = 0xE58FD008;
	*((uint32*)(osaddr+4)) = 0xE59FD000;
	*((uint32*)(osaddr+8)) = 0xE8BDBFFF;
	/* Now call it to get it to decompress itself. */
	(*((vfunc*)(osaddr-8)))();

	/*
	 * Ok, now start constructing the misc code buffer.
	 * It looks like this:
	 *	MOV	R0,vtable
	 *	MOV	PC,R8
	 * vtable
	 *	DCD	memcpy
	 *	DCD	memset
	 *	... etc ...
	 */
	vtable = (uint32*)miscaddr;
	*vtable++ = 0xE1A0000F;
	*vtable++ = 0xE1A0F008;
	/* Leave room for the vtable (14 entries) */
	dst = (uint32)(vtable+14);

	/*
	 * Copy each function into the misc code buffer,
	 * and put an entry into vtable to point to it.
	 * We must copy them in the proper order, because some of the
	 * functions have relative branches into other functions!
	 */
	for (p = functable;  p->offset != 0;  p++) {
		MEM_MOVE((POINTER)dst, (POINTER)(osaddr + p->offset), p->size);
		vtable[p->index] = dst;
		dst += p->size;
		if (p->reloc) {
			/*
			 * There is only one relocation in the misc code
			 * (in InitList) so make it a special case.
			 * We don't bother to do generic relocations.
			 */
			*((uint32*)dst) = dst+4;
			dst += 4;
			*((uint32*)dst) = 0x4C697374; /* "List" */
			dst += 4;
			*((uint32*)dst) = 0;
			dst += 4;
		}
	}
}
#endif /* SUPPORT_OLD_MISCCODE */
