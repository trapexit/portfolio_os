/* $Id: devdipir.c,v 1.8 1994/07/26 17:51:43 markn Exp $ */

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "setjmp.h"
#include "rom.h"
#include "dipir.h"
/*#include "aif.h"*/
#include "rom.h"

/* cromdipir.c : c code to handle dipir events */
/* This is cdrom resident and is loaded by the rom resident dipir code */
/* stack has been setup, the DipirEnv * is passed in. */

extern int CurrentOsVersion(void);
extern void EnableIrq(void);

DeviceRoutines *dvr;
DipirRoutines *dipr;

#define OS_SIZE	(128*1024)

#define PHYS_BS	2048

int
LoadNewOsComponent(de, startblock, bytes)
DipirEnv *de;
uint32 startblock;
uint32 bytes;
{
    int i;
    int status;
    uint32 os_blocks;
    char *p[2];
    char *dst;

    dst = (char *)OS_LOAD_ADDR;
    PUTS("===LOAD OS COMPONENT===");

    os_blocks = (bytes-SIG_LEN)/PHYS_BS;
    p[0] = de->databuff1;
    p[1] = de->databuff2;

    de->CurrentBuff = de->databuff1;
    /*ASYNC*/READ(startblock,PHYS_BS);
    startblock += PHYS_BS;
    os_blocks--;
    bytes -= PHYS_BS;
    i = 1;

    DIGESTINIT();
    while (os_blocks)
    {
	/*WAITREAD();*/

	de->CurrentBuff = p[i];

	/*ASYNC*/READ(startblock, PHYS_BS);
    	startblock += PHYS_BS;
	os_blocks--;
	bytes -= PHYS_BS;

	i = 1-i;
        UPDATEDIGEST(p[i],PHYS_BS);
	MEM_MOVE((POINTER)dst,(POINTER)p[i],PHYS_BS);   dst += PHYS_BS;
    }
    /*WAITREAD();*/
    i = 1-i;
    UPDATEDIGEST(p[i],PHYS_BS);
    MEM_MOVE((POINTER)dst,(POINTER)p[i],PHYS_BS);   dst += PHYS_BS;

    /* now finish up */
    de->CurrentBuff = p[0];
    READ(startblock, PHYS_BS);
    startblock += PHYS_BS;
    os_blocks--;
    UPDATEDIGEST(p[0],bytes-SIG_LEN);
    MEM_MOVE((POINTER)dst,(POINTER)p[0],bytes-SIG_LEN);   dst += PHYS_BS;
    if (bytes <= PHYS_BS)
    {
	MEM_MOVE((POINTER)p[1], (POINTER)p[0]+bytes-SIG_LEN, SIG_LEN);
    }
    else
    {
	/* construct signature here */
	MEM_MOVE((POINTER)p[1],(POINTER)p[0]+bytes-SIG_LEN,PHYS_BS-bytes+SIG_LEN);
	/* now get the last block */
	bytes -= PHYS_BS;
    	READ(startblock, bytes);
	MEM_MOVE((POINTER)p[1]+SIG_LEN-bytes,(POINTER)p[0],bytes);
    }

    FINALDIGEST();
    status = RSAFINALTHDO((unsigned char *)p[1], SIG_LEN);
    return status;
}

/* This needs to know about page sizes! */
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

int
LoadNewOs(de, startblock, bytes, miscstartblock, miscbytes)
DipirEnv *de;
uint32 startblock;
uint32 bytes;
uint32 miscstartblock;
uint32 miscbytes;
{
    int status;
    uint32 *kerneladdr;
    uint32 *miscaddr;

    /* bring in misc_code first */
    PUTS("LoadNewOS ");
    status = LoadNewOsComponent(de, miscstartblock, miscbytes);
    if (status == 0) return 0;
    miscaddr = XferCodeAndDoNotReboot((uint32 *)(OS_LOAD_ADDR+8));

    PUTS("Load real OS ");

    /* Now bring in real os code */
    status = LoadNewOsComponent(de, startblock, bytes);
    if (status == 0) return 0;

    if (status)
    {	/* Give ROM a chance, else XferCodeAndDoNotReboot ourselves */
	kerneladdr = ROMDOESTRANSFER(((uint32 *)(OS_LOAD_ADDR+8)));
	if( !kerneladdr )
		kerneladdr = XferCodeAndDoNotReboot((uint32 *)(OS_LOAD_ADDR+8));
	PUTS("About to call reboot ");
	/*while (1);*/
	REBOOT(kerneladdr, 0x100, (int)de->DiscLabel, (int)miscaddr);
    }
    return status;
}

int
VerifyNewOs(de, startblock, bytes)
DipirEnv *de;
uint32 startblock;
uint32 bytes;
{	/* Verify OS is good */
    int i;
    int status;
    uint32 os_blocks;
    char *p[2];

    PUTS("===VERIFY OS===");
    os_blocks = (bytes-SIG_LEN)/PHYS_BS;
    de->CurrentBuff = de->databuff1;
    p[0] = de->databuff1;
    p[1] = de->databuff2;

    /*ASYNC*/READ(startblock,PHYS_BS);
    startblock += PHYS_BS;
    os_blocks--;
    bytes -= PHYS_BS;
    i = 1;

    DIGESTINIT();
    while (os_blocks)
    {
	/*WAITREAD();*/

	de->CurrentBuff = p[i];

	/*ASYNC*/READ(startblock, PHYS_BS);
    	startblock += PHYS_BS;
	os_blocks--;
	bytes -= PHYS_BS;

	i = 1-i;
        UPDATEDIGEST(p[i],PHYS_BS);
    }
    /*WAITREAD();*/
    i = 1-i;
    UPDATEDIGEST(p[i],PHYS_BS);

    /* now finish up */
    de->CurrentBuff = p[0];
    READ(startblock, PHYS_BS);
    startblock += PHYS_BS;
    os_blocks--;
    UPDATEDIGEST(p[0],bytes-SIG_LEN);
    if (bytes <= PHYS_BS)
    {
	MEM_MOVE((POINTER)p[1], (POINTER)p[0]+bytes-SIG_LEN, SIG_LEN);
    }
    else
    {
	/* construct signature here */
	MEM_MOVE((POINTER)p[1],(POINTER)p[0]+bytes-SIG_LEN,PHYS_BS-bytes+SIG_LEN);
	/* now get the last block */
	bytes -= PHYS_BS;
    	READ(startblock, bytes);
	MEM_MOVE((POINTER)p[1]+SIG_LEN-bytes,(POINTER)p[0],bytes);
    }

    FINALDIGEST();
    status = RSAFINALTHDO((unsigned char *)p[1], SIG_LEN);
    return status;
}

DIPIR_RETURN
cddipir(de)
DipirEnv *de;
{
	RomTag	*rt;
	int	status;
	int	Version;
	uint32	rtstartblock;
	uint32	rtbytes;

	uint32	miscstartblock;
	uint32	miscbytes;

	/* Load New OS/RSA/LAUNCH NEW OS */
	dvr = de->dvr;
	dipr = de->DipirRoutines;
	PUTS("!!Entering DEV dipir program!! ");
        return 0;

	/* We need to make sure we are not being called */
	/* for the wrong device! */

	rt = FindRT((RomTag *)de->databuff2,RSANODE,RSA_OS);
	if (!rt) return -1;

	/* Found the os */
	Version = MakeInt16(rt->rt_Version, rt->rt_Revision);
	PUTS("Disk OS="); PUTHEX(Version);
	PUTS("ROM OS=");  PUTHEX(CurrentOsVersion());
	rtstartblock	= rt->rt_Offset + de->RomTagBlock;
	rtbytes		= rt->rt_Size;

	rt = FindRT((RomTag *)de->databuff2,RSANODE,RSA_MISCCODE);
	if (!rt)
	{
	     PUTS("No MISC CODE!");
	     return -1;
	}
	/* Found misc_code */
	miscstartblock = rt->rt_Offset + de->RomTagBlock;
	miscbytes = rt->rt_Size;

	PUTS("miscstartblock="); PUTHEX(miscstartblock);
	PUTS("miscbytes="); PUTHEX(miscbytes);
	/* Everything from databuff2 must be saved 'cause it will be nuked! */
	rt = NULL;

	if (Version > CurrentOsVersion())
	{
		LoadNewOs(de, rtstartblock, rtbytes, miscstartblock, miscbytes);
		return DIPIR_RETURN_TROJAN; /* Load either reboots or fails */
	}

	status = VerifyNewOs(de, rtstartblock, rtbytes);
	if (status == 0)	return DIPIR_RETURN_TROJAN;
	
	status = VerifyNewOs(de, miscstartblock, miscbytes);
	if (status == 0)	return DIPIR_RETURN_TROJAN;
	return 1;
}

/*	!!! Note: Needs updating with #define return codes !!!!!! -Bryce */
