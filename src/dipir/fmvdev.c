/* $Id: fmvdev.c,v 1.16 1994/08/02 20:13:47 markn Exp $ */

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "rom.h"
#include "dipir.h"
#include "fmvdev_rev.h"

#define	FMV_DRIVER 1

#ifdef FMV_DRIVER

/* fmvdipir.c : c code to handle dipir events */
/* This runs in its own little environment! and cannot depend */
/* on anything in the kernel */
/* stack has been setup, there are no parameters passed in */

static DipirEnv *de;
static DipirRoutines *dipr;

static int
ReadBlock(int b)	/* Read a byte from Woody/Clio external rom */
{
   char *dst = de->CurrentBuff;
   UncleRegs *ur = (UncleRegs *)UNCLEADDR;

   ur->unc_AddressReg = (uint32)b;
   *dst = (char)ur->unc_DataReg;
   de->statcnt = 2;
   de->statbuff[1] = 0;	/* no errors */
   return 0;	/* no errors */
}

static void
AsyncReadBlock(int b)
{
	(void) ReadBlock(b);
}

static int
WaitReadBlock(void)
{
	return WAITXBUS();
}

static int
ReadFmvInfo(void)
{
    DiscInfo *di = (DiscInfo *)de->statbuff;
    di->di_DiscId = 0;
    di->di_FirstTrackNumber = 1;
    di->di_LastTrackNumber = 1;
    de->statcnt = sizeof(DiscInfo)+1;
    di[1].di_cmd = 0;
    return 0;
}

static int
ReadSessionInfo(void)
{
    de->statbuff[1] = 0;
    de->statcnt = 2;
    return 0;
}

static int
ReadTOC(t)
int t;
{
    TOCInfo *ti = (TOCInfo *)de->statbuff;
    ti->toc_AddrCntrl = ACB_DIGITAL_TRACK;
    ti->toc_CDROMAddr_Frm = 0;
    ti->toc_CDROMAddr_Sec = 0;
    ti->toc_CDROMAddr_Min = 0;
    de->statcnt = sizeof(TOCInfo)+1;
    ti[1].toc_cmd = 0;
    return 0;
}

static void
EjectDisc(void)
{
}

static int32 CheckGoldDisc(void) {
    return 0;
}

static int32 GetMfgPlant(void) {
    return 0;
}

static uint32
GetDriverInfo()
{
	return( DIPIR_DEVICE_ID_FMVCARD | FMVDEV_VERREV );
}


static DeviceRoutines dvr =
{
	ReadBlock,
	ReadFmvInfo,
	ReadSessionInfo,
	ReadTOC,
	EjectDisc,
	AsyncReadBlock,
	GetDriverInfo,
	CheckGoldDisc,
	GetMfgPlant,
	DEVICE_ROUTINES_VERSION_0+1,
	WaitReadBlock,
};

int
FMV_InitDisc(struct DipirEnv *lde)
{
    de = lde;
    dipr = de->DipirRoutines;
    PUTS(" Enter InitFmv ");
    CHECKPOINT(0x4444);

    /* We should never get here if the Uncle device */
    /* does not have a rom in the right space */

#if 0
    /* New Uncle */
    dvr.ReadBlock = UncleReadBlock;
    dvr.AsyncReadBlock = (void (*)())UncleReadBlock;
#endif

    /* initialize device routines */
    de->dvr = &dvr;
    de->BlockSize = 1;
    return 1;	/* disc ready to be interrogated */
}

int
InitUncle(struct DipirEnv *lde)
{
	return FMV_InitDisc(lde);
}

#else /* FMV_DRIVER */

int InitUncle(struct DipirEnv *lde) { return 0; }
int FMV_InitDisc(struct DipirEnv *lde) { return 0; }

#endif /* FMV_DRIVER */
