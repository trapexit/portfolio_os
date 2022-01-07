/* $Id: depotdev.c,v 1.4 1994/09/23 21:20:34 markn Exp $ */

/*
 * Dipir device driver for DEPOT/VISA.
 */

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "rom.h"
#include "dipir.h"
#include "depot.h"
#include "depotdev_rev.h"

static DipirEnv *de;
static DipirRoutines *dipr;
static int ReadError;

static char cacheBuf[128];
static uint32 cacheBlock = (uint32)(~0);
static uint32 cacheLen = sizeof(cacheBuf);

#define	INRANGE(t,start,len)	((t) >= (start) && (t) < (start)+(len))

/*
 * Issue a standard memory transfer command.
 */
static void
IssueCmd(uint8 cmdtag, uint32 addr, uint16 len)
{
	uint8 cmd[7];

	cmd[0] = cmdtag;
	cmd[1] = (uint8)(addr >> 24);
	cmd[2] = (uint8)(addr >> 16);
	cmd[3] = (uint8)(addr >> 8);
	cmd[4] = (uint8)(addr);
	cmd[5] = (uint8)(len >> 8);
	cmd[6] = (uint8)(len);
	ASYNCSQUIRTOUTCMD(cmd, sizeof(cmd));
}

/*
 * Write data out to the XBus.
 * The standard routines in cdipir (WAITXBUS etc.) deal with
 * reading from XBus but not writing to it.
 */
static void
SquirtOutData(uint8 *src, uint32 len)
{
        ExpansionBus *xb = XBUS_ADDR;

	while (len-- > 0)
	{
		xb->xb_Data[0] = *src++;
	}
}

/*
 * Read a block (byte) of data from Depot space.
 */
static void
AsyncReadBlock(int b)
{
	uint32 block = (uint32) b;
	uint32 iblock;
	char *savebuf;
	int status;

	ReadError = 0;

CheckCache:
	/*
	 * First check the cached data.
	 * We maintain a cache because it's too slow to 
	 * issue a command for every byte.
	 */
	if (INRANGE((uint32)block, cacheBlock, cacheLen))
	{
		/* Assume de->BlockSize == 1 */
		*(de->CurrentBuff) = cacheBuf[block-cacheBlock];
		return;
	}

	/*
	 * See what address range we're in.
	 * Depot space is divided into several ranges.
	 */
	de->DataExpected = (int)cacheLen;
	if (INRANGE(block, DEPOT_ROM_XADDR, DEPOT_ROM_LEN))
	{
		/* Depot ROM */
		iblock = block - DEPOT_ROM_XADDR;
		IssueCmd(CMD_READ_TRANSFER, 
			iblock + DEPOT_ROM_IADDR, (uint16)de->DataExpected);
	} else if (INRANGE(block, VISA_ROM_XADDR, VISA_ROM_LEN))
	{
		/* Internal VISA ROM */
		iblock = block - VISA_ROM_XADDR;
		IssueCmd(CMD_DOWNLOAD_VISA, 
			iblock*2 + VISA_ROM_IADDR, (uint16)de->DataExpected);
	} else if (INRANGE(block, VISA_EXT_XADDR, VISA_EXT_LEN))
	{
		/* External VISA ROM */
		iblock = block - VISA_EXT_XADDR;
		IssueCmd(CMD_DOWNLOAD_VISA, 
			iblock + VISA_EXT_IADDR, (uint16)de->DataExpected);
	} else if (INRANGE(block, DEPOT_REG_XADDR, DEPOT_REG_LEN))
	{
		/* Depot register */
		iblock = block - DEPOT_REG_XADDR;
		IssueCmd(CMD_READ_TRANSFER, 
			iblock + DEPOT_REG_IADDR, (uint16)de->DataExpected);
	} else 
	{
		PUTS("Invalid Depot address: "); PUTHEX(block);
		ReadError = 1;
		return;
	}

	/*
	 * Read the data into the cache instead of the caller's buffer.
	 * Then go back and find the requested byte in the cache.
	 */
	savebuf = de->CurrentBuff;
	de->CurrentBuff = cacheBuf;
	status = WAITXBUS();
	de->CurrentBuff = savebuf;
	if (status < 0)
	{
		ReadError = 1;
		return;
	}
	cacheBlock = (uint32)block;
	goto CheckCache;
}

/*
 * Wait for AsyncReadBlock to finish.
 * Actually, AsyncReadBlock is synchronous.
 * This just allows us to return errors.
 */
static int 
WaitReadBlock(void)
{
	if (ReadError) 
	{
		ReadError = 0;
		return -1;
	}
	return ReadError;
}

/*
 * Read a block.
 */
static int
ReadBlock(int b)
{
	AsyncReadBlock(b);
	return WaitReadBlock();
}

/* 
 * Read a Depot register.
 */
static uint8
ReadRegister(uint32 regaddr)
{
	uint8 reg;

	de->DataExpected = sizeof(reg);
	de->CurrentBuff = &reg;
	IssueCmd(CMD_READ_TRANSFER, regaddr, sizeof(reg));
	WAITXBUS();
	return reg;
}

/* 
 * Write a Depot register.
 */
static void
WriteRegister(uint32 regaddr, uint8 reg)
{
	de->DataExpected = 0;
	IssueCmd(CMD_WRITE_TRANSFER, regaddr, sizeof(reg));
	SquirtOutData(&reg, sizeof(reg));
	WAITXBUS();
}

/*
 * Read "disc" info.
 */
static int
ReadInfo(void)
{
	DiscInfo *di = (DiscInfo *)(de->statbuff);

	di->di_DiscId = 0;
	di->di_FirstTrackNumber = 1;
	di->di_LastTrackNumber = 1;
	de->statcnt = sizeof(DiscInfo) + 1; /* 1 for status byte */
	*((uint8*)(di+1)) = 0; /* Fake status byte = no error */
	return 0;
}

/*
 * Read "session" info.
 */
static int
ReadSessionInfo(void)
{
	de->statbuff[1] = 0;
	de->statcnt = 2;
	return 0;
}

/*
 * Read table of contents.
 */
static int
ReadTOC(int track)
{
	TOCInfo *ti = (TOCInfo *)(de->statbuff);
	ti->toc_AddrCntrl = ACB_DIGITAL_TRACK;
	/* Say data starts at the beginning of Depot ROM. */
	ti->toc_CDROMAddr_Frm = DEPOT_ROM_XADDR;
	ti->toc_CDROMAddr_Sec = 0;
	ti->toc_CDROMAddr_Min = 0;
	de->statcnt = sizeof(TOCInfo) + 1; /* 1 for status byte */
	*((uint8*)(ti+1)) = 0; /* Fake status byte = no error */
	return 0;
}

/*
 * Eject the card.
 * This doesn't physically eject the card, but prevents further access to it.
 */
static void
EjectDisc(void)
{
	uint8 reg;

	PUTS("=== DEPOT EJECT! ===");
	reg = ReadRegister(DEPOT_REG_FUNC_CTL);
	reg |= DEPOT_CARD_EJECT;
	WriteRegister(DEPOT_REG_FUNC_CTL, reg);
}

static int32 
CheckGoldDisc(void) 
{
	return 0;
}

static int32 
GetMfgPlant(void) 
{
	return 0;
}

static uint32
GetDriverInfo()
{
	return DIPIR_DEVICE_ID_DEPOT | DEPOTDEV_VERREV;
}


static DeviceRoutines dvr =
{
	ReadBlock,
	ReadInfo,
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

/*
 * Initialize the device and the driver.
 */
int
DEPOT_InitDisc(struct DipirEnv *lde)
{
	de = lde;
	dipr = de->DipirRoutines;
	de->dvr = &dvr;
	de->BlockSize = 1;
	de->ManuIdNum = MANU_3DO;
	de->ManuDevNum = MANU_3DO_DEPOT;
	de->ManuRevNum = 1;

	WriteRegister(DEPOT_REG_FUNC_CTL,
		DEPOT_CARD_POWER | DEPOT_CARD_RESET | 
		DEPOT_WRITE_PROTECT | DEPOT_WRITE_ROM);
	return 1;
}
