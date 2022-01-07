/* $Id: fmvdipir.c,v 1.31 1994/09/15 17:34:00 markn Exp $ */

#define	MISC_CODE 1
#define	APPSPLASH 1

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "setjmp.h"
#include "rom.h"
#include "dipir.h"
#include "rom.h"

#ifndef RSA_APPSPLASH
#define RSA_APPSPLASH		0x14	/* App splash screen image */
#endif

/* fmvdipir.c : c code to handle fmv dipir events */
/* This is fmv rom resident and is loaded by the rom resident dipir code */
/* stack has been setup, the DipirEnv * is passed in. */

extern uint32 CurrentOsVersion(void);
extern void FindMemSize(uint32 *pDramSize, uint32 *pVramSize);
extern uint32 ModMCTL(uint32 op, uint32 newmctl);
extern int32 DefaultDisplayImage(void *image0, char *pattern);

DeviceRoutines *dvr;
DipirRoutines *dipr;
static uint32 dramSize;
static uint32 vramSize;

#define LOGICAL_BS	2048


#define LoadOsComponent(de,blk,siz,appkey) \
	ReadOsComponent(de,(char*)OS_LOAD_ADDR,blk,siz,TRUE,appkey)
#define VerifyOsComponent(de,blk,siz,appkey) \
	ReadOsComponent(de,(char*)0,blk,siz,FALSE,appkey)

static int
ReadOsComponent(DipirEnv *de, char *dst, uint32 startblock, uint32 bytes,
		bool doload, bool appkey)
{
	int status;
	uint32 os_blocks;
	char *NextBuff;

	PUTS("===FMV LOAD OS COMPONENT===");

	os_blocks = (bytes - SIG_LEN) / LOGICAL_BS;
	DoubleBuffer(de);

	if (READ(startblock, LOGICAL_BS))
	{
		PUTS("FMV read error");
		return 0;
	}
	startblock += LOGICAL_BS;
	os_blocks--;
	bytes -= LOGICAL_BS;

	DIGESTINIT();
	while (os_blocks)
	{
		SwitchBuffers(de);
		if (READ(startblock, LOGICAL_BS))
		{
			PUTS("FMV read error"); 
			return 0;
		}
		startblock += LOGICAL_BS;
		os_blocks--;
		bytes -= LOGICAL_BS;
		UPDATEDIGEST(NextBuff, LOGICAL_BS);
		if (doload)
		{
			MEM_MOVE(dst, NextBuff, LOGICAL_BS);
			dst += LOGICAL_BS;
		}
	}
	UPDATEDIGEST(de->CurrentBuff, LOGICAL_BS);
	if (doload)
	{
		MEM_MOVE(dst, de->CurrentBuff, LOGICAL_BS);
		dst += LOGICAL_BS;
	}

	/* now finish up */
	SingleBuffer(de, de->databuff1);
	if (READ(startblock, LOGICAL_BS))
	{
		PUTS("FMV read error"); 
		return 0;
	}
	startblock += LOGICAL_BS;
	os_blocks--;
	UPDATEDIGEST(de->databuff1, bytes-SIG_LEN);
	if (doload)
	{
		MEM_MOVE(dst, de->databuff1, bytes-SIG_LEN);
		dst += LOGICAL_BS;
	}

	if (bytes <= LOGICAL_BS)
	{
		MEM_MOVE(de->databuff2, de->databuff1+bytes-SIG_LEN, SIG_LEN);
	} else
	{
		/* construct signature here */
		MEM_MOVE(de->databuff2, de->databuff1+bytes-SIG_LEN,
			LOGICAL_BS-bytes+SIG_LEN);
		/* now get the last block */
		bytes -= LOGICAL_BS;
		if (READ(startblock, bytes))
		{
			PUTS("FMV read error"); 
			return 0;
		}
		MEM_MOVE(de->databuff2+SIG_LEN-bytes, de->databuff1, bytes);
	}

	FINALDIGEST();
	if (appkey)
		status = RSAFINALAPP((unsigned char *)de->databuff2, SIG_LEN);
	else
		status = RSAFINALTHDO((unsigned char *)de->databuff2, SIG_LEN);
	return status;
}

uint32 *
XferCodeAndDoNotReboot(uint32 *image)
{
	uint32 *loadaddr;
	uint32 *kerneladdr;
	uint32 size;

	kerneladdr = (uint32 *)(*image);
	while (loadaddr = (uint32 *)(*image++))
	{
		size = *image++;
		while (size) {
			*loadaddr++ = *image++;
			size -= sizeof (uint32);
		}
	}
	return kerneladdr;
}

DIPIR_RETURN
cddipir(DipirEnv *de)
{
	RomTag *rt;
	int32 status;
	uint32 fmvVersion;
	uint32 currVersion;
	uint32 entryMctl;
	DIPIR_RETURN ret;

	uint32 osstartblock;
	uint32 osbytes;
	uint32 *osaddr;
#ifdef MISC_CODE
	uint32 miscstartblock;
	uint32 miscbytes;
	uint32 *miscaddr;
#endif
#ifdef APPSPLASH
	uint32 splashstartblock;
	uint32 splashbytes;
#endif

	dvr = de->dvr;
	dipr = de->DipirRoutines;
	PUTS("!!Entering FMV dipir program!! ");

	PUTS("Turning on video");
	entryMctl = ModMCTL(MCTL_OP_BITSET, VSCTXEN | CLUTXEN);

	FindMemSize(&dramSize, &vramSize);

	/* Find OS on FMV */
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_OS);
	if (rt == NULL)
	{
		PUTS("No OS on FMV");
		goto Trojan;
	}
	fmvVersion = MakeInt16(rt->rt_Version, rt->rt_Revision);
	currVersion = CurrentOsVersion();
	PUTS("FMV  OS="); PUTHEX(fmvVersion);
	PUTS("Curr OS="); PUTHEX(currVersion);
	osstartblock = rt->rt_Offset + de->RomTagBlock;
	osbytes = rt->rt_Size;
	PUTS("osstartblock="); PUTHEX(osstartblock);
	PUTS("osbytes="); PUTHEX(osbytes);

#ifdef MISC_CODE
	/* Find misc code on FMV */
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_MISCCODE);
	if (rt == NULL)
	{
	     PUTS("No MISC_CODE on FMV");
	     goto Trojan;
	}
	/* Found misc_code */
	miscstartblock = rt->rt_Offset + de->RomTagBlock;
	miscbytes = rt->rt_Size;
	PUTS("miscstartblock="); PUTHEX(miscstartblock);
	PUTS("miscbytes="); PUTHEX(miscbytes);
#endif

#ifdef APPSPLASH
	/* Find splash screen on FMV */
        rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_APPSPLASH);
        if (rt == NULL)
        {
                PUTS("No splash screen on FMV");
		goto Trojan;
        }
	splashstartblock = rt->rt_Offset + de->RomTagBlock;
	splashbytes = rt->rt_Size;
#endif /* APPSPLASH */

        if (currVersion >= fmvVersion)
        {
                /*
                 * Current OS is the newest.
                 * Just return and let the current OS keep running.
                 * (But verify the OS on the FMV anyway.)
                 */
		if (VerifyOsComponent(de, osstartblock, osbytes, 0) == 0)
                        goto Trojan;
#ifdef MISC_CODE
		if (VerifyOsComponent(de, miscstartblock, miscbytes, 0) == 0)
                        goto Trojan;
#endif
#ifdef APPSPLASH
		if (VerifyOsComponent(de, splashstartblock, splashbytes, 1) == 0)
                        goto Trojan;
#endif
                PUTS("Good FMV, old os");
                ret = DIPIR_RETURN_THREE_BUCKS;
                goto Exit;
        }

#ifdef APPSPLASH
	/* Display the splash screen */
        if (splashstartblock != 0)
        {
                status = LoadOsComponent(de, splashstartblock, splashbytes, 1);
                if (status == 0)
                {
                        PUTS("Bad sig on splash screen");
                        goto Trojan;
                }
                if (DISPLAYIMAGE((void*)OS_LOAD_ADDR, HW_SPLASH_PATTERN) == 0)
                {
                        PUTS("DisplayImage failed");
                        goto Trojan;
                }
        }
#endif /* APPSPLASH */

	/* Load new OS and reboot to it. */
#ifdef MISC_CODE
	/* bring in misc_code first */
	PUTS("FMV Load misc");
	status = LoadOsComponent(de, miscstartblock, miscbytes, 0);
	if (status == 0)
		goto Trojan;
	miscaddr = XferCodeAndDoNotReboot((uint32 *)(OS_LOAD_ADDR+8));
#endif

	/* Now bring in real os code */
	PUTS("FMV Load OS");
	status = LoadOsComponent(de, osstartblock, osbytes, 0);
	if (status == 0)
		goto Trojan;

	/* Give ROM a chance, else XferCodeAndDoNotReboot ourselves */
	osaddr = ROMDOESTRANSFER((uint32 *)(OS_LOAD_ADDR+8));
	if (osaddr == 0)
		osaddr = XferCodeAndDoNotReboot((uint32 *)(OS_LOAD_ADDR+8));
	PUTS("About to call reboot ");
	*((uint32**)MAGIC_KERNELADR) = osaddr;
	/* ModMCTL(MCTL_OP_SET, entryMctl); */
	REBOOT(osaddr, 0x100, (int)de->DiscLabel, (int)miscaddr);

        /* Couldn't reboot. */
Trojan:
        ret = DIPIR_RETURN_TROJAN;
Exit:
        /* Restore video */
        /* ModMCTL(MCTL_OP_SET, entryMctl); */
	PUTS("FMV return "); PUTHEX(ret);
        return ret;
}
