/*	$Id: cdipir.c,v 1.128.1.4 1995/02/14 22:20:13 bungee Exp $
**
**	Disk insertion code
**
**	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
*/

#define DOWNLOAD_DRIVERS 1
#define DRIVERPACK 1
//#define NO_RSA_DRIVER_CHECK 1
//#define MEMORY_PUTS 1
#define SERIAL_SSIO 1
#define SERIAL_ZSIO 1
#define APPSPLASH 1

#ifndef BOOT_DEVICE_UNIT_NUMBER
#define BOOT_DEVICE_UNIT_NUMBER 0
#endif

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "setjmp.h"
#include "rom.h"
#include "aif.h"
#include "dipir.h"
#include "dipir_rev.h"

#define	AIFSize(aif)	((aif)->aif_ImageROsize + (aif)->aif_ImageRWsize + (aif)->aif_ZeroInitSize)


#ifdef SERIAL_SSIO
#define	SSIO_DATA	*((volatile uint32 *)(0x03180040))
#define	SSIO_CSR	*((volatile uint32 *)(0x03180044))
#endif

#ifdef SERIAL_ZSIO
#include "scc8530.h"
#define	ZSIO_SCC_BASE	0x03180000
#define	ZSIO_PORT	((scc8530 *)ZSIO_SCC_BASE)->ctlA
#endif


#ifdef DOWNLOAD_DRIVERS
#define MAX_DRIVER_SIZE (16*1024)
typedef struct DipirExpDev { 
        uint8 ded_Unit; /* device id */
        uint8 ded_Flags; /* misc flags */
	uint32	ded_LogicalSizeOfDriver;       /* alloc size of driver image */
	uint32	ded_PhysicalSizeOfDriver;      /* phys size of driver image  */
	void	*ded_Driver;    	       /* ptr to start of driver */
} DipirExpDev;

uint32 DriverBuffer[MAX_DRIVER_SIZE/sizeof(uint32)];
uint32 DriverBufferSize = sizeof(DriverBuffer);

extern int32 DownloadDriver(struct DipirEnv *lde);
#endif /* DOWNLOAD_DRIVERS */

#ifdef DRIVERPACK

/*
 * Header of a pack file:
 * First uint32 is number of entries, followed by
 * an array of DriverDesc structures, one for each entry.
 */
struct DriverDesc
{
	uint16 dd_ManuId;
	uint16 dd_ManuDev;
	uint32 dd_Offset;
	uint32 dd_Size;
};

static DIPIR_RETURN SwapInDriver(struct DipirEnv *lde);
static int FindPackEntry(uint32 ManuId, uint32 ManuDev, struct DriverDesc *dd);

#else /* DRIVERPACK */

extern DriverInitFunction MEICD_InitDisc;
#ifdef MEICD563
extern DriverInitFunction MEICD563_InitDisc;
#endif
extern DriverInitFunction LCCD_InitDisc;
extern DriverInitFunction FMV_InitDisc;

struct DriverSelect {
	DriverInitFunction *	dsel_Init;
	uint16			dsel_Manu;
	uint16			dsel_ManuDev;
} driverSelect[] = {
	{ MEICD_InitDisc,	MANU_MEI,	MANU_MEI_CD },
#ifdef MEICD563
	{ MEICD563_InitDisc,	MANU_MEI,	MANU_MEI_CD563 },
#endif
	{ LCCD_InitDisc,	MANU_LC,	MANU_LC_CD },
	{ 0 }
};

#endif /* DRIVERPACK */

static void hardboot(void);
extern void  ReadNBytes(uint32 *,uint32 *,uint32);
extern void callaif2(uint32, uint32, unsigned char *);
extern void InitECC(void);
extern int32 SectorECC(uint8 *buf);
extern DIPIR_RETURN cddipir(DipirEnv *de);
extern uint32 ModMCTL(uint32 op, uint32 newmctl);
extern int32 DefaultDisplayImage(void *image0, char *pattern);


#define XBUS_READIDCMD 		0x83
#define XBUS_READDRIVERCMD 	0x8E
#define XBUS_STATUSERR 		0x08

/* Some Dipir Prototypes */

extern void EnableIrq(void); /* used in non ROMBUILD only */

extern int RSAInit(void);
extern DIPIR_RETURN CallAIF(char *code, struct DipirEnv *lde);

/* cdipir.c : c code to handle dipir events */
/* This runs in its own little environment! and cannot depend */
/* on anything in the kernel */
/* stack has been setup, there are no parameters passed in */


#define WRITEPOLL(val)          POLL = (vuint32)(val)
#define POLL                    xb->xb_Poll[0]
#define SELECTXBUS(u)   	xb->xb_Sel[0] = u;
#define READPOLL(poll,unit)     xb->xb_Sel[0] = unit; poll = (uint8)POLL

#define DEVHAS_DRIVER   0x500   /* Driver downloaded from Device rom */

#define TESTREAD(s)     (s & XBUS_POLL_READ_VALID)
#define TESTWRITE(s)    (s & XBUS_POLL_WRIT_VALID)
#define TESTSTAT(s)     (s & XBUS_POLL_STAT_VALID)

#define BAD_READ	-1
#define	BAD_DATA	-2
#define GOOD_DATA	0


extern jmp_buf *jmp_back;
extern void myfucking_longjmp(jmp_buf *,int);
extern uint32 ignoreabort;
extern uint32 abortignored;
extern uint32 discOsVersion;

static DiscLabel myDiscLabel;

DeviceRoutines *dvr;	/* Drive specific/revectorable functions  */

extern int ret_code;
uint8		thdoflags;	/* Copy of sherry's _3DO_Flags */
#ifdef DRIVERPACK
void *		PackFile;	/* RomFile descriptor for driver pack file */
#endif
uint32		entryMctl;	/* MCTL register on entry to dipir */

volatile uint32 longstatbuff[8];	/* line it up on 32 bit boundary */

#define	MAX_BLOCK_SIZE	(2*1024)	/* max block size */
#define MIN_STRAPSIZE	(1024)		/* cdromdipir no smaller */
/*
 * Use 16K for both DEBUG and production now.
 * Remember that old machines have an 8K BOOTSTRAPSIZE, so any
 * cdromdipir put on a disc must have a footprint less than 8K
 * The 16K here allows the cddipir from ROM (in the pak file)
 * to be larger, since it must handle all disc versions.
 */
#define BOOTSTRAPSIZE	(16*1024)	/* cdromdipir + SIG_LEN no larger */

/*
 * One big buffer, divided into three parts: databuff1, databuff2
 * and CDRSACode.  
 */
char bigbuffer[MAX_BLOCK_SIZE+MAX_BLOCK_SIZE+BOOTSTRAPSIZE];
#define	DataBuffer1	(&bigbuffer[0])
#define	DataBuffer2	(&bigbuffer[MAX_BLOCK_SIZE])
#define	DevDipirBuffer	(&bigbuffer[MAX_BLOCK_SIZE+MAX_BLOCK_SIZE])

extern unsigned int DIGEST[4];

DipirEnv lde =
{
	0,	/* VolumeLabelBlock */
	-1,	/* FirstBlock */
	0,	/* RomTagBlock */
	DataBuffer1,
	DataBuffer2,
	DevDipirBuffer,
	(char *)longstatbuff,
	DataBuffer1,	/* CurrentBuff */
	0,	/* DataExpected */
	&myDiscLabel,
	0,	/* DipirRoutines, filled in later */
	0,	/* DeviceRoutines */
	0,0,0,0,0, /* Manu IdNum,DevNum,RevNum,Flags,RomTagTableSize */
	0,	/* statcnt */
	0,	/* device */
	DIPIR_VERSION,
	DIPIR_REVISION,
	0,	/* BlockSize */
	(unsigned char *)DIGEST,
	0, 0, 0, 0,
#ifdef DOWNLOAD_DRIVERS
        (unsigned char *)(&DriverBuffer[(3*1024)/sizeof(long)]),
#else
	0,
#endif
};


void
do_checkpoint(int i)	/* Checkpoint function */
{
	ret_code = i;
}

#ifdef DEBUG
/* Debugging functions */

#ifdef MEMORY_PUTS
#define	CHARBUFFSIZE 0x800
char _charbuff[CHARBUFFSIZE];
int charcount = 0;
char *charbuff = _charbuff;
#endif /* MEMORY_PUTS */

#ifdef SERIAL_SSIO
static bool ssioSerialCardGood = FALSE;
#endif /* SERIAL_SSIO */
#ifdef SERIAL_ZSIO
static bool zsioSerialCardGood = FALSE;
#endif /* SERIAL_ZSIO */

#ifdef SERIAL_ZSIO
static void
zsioWriteReg(uint32 reg, uint32 data)
{
	uint32 junk;

	ignoreabort = 1;
again:
	abortignored = 0;
	SCC_HOLDOFF();
	junk = ZSIO_PORT;
	if (abortignored) goto again;
	SCC_HOLDOFF();
	ZSIO_PORT = reg;
	if (abortignored) goto again;
	SCC_HOLDOFF();
	ZSIO_PORT = data;
	if (abortignored) goto again;
	ignoreabort = 0;
}

static uint32
zsioReadReg(uint32 reg)
{
	uint32 data;
	uint32 junk;

	ignoreabort = 1;
again:
	abortignored = 0;
	SCC_HOLDOFF();
	junk = ZSIO_PORT;
	if (abortignored) goto again;
	SCC_HOLDOFF();
	ZSIO_PORT = reg;
	if (abortignored) goto again;
	SCC_HOLDOFF();
	data = ZSIO_PORT;
	if (abortignored) goto again;
	ignoreabort = 0;
	return data;
}

static void
zsioSetBaud(uint32 br)
{
	zsioWriteReg(SCC_BAUDLO, SCC_BAUD_CONST_LO(br));
	zsioWriteReg(SCC_BAUDHI, SCC_BAUD_CONST_HI(br));
}

static void
zsioInitSCC(void)
{
	zsioWriteReg(SCC_MICR, SCC_MICR_RESET);
	zsioWriteReg(SCC_MISCPARM, SCC_MISCPARM_CLK1 | SCC_MISCPARM_1STOP);
	zsioWriteReg(SCC_RXPARM, SCC_RXPARM_8BIT);
	zsioWriteReg(SCC_TXPARM, SCC_TXPARM_8BIT);
	zsioWriteReg(SCC_MICR, 0);
	zsioWriteReg(SCC_MISC, 0);
	zsioWriteReg(SCC_CLOCK, SCC_CLOCK_TRXCO_XTAL | SCC_CLOCK_TXCI_BRGEN | 
			SCC_CLOCK_RXCI_BRGEN);
	zsioSetBaud(9600);
	zsioWriteReg(SCC_MISCCTL, 0);
	zsioWriteReg(SCC_MISCCTL, SCC_MISCCTL_BRGEN);
	zsioWriteReg(SCC_RXPARM, SCC_RXPARM_RXEN | SCC_RXPARM_8BIT);
	zsioWriteReg(SCC_TXPARM, SCC_TXPARM_RTS | SCC_TXPARM_TXEN | 
			SCC_TXPARM_8BIT | SCC_TXPARM_DTR);
}
#endif /* SERIAL_ZSIO */

#ifdef SERIAL_SSIO

static uint32
ssioGetCSR(void)
{
	uint32 val;

	ignoreabort = 1;
again:
	abortignored = 0;
	val = SSIO_CSR;
	if (abortignored) goto again;
	ignoreabort = 0;
	return val;
}

static void
ssioPutCSR(uint32 val)
{
	ignoreabort = 1;
again:
	abortignored = 0;
	SSIO_CSR = val;
	if (abortignored) goto again;
	ignoreabort = 0;
}

#if 0
static uint32
ssioGetData(void)
{
	uint32 val;

	ignoreabort = 1;
again:
	abortignored = 0;
	val = SSIO_DATA;
	if (abortignored) goto again;
	ignoreabort = 0;
	return val;
}
#endif

static void
ssioPutData(uint32 val)
{
	ignoreabort = 1;
again:
	abortignored = 0;
	SSIO_DATA = val;
	if (abortignored) goto again;
	ignoreabort = 0;
}

#endif /* SERIAL_SSIO */

static void
PutCharInit(void)
{
#ifdef MEMORY_PUTS
	{
		uint32 i;

		for (i = 0; i < CHARBUFFSIZE; i++)	
			charbuff[i] = ' ';
		charcount = 0;
	}
#endif /* MEMORY_PUTS */
#ifdef SERIAL_SSIO
	{
		ssioPutCSR(0);
		if (ssioGetCSR() == 0x21)
			ssioSerialCardGood = TRUE;
	}
#endif /* SERIAL_SSIO */
#ifdef SERIAL_ZSIO
	{
		uint32                  csr;

		zsioWriteReg(SCC_MICR, SCC_MICR_RESET);
		csr = zsioReadReg(SCC_CSR);
		if ((csr & SCC_CSR_ATRESET_MASK) == SCC_CSR_ATRESET_VAL) 
		{
			zsioInitSCC();
			csr = zsioReadReg(SCC_CSR);
			if ((csr & SCC_CSR_ATRESET_MASK) == SCC_CSR_ATRESET_VAL)
				zsioSerialCardGood = TRUE;
		}
	}
#endif /* SERIAL_ZSIO */
}

void 
realputchar(char c)
{
#ifdef MEMORY_PUTS
	/* Put the character in a memory buffer */
	{
		charbuff[charcount++] = c;
		if (charcount >= CHARBUFFSIZE) charcount = 0;
		charbuff[charcount] = '>';	/* Current end marker */
	}
#endif /* MEMORY_PUTS */
	/* Put the character at a magic address, for Logic Analyser debugging */
	{
		volatile int32 *pointer = (int32 *)(MADAM); 
		if (c != '\n')
			*pointer = c;
	}

	/* SERIAL stuff below here */
	if (c == '\n') {
		/* Assume a Mac is receiving the data;
		 * Mac doesn't like newlines */
		c = '\r';
	}
#ifdef SERIAL_SSIO
	if (ssioSerialCardGood)
	{
		while ((ssioGetCSR() & 0x20) == 0) 
			continue;
		ssioPutData(c);
	}
#endif /* SERIAL_SSIO */
#ifdef SERIAL_ZSIO
	if (zsioSerialCardGood)
	{
		int32 watchdog;
		char retryChar;
		static char lastChar = 0;
	 again:
		watchdog = 10000;
		retryChar = 0;
		while ((zsioReadReg(SCC_CSR) & SCC_CSR_TXRDY) == 0) {
			if (--watchdog <= 0) {
				zsioInitSCC();
				retryChar = lastChar;
			}
		}
		if (retryChar) {
			zsioWriteReg(SCC_TXDATA, retryChar);
			goto again;
		}
		zsioWriteReg(SCC_TXDATA, c);
		lastChar = c;
	}
#endif /* SERIAL_ZSIO */
}

void 
realputs(char *s)
{
	realputchar('`');
	while (*s)
		realputchar(*s++);
}

void 
realputhex(int32 iv) 
{
	uint32 v = (uint32)iv;
	int i;
	bool emit = FALSE;
	uint32 hexval;

	realputchar('#');
	for (i = 0;  i < 8;  i++, v <<= 4)
	{
		if (!emit)
		{
			if ((v & 0xf0000000) == 0) {
				/* Don't display leading zeros */
				continue;
			}
			emit = TRUE;
		}
		hexval = v >> 28;
		if (hexval < 10)
			realputchar((char)(hexval+'0'));
		else
			realputchar((char)(hexval+'a'-10));
	}
	if (!emit)
		realputchar('0');
	realputchar('\n');
}
#else /* DEBUG */
/* No-debug code goes here */
void realputchar(char c) {}
void realputs(char *s) {}
void realputhex(int32 iv) {}
#endif /* DEBUG */

int
ReadTimer(void)
{
	/* get current milliseconds since last timer reset */
	int i;
	HardTimer *ht;
	ht = (HardTimer *)(Timer0);
	ht += 15;	/* 16th timer */

	i = 0xffff;
	i -= (int)ht->ht_cnt;
	return i;
}

void
ResetTimer(void)
{
	HardTimer *ht;
	ht = (HardTimer *)(Timer0);
	ht += 15;	/* 16th timer */
	ht->ht_cnt = 0xffff;
}

void
WaitMills(int num)
{
	ResetTimer();
	while (ReadTimer() < num) ;
}

uint8
ReadPoll(uint8 unit)
{
    	uint8 val;
	ExpansionBus *xb = XBUS_ADDR;
	READPOLL(val,unit);
	return val;
}

uint8
ReadStatusByte(void)
{
	volatile uint32 *src;
	ExpansionBus *xb = XBUS_ADDR;
	src = &xb->xb_CmdStat[0];
	return (uint8)*src;
}

void
ReadData(void)
{
        /* drain read data fifo */
        uint32 cnt = (uint32)lde.DataExpected;
        uint8 *p = (uint8 *)lde.CurrentBuff;
        ExpansionBus *xb = XBUS_ADDR;

	/* If cnt is multiple of 8, we can use the fast ReadNBytes() */
        if ((cnt % 8) == 0)
	{
		/* assume long word aligned buffer */
        	uint32 *p32 = (uint32 *)p;  
		ReadNBytes((uint32 *)&xb->xb_Data[0], p32, cnt);
	} else 
	{
		while (cnt--) 
			*p++ = (uint8)xb->xb_Data[0];
	}
        lde.DataExpected = 0;
}

int
ReadStatusFifo(void)
{
	ExpansionBus *xb = XBUS_ADDR;
	uint8 poll;
	int timeout = 0;
	int maintimeout = 0;

	while (1)
	{
		poll = ReadPoll(lde.device);
		if (TESTSTAT(poll))
			break;
		if (TESTREAD(poll))
			ReadData();
		if (++timeout > 1000000)
		{
			puts("timeout waiting for ID response!\n");
			if (++maintimeout > 9)
			{
				/* Device is hosed */
				puts("Giving up, reseting system\n");
				hardboot();
			}
			timeout = 0;
		}
	}
	if (TESTREAD(poll))	
		ReadData();
	while (TESTSTAT(poll))
	{
		lde.statbuff[lde.statcnt++] = ReadStatusByte();
		SELECTXBUS(lde.device);
		poll = (uint8)POLL;
	}
	return 0;
}

static void
AsyncSquirtOutCmd(uint8 *src, int len)
{
	ExpansionBus *xb = XBUS_ADDR;
	uint8 tag = src[0];

	if (len == 7)
	{	/* most common case */
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src;
	} else {
		while (len--)	
			xb->xb_CmdStat[0] = *src++;
	}
	lde.statcnt = 0;
	if (lde.DeviceFlags & DEV_NO_TAG_BYTE)
	{
		/* Device does not return command tag byte as 
		 * first byte of status.  Fake it. */
		lde.statbuff[0] = tag;
		lde.statcnt = 1;
	}
}

static void
SquirtOutCmd(uint8 *src, int len)
{
	AsyncSquirtOutCmd(src,len);
	ReadStatusFifo();
}

int
WaitXBus(void)
{
	int errstat;
	ReadStatusFifo();
	errstat = lde.statbuff[lde.statcnt-1];
	if (errstat & 0x10) return -1;
	return 0;
}

#if 0
static int
MultiSession(void)
{
	READSESSIONINFO();
	if (lde.statbuff[1] & 0x80) return 1;
	return 0;
}
#endif

extern void RealBoot(uint32 *,int,int,int);

static void
reboot(uint32 *romboot, int r6, int r7,int r8)
{
	Clio *clio = (Clio *)CLIO;
	unsigned int intenbits[2];

	puts("Enter Reboot ##\n"); puthex(r6); puthex(r7); puthex(r8);

	DEINITDISC();

	/* Disable video again, because destination expects video to be off. */
	if (romboot == (uint32*)MAGIC_RESET)
		ModMCTL(MCTL_OP_SET, entryMctl);

	/* reset the hardware first */
	intenbits[0] = (unsigned int)clio->ClioInt0.ClrIntEnBits;
	intenbits[1] = (unsigned int)clio->ClioInt1.ClrIntEnBits;
	clio->ClioInt0.ClrIntEnBits = -1;
	clio->ClioInt1.ClrIntEnBits = -1;

	/* If development system, then just return */

	/* If real system, then MAGIC_RESET jumps back to debugger and */
	/* reboot the machine as if a por occured */
	/* Otherwise launch new OS at address provided */
	RealBoot(romboot,r6,r7,r8);

	/* must be development */
	clio->ClioInt0.SetIntEnBits = intenbits[0];
	clio->ClioInt1.SetIntEnBits = intenbits[1];
}

#define	WATCHDOG_NEEDLES	9	/* Count time in units of "Needles" */
#define	TEJU_MAGICOUNT		10

static void
hardboot(void)
{
	Clio *clio = (Clio *)CLIO;
	Madam *madam = (Madam *)MADAM;
	vuint32 *adbio = ADBIO;
	int i;

	puts("Enter HardReboot ##\n"); 

	/* Mute the audio kludge ADBIO 1 */
	*adbio &= (vuint32)(~(ADBIO_AUDIO_MUTE | ADBIO_AUDIO_MUTE_EN));

	/*
	 * Force a hard boot by triggering the watchdog timer.
	 * First set up the watchdog timer normally.
	 * Then, accelerate the countdown so the reset happens right away.
	 *
	 * First, enable the watchdog reset by writing the standard
	 * initial count to the WatchDog register.
	 */
	clio->WatchDog = 0x0b;
	/* Hammer the expansion bus also */
	clio->clio_ExpansionBus.xb_SetExpCtl = XB_EXPRESET;

	/* Wait 9 NEEDLES.  This is at least 100ms, no matter what. */
	for( i=0; i<WATCHDOG_NEEDLES; i++ ) {
		while ((*VCNT&VCNT_MASK) <= TEJU_MAGICOUNT)
			continue;
		while ((*VCNT&VCNT_MASK) >= TEJU_MAGICOUNT)
			continue;
	}

	/*
	 * Now accelerate the countdown.
	 * This is done differently for Anvil vs. Opera.
	 */
	if (IS_ANVIL())
	{
		clio->WatchDog = 0;
		madam->AnvilFeature |= MADAM_AnvilWatchDog;
	} else /* opera */
	{
		*adbio |= (ADBIO_WATCHDOG | ADBIO_WATCHDOG_EN);
	}

	while (1) {
#ifdef DEBUG
		realputchar('!');	/* Wait for Armageddon */
		puthex(clio->CStatBits);
		WaitMills(5);
#endif
	}
}

void
ResetDevAndExit(void)
{
	puts("ResetDevAndExit: doing longjmp");
	myfucking_longjmp(jmp_back,1);
}

_3DOBinHeader *
Get3DOBinHeader(void)
{
	return (_3DOBinHeader *)(*(int *)(MAGIC_KERNELADR)+ sizeof(AIFHeader));
}

uint8
GetCurrent3DOFlags(void)   /* Return sherry's _3DOBInHeader flags */
{
	_3DOBinHeader *bh = Get3DOBinHeader();
	return (bh->_3DO_Flags);
}


extern void DipirInitDigest(void);
extern void DipirUpdateDigest(char *input, int len);
extern void DipirFinalDigest(void);
extern int B_CreateKeyObject(B_KEY_OBJ *key);
extern int B_SetKeyInfo(B_KEY_OBJ, B_INFO_TYPE, POINTER);
extern bool RSACheck(void *buff, int32 buffsize);
extern int32 GenRSACheck(unsigned char *input,int inputLen, unsigned char *signature,int signatureLen);


int Read(int b,int cnt)
{
	/* Read multiples of blocks in */
	char *savedst;

	savedst = (char *)lde.CurrentBuff;
	while (cnt>0)
	{
		if (READBLOCK(b) < 0)	
			return -1;
		cnt -= lde.BlockSize;
		b++;
		lde.CurrentBuff += lde.BlockSize;
	}
	lde.CurrentBuff = savedst;
	return 0;
}

/*	Backhook from cdromdipir just in case the ROM (the machine)
**	ever has to decide how to place a sherry image in memory.
*/
uint32 *
ROMDoesTransfer(uint32 *image)
{
	return 0;	/* Return 1 if the ROM ever did the Xfer */
}

#ifdef ROMBUILD
/*
 * Copy data from ROM.
 * Handle possible data aborts (if video is on).
 */
static void
CopyFromRom(POINTER adst, POINTER asrc, uint32 len)
{
	uint8 *src = (uint8*)asrc;
	uint8 *dst = (uint8*)adst;

	ignoreabort = 1;
	while (len > 0) 
	{
		abortignored = 0;
		*dst = *src;
		if (abortignored) 
			continue;
		src++;
		dst++;
		len--;
	}
	ignoreabort = 0;
}
#endif /* ROMBUILD */

#ifdef ROMBUILD
/*
 * Return a pointer to a RomTag in the system ROM.
 * Note: returns a pointer to a static structure which may be overwritten
 * by another call to FindSysRomTag.
 */
static RomTag *
SysRomTagPointer(int subsys, int type)
{
	RomTag *romrt;
	RomTag copyrt;

	if (sysRomTags == NULL)
		return NULL;
	for (romrt = sysRomTags; ; romrt++) 
	{
		CopyFromRom((POINTER)&copyrt, (POINTER)romrt, sizeof(RomTag));
		if (copyrt.rt_SubSysType == 0)
			break;
		if (copyrt.rt_SubSysType == subsys && copyrt.rt_Type == type)
			return romrt;
	}
	return NULL;
}
#endif /* ROMBUILD */

void *
FindSysRomTag(int subsys, int type)
{
#ifndef ROMBUILD
	return NULL;
#else
	RomTag *romrt;
	static RomTag staticrt;

	romrt = SysRomTagPointer(subsys, type);
	CopyFromRom((POINTER)&staticrt, (POINTER)romrt, sizeof(RomTag));
	return (void*) &staticrt;
#endif
}

/*
 * Functions to deal with ROM files.
 */

void *
OpenRomFile(uint32 subsysType, uint32 type)
{
#ifdef ROMBUILD
	return SysRomTagPointer((int)subsysType, (int)type);
#else /* ROMBUILD */
	/*
	 * Running in the debug environment, there is no ROM.
	 * But there is a PACK file image in memory at boot time, 
	 * so long as we haven't already launched the system 
	 * (which overwrites the pack file image).
	 */
	if (subsysType != RT_SUBSYS_ROM || type != ROM_DIPIR_DRIVERS)
	{
		puts("Attempt to open ROM file in debug!\n");
		puthex(subsysType); puthex(type);
		return NULL;
	}
	if (thdoflags & _3DO_LUNCH)
	{
		puts("system already launched; pack file is lost\n");
		return NULL;
	}
	/* Note: in debug version, we return not a RomTag*, 
	 * but a direct pointer to the file image. */
	return (void*)PACK_FILE_ADDR;
#endif /* ROMBUILD */
}

uint32
ReadRomFile(void *file, uint32 offset, uint32 length, uint8 *buffer)
{
	if (file == NULL) 
		return 0;
#ifdef ROMBUILD
	{
		RomTag romtag;
		
		CopyFromRom((POINTER)&romtag, (POINTER)file, sizeof(RomTag));

		if (length > romtag.rt_Size - offset)
		{
			length = romtag.rt_Size - offset;
		}
		CopyFromRom((POINTER)buffer, 
			(POINTER)((uint8*)sysRomTags + romtag.rt_Offset + offset),
			length);
	}
#else /* ROMBUILD */
	T_memmove((POINTER)buffer, 
		(POINTER)((uint8*)file + offset), 
		(unsigned int)length);
#endif /* ROMBUILD */
	return length;
}

void
CloseRomFile(void *file)
{
	return;
}

#if 0
#define	SYSINFO_BUFFER		0x11000
#define	MAX_SYSINFO_SIZE	1024
#define	MIN_SYSINFO_SIZE	160
static int
LoadSysInfo(void)
{
	uint32 *vec;
	uint32 n;
#ifdef ROMBUILD
	void *fd;

	fd = OpenRomFile(RT_SUBSYS_ROM, ROM_SYSINFO);
	if (fd == NULL)
		return -1;
	if (ReadRomFile(fd, 0, MAX_SYSINFO_SIZE, (uint8*)SYSINFO_BUFFER) < MIN_SYSINFO_SIZE)
		return -1;
	CloseRomFile(fd);
	callaif2(0,0,(unsigned char *)SYSINFO_BUFFER);
#endif /* ROMBUILD */
	/* pull out vectors, etc. */
	vec = (uint32*)(((uint8*)SYSINFO_BUFFER) + sizeof(struct AIFHeader) +
			sizeof(struct _3DOBinHeader));
	*((uint32*)0x20C) = vec[1];
	*((uint32*)0x210) = vec[2];
	return 0;
}
#endif //0

static uint32
DipirEndAddr(void)
{
	extern uint32 DipirEnd;
	return DipirEnd;
}

uint32
GetDiscOsVersion(void)
{
	return discOsVersion;
}

static int
SetXBusSpeed(uint32 speedRate)
{
	uint32 speedCode;
	uint32 bits2;
	uint32 bits4;
	ExpansionBus *xb = XBUS_ADDR;

#define XBUS_MIN_PERIOD  120
#define XBUS_MAX_PERIOD  800
	static const uint8 xbus_speed_codes[] = {
	   XBUSTYPE(0,0,0), XBUSTYPE(0,0,0), XBUSTYPE(0,0,0), XBUSTYPE(0,0,0),
	   XBUSTYPE(0,1,0), XBUSTYPE(1,1,0), XBUSTYPE(1,1,1), XBUSTYPE(1,2,1),
	   XBUSTYPE(2,2,1), XBUSTYPE(2,2,2), XBUSTYPE(2,3,2), XBUSTYPE(3,3,2),
	   XBUSTYPE(3,3,3), XBUSTYPE(3,4,3), XBUSTYPE(4,4,3), XBUSTYPE(4,5,3),
	   XBUSTYPE(5,5,3), XBUSTYPE(5,6,3), XBUSTYPE(6,6,3), XBUSTYPE(6,7,3),
	   XBUSTYPE(7,7,3)
	};

        if (speedRate == 0) {
		/* Use default */
		bits2 = XTYPE2(0,1,0);
		bits4 = XTYPE4(1,2,1);
        } else if (speedRate >= XBUS_MIN_PERIOD &&
		   speedRate <= XBUS_MAX_PERIOD) {
		/* Get bits from table */
		speedCode = xbus_speed_codes[(speedRate + 39) / 40];
		bits2 = speedCode << XB_TYPE2Shift;
		bits4 = speedCode << XB_TYPE4Shift;
        } else {
		return -1;
        }
	xb->xb_ExpType = XTYPE1(0,1,0) | bits2 | XTYPE3(1,2,1) | bits4;
        return 0;
}

int32
CheckRT(RomTag *rt)
{
	int32 rttlen;

	rttlen = 0;
	while (rt->rt_SubSysType)
	{
		rt++;
		rttlen += sizeof(RomTag);
		if (rttlen >= MAX_ROMTAG_BLOCK_SIZE - SIG_LEN - sizeof(RomTag))
			return -1;
	}
	rttlen += sizeof(RomTag);	/* Count terminator */
	return rttlen;
}

/*
 * Old version of display image (1 parameter), for compatibility with FMV card.
 */
int32 
OldDisplayImage(void *image0)
{
	return DefaultDisplayImage(image0, HW_SPLASH_PATTERN);
}

uint32
BootDeviceUnit(void)
{
	return (BOOT_DEVICE_UNIT_NUMBER);
}

static struct DipirRoutines dr =
{
	(void (*)(char))realputchar,
	(void (*)(char *))realputs,
	(void (*)(int))realputhex,
	ResetTimer,
	ReadTimer,
	do_checkpoint,	/* CheckPoint */
	ResetDevAndExit,
	SquirtOutCmd,
	reboot,
	DipirInitDigest,
	DipirUpdateDigest,
	DipirFinalDigest,
	RSAFinalAPP,		/* 0=bad  [Use old "RsaFinal" location */
				/*  to support old disks (both of them :-)] */
	RSAFinalWithKey,	/* 0=bad */
	0,
	0,
	T_memmove,
	Read,
	WaitXBus,
	AsyncSquirtOutCmd,
	RSAFinalTHDO,		/* 0=bad */
	ROMDoesTransfer,
	hardboot,
	0,
	RSACheck,
	GenRSACheck,
	SectorECC,
	SetXBusSpeed,
	DIPIR_ROUTINES_VERSION_0+3,
	OpenRomFile,
	ReadRomFile,
	CloseRomFile,
	DipirEndAddr,
	FindSysRomTag,
	OldDisplayImage,
	DefaultDisplayImage,
	BootDeviceUnit,
};

/*
 *	Calling environment ->
 *		databuff1 - VolLabel
 *		databuff2 - RTT
 *		CDRSACode - CDROM Dipir code
 */
DIPIR_RETURN
CallCDROMDipir(RomTag *rt, int32 rttlen)
{
	int ok;
	DIPIR_RETURN ret;
	uint32 DipirSize;
	int32 discVersion;
	int32 romVersion;

	puts("CallCDROMDipir");
	if (rt->rt_Size > BOOTSTRAPSIZE || rt->rt_Size < MIN_STRAPSIZE)
		return DIPIR_RETURN_TROJAN;

	/*
	 * Set discOsVersion now.  Easier to do here than in cdromdipir.
	 */
	{
		RomTag *osrt;
		osrt = FindRT((RomTag *)lde.databuff2, RSANODE, RSA_OS);
		if (osrt != NULL) 
		{
#ifdef DISC_ROMTAG_HAS_CORRECT_OS_VERSION
			discOsVersion = MakeInt16(osrt->rt_Version, osrt->rt_Revision);
#else
			_3DOBinHeader *bh;

			lde.CurrentBuff = lde.CDRSACode;
			if (Read((int)(osrt->rt_Offset + lde.RomTagBlock), 
				(4*sizeof(uint32))+
				 sizeof(struct AIFHeader)+sizeof(_3DOBinHeader)))
				return DIPIR_RETURN_TROJAN;
			bh = (_3DOBinHeader*)
			     (((struct AIFHeader *)(((uint32*)lde.CurrentBuff)+4)) +1);
			discOsVersion = MakeInt16(bh->_3DO_Item.n_Version, 
						bh->_3DO_Item.n_Revision);
			puts("discOsVersion="); puthex(discOsVersion);
		}
#endif
	}

	/* Round up cdromdipir size to the next block size */
	DipirSize = 0;
	while (DipirSize < rt->rt_Size)
		DipirSize += lde.BlockSize;

	/* Read in the CDROMDIPIR */
	lde.CurrentBuff = lde.CDRSACode;
        if (Read((int)(lde.RomTagBlock+rt->rt_Offset), (int)DipirSize))
		return DIPIR_RETURN_TROJAN;

	/* We're going to be busy with RSA computations for a while;
	 * give the driver a chance to do something. */
	BEGIN_LONG_OP();


	/* Check CDROMDIPIR with the 3DO key. */
	DipirInitDigest();
	DipirUpdateDigest((char *)lde.CDRSACode, (int)(rt->rt_Size-SIG_LEN));
	DipirFinalDigest();
	ok = RSAFinalTHDO(
		(unsigned char *)lde.CDRSACode+rt->rt_Size-SIG_LEN, SIG_LEN);
	puts("RsaFinal 3DO:"); puthex(ok);
#ifndef	LEAKY_DIPIR
	if( !ok )	
	{
		END_LONG_OP();
		return DIPIR_RETURN_TROJAN;
	}
#endif

	/*
	 * Now cross-sign-check the VolumeLabel, RTT, and verified
	 * CDROMDipir with the APP key.
	 */
	DipirInitDigest();
	DipirUpdateDigest((char *)lde.DiscLabel, sizeof(*lde.DiscLabel) );
	DipirUpdateDigest((char *)lde.databuff2, (int)rttlen );
	DipirUpdateDigest((char *)lde.CDRSACode, (int)rt->rt_Size );
	DipirFinalDigest();
	ok = RSAFinalAPP((unsigned char *)lde.databuff2+rttlen, SIG_LEN);
	puts("RsaFinal Cross-APP:"); puthex(ok);
#ifndef	LEAKY_DIPIR
	if (!ok)
	{
		END_LONG_OP();
		return DIPIR_RETURN_TROJAN; /* With prejudice */
	}
#endif

	/* Now decrypt it */
	puts("CheezoDecrypt-"); puthex(*(uint32 *)lde.CDRSACode);
	DecryptBlock(lde.CDRSACode, rt->rt_Size);
	puthex(*(uint32 *)lde.CDRSACode); puts("-end\n");

	/* RSA check ONE MORE TIME!  (pre post cheezo cross) */
	DipirInitDigest();
	DipirUpdateDigest((char *)lde.CDRSACode, 
		(int)(rt->rt_Size-SIG_LEN-SIG_LEN));
	DipirFinalDigest();
	ok = RSAFinalTHDO(
	  (unsigned char *)lde.CDRSACode+rt->rt_Size-SIG_LEN-SIG_LEN, SIG_LEN);
	puts("RsaFinal Post-Cheeze:"); puthex(ok);

	END_LONG_OP();

#ifndef	LEAKY_DIPIR
	if (!ok)
		return DIPIR_RETURN_TROJAN; /* With prejudice */
#endif

	/*
	 *	For greater security code may be inserted here to
	 *	decide if we want the disc based code called for
	 *	external drives.	
	 */

	/* See whether disc or ROM has a newer cd-dipir, and call it */
#ifdef ROMBUILD
	{
		RomTag *romrt;

		romrt = (RomTag*) FindSysRomTag(RT_SUBSYS_ROM, ROM_DIPIR);
		if (romrt == NULL)
			return DIPIR_RETURN_TROJAN;
		romVersion = MakeInt16(romrt->rt_Version, romrt->rt_Revision);
	}
#else
	/* For the development version, force it to use built-in cd-dipir
	 * rather than the cd-dipir on the disc. */
	romVersion = 0xFFFF; 
#endif
	discVersion = MakeInt16(rt->rt_Version, rt->rt_Revision);
	puts("disc dipir "); puthex(discVersion);
	puts("ROM  dipir "); puthex(romVersion);

    	if (discVersion >= romVersion)
	{
		puts("calling disc dipir at"); puthex(lde.CDRSACode);
		ret = CallAIF(lde.CDRSACode, &lde);	
	} else 
	{
#ifdef DRIVERPACK
		struct DriverDesc dd;

		if (FindPackEntry(MANU_3DO, MANU_3DO_CDDIPIR, &dd))
		{
			puts("cannot find cddipir in pack file");
			return DIPIR_RETURN_TROJAN;
		}
		if (dd.dd_Size > BOOTSTRAPSIZE || dd.dd_Size < MIN_STRAPSIZE)
		{
			puts("cddipir code too big");
			return DIPIR_RETURN_TROJAN;
		}
		/* Read in the dipir */
		if (ReadRomFile(PackFile, dd.dd_Offset, dd.dd_Size, 
				(uint8*)lde.CDRSACode) != dd.dd_Size)
		{
			puts("cannot read cddipir from pack file");
			return DIPIR_RETURN_TROJAN;
		}
		if (AIFSize((AIFHeader*)lde.CDRSACode) > BOOTSTRAPSIZE)
		{
			puts("cddipir code+bss too big");
			return DIPIR_RETURN_TROJAN;
		}
		puts("calling builtin dipir (swapped in)");
		ret = CallAIF(lde.CDRSACode, &lde);	
#else
		puts("calling builtin dipir (linked in)");
		ret = cddipir(&lde);	
#endif
	}
	return ret;
}

#ifndef NULL_DIPIR

static int32
VerifyLabel(int32 b)
{
	int i;
	DiscLabel *dl = (DiscLabel *)lde.databuff1;

	if (Read((int)b, sizeof(DiscLabel))) {
		puts("VerifyLabel: BAD READ"); 
		return BAD_READ;
	}
	puts("lde.databuff1="); puthex(lde.databuff1);
	if (dl->dl_RecordType != 1) {
		puts("VerifyLabel: record type != 1: ");
		puthex(dl->dl_RecordType);
		return BAD_DATA;
	}

	/* Check sync bytes (alternating pattern to check for bit shifts) */
	for (i=4; i >= 0; i--) {
		if( dl->dl_VolumeSyncBytes[i] != VOLUME_SYNC_BYTE ) {
			puts("VerifyLabel: bad sync byte:");
			puthex(dl->dl_VolumeSyncBytes[i]);
			return BAD_DATA;
		}
	}
	return b;
}
#endif /* NULL_DIPIR */

#ifndef NULL_DIPIR

static int32
FindVolumeLabelBlock(void)
{
	int32 block;

	/*
	 * Check for VolumeLabels at...    (150 = two seconds 0:2:0)
	 *		150 +225 150 +32786 fail
	 * *Expects* a RTT after each VolumeLabel !!!
	 */
	block = VerifyLabel(lde.FirstBlock);
	if (block >= 0)
		return block;
	if (block == BAD_DATA)
		return BAD_DATA;

	block = VerifyLabel(lde.FirstBlock + DISC_LABEL_OFFSET);
	puts("LabelBlock=");puthex(block);
	if (block >= 0)
		return block;
	if (block == BAD_DATA)	
		return BAD_DATA;

	block = VerifyLabel(lde.FirstBlock);
	puts("LabelBlock=");puthex(lde.VolumeLabelBlock);
	if (block >= 0)
		return block;
	if (block == BAD_DATA)	
		return BAD_DATA;

	block = VerifyLabel(lde.FirstBlock + DISC_LABEL_OFFSET +
						DISC_LABEL_AVATAR_DELTA);
	return block;
}
#endif /* NULL_DIPIR */

#ifndef NULL_DIPIR
/*
 *	 2 = Good 3DO data_disk
 *	 1 = Good 3DO
 *	 0 = Not 3DO
 *	-1 = Fake 3DO Ick!  Ouch!  Get it outta here!
 *
 *	Uses, potentially, all the databuffers
 */
static DIPIR_RETURN
cd_is_3do(void)
{
	DiscInfo *di = (DiscInfo *)lde.statbuff;
	volatile TOCInfo *ti;
	RomTag *rt;
	int32 rttlen;		/* Length of RTT in bytes */
	DIPIR_RETURN status;
	int VolLabSize;

	puts("enter cd_is_3do\n");
	lde.CurrentBuff = lde.databuff1;

	/* Read disc info. */
	if (READDISCINFO() < 0)
		return DIPIR_RETURN_ROMAPPDISC;
	if (di->di_DiscId != 0)
	{
		puts("di_DiscId != 0: "); puthex(di->di_DiscId);
		return DIPIR_RETURN_ROMAPPDISC;
	}

	/* CD-DA or CD-ROM */
	puts("FirstTrack=");puthex(di->di_FirstTrackNumber);
	puts("LastTrack=");puthex(di->di_LastTrackNumber);
	if (di->di_FirstTrackNumber != 1)
		return DIPIR_RETURN_ROMAPPDISC;
	if (di->di_LastTrackNumber < 1)
		return DIPIR_RETURN_ROMAPPDISC;
    
	/* Check for MultiSession disc used to be here;
	 * now it is in the CD (device) dipir */

	/*
	 * Exit if the first track is CDDA.  (Note:  current 3DO disks
	 * have 16 bit audio in CDROM data format, and never have CDDA tracks).
	 */
	ti = (TOCInfo *)lde.statbuff;
	puts("ReadTOC(1)\n");
	if (READTOC(1) < 0) 
		return DIPIR_RETURN_ROMAPPDISC;
	{
		uint8 AddrCntrl;
		AddrCntrl = ti->toc_AddrCntrl;
		if ((AddrCntrl & ACB_AUDIO_PREEMPH) ||
		    (AddrCntrl & ACB_FOUR_CHANNEL) ||
		    (AddrCntrl & ACB_DIGITAL_TRACK) == 0)
			return DIPIR_RETURN_ROMAPPDISC;
	}

	/*
	 * Have to read in key 3do tracks
	 * Past this point we return with prejudice (assume bad-guy 3do disk)
	 */

	/* This calculation is device dependent! */
	lde.FirstBlock = (int32)
		(ti->toc_CDROMAddr_Frm + 
		 ((int32)ti->toc_CDROMAddr_Sec * FRAMEperSEC) +
		 ((int32)ti->toc_CDROMAddr_Min * FRAMEperSEC * SECperMIN));
	puts("FirstBlock="); puthex(lde.FirstBlock);

	lde.VolumeLabelBlock = FindVolumeLabelBlock();
	if (lde.VolumeLabelBlock < 0)
		return DIPIR_RETURN_TROJAN;

	/* Found what could be a valid block... let's play RSA games! */
	lde.RomTagBlock = lde.VolumeLabelBlock;
	VolLabSize = 0;	/* round up to next block size */
	while (VolLabSize < sizeof(*lde.DiscLabel) )
	{
		VolLabSize += lde.BlockSize;
		lde.RomTagBlock++;	/* find the next logical block */
	}
	/* Copy databuf1 to static disk label structure */
	T_memmove((POINTER)lde.DiscLabel, (POINTER)lde.databuff1, 
		sizeof(*lde.DiscLabel));

	/*
	 *  We may have been asked to come this far, yet the new disc
	 *  is not shareable.  If so, hardboot.  FMV drops through.
	 */
	if (!(lde.DiscLabel->dl_VolumeFlags & VOLUME_FLAGS_DATADISC)) {
		if (thdoflags & _3DO_LUNCH) {
			puts("NoData&Lunch Hardboot\n");
			hardboot();
		}
	}

	/* ROM Tag Block assumed to immediatly follow the volume label */
	puts("VolBlock="); puthex(lde.VolumeLabelBlock);
	puts("RTTBlock="); puthex(lde.RomTagBlock);
	lde.CurrentBuff = lde.databuff2;
	if (Read((int)lde.RomTagBlock,(int)MAX_ROMTAG_BLOCK_SIZE))
		return DIPIR_RETURN_TROJAN;
	/*
	 * Under the new scheme we'll cross-RSA the whole mess later.
	 * for now we use the data without really trusting it.
	 */

	/*
	 * count length of RTT table, for later use
	 * also acts as a sanity check
	 */
	rttlen = CheckRT((RomTag *)lde.CurrentBuff);
	if (rttlen < 0)
		return DIPIR_RETURN_TROJAN;
	puts("TagTablLen="); puthex(rttlen);
	/*
	 * Must have at least two entries, yet still leave room in the block
	 * for signature.
	 */
	if (rttlen < (sizeof(RomTag)*3))
		return DIPIR_RETURN_TROJAN;

#ifdef PAL_REJECT_OLD_DISCS
	/* THIS CODE CHECK THE OS VERSION IN THE TAG, AND REJECTS THE DISK IF
	   IT IS INITIAL REVISION, WHICH WE ASSUME DOESN'T UNDERSTAND PAL
	*/

	rt = FindRT((RomTag *)lde.CurrentBuff, RSANODE, RSA_OS); 
	if (rt && rt->rt_Version == 0)
		return DIPIR_RETURN_TROJAN;
#endif

	/* Now have valid volume label and valid RSACode */
	/* Search romtag table for CD-dipir to call */
	rt = FindRT((RomTag *)lde.CurrentBuff, RSANODE, RSA_NEWKNEWNEWGNUBOOT);
	if (rt != NULL)
		status = CallCDROMDipir(rt, rttlen);
	else
		status = DIPIR_RETURN_TROJAN;
	lde.CurrentBuff = lde.databuff1;
	puts("cd_is_3do returns:"); puthex(status);
	return status;
}
#endif /* NULL_DIPIR */

#ifndef NULL_DIPIR

static uint8
ParseReadIdStatus(void)
{
	uint8 *s = (uint8 *)lde.statbuff;

	/*
	 * Sanity checking on returned status.   Bogus status
	 * Packet is returned when running InsertDisc - you
	 * crash if disk inserted during InsertDisc.  This
	 * is a kludge for now to detect & reset. 
	 */
	puts(" Ide.statcnt="); puthex(lde.statcnt);
	puts(" statbuff[0]="); puthex((uint32)s[0]);
	if (s[0] != XBUS_READIDCMD || lde.statcnt != 12) 
	{
#ifdef MEICD563
		if (B_memcmp((char*)s, "CR-563", 6) == 0)
		{
			/*
			 * CR-563 drive returns a weird ASCII string
			 * instead of a real ReadId response packet.
			 * Fake up something that looks like a real packet.
			 */
			puts("found CR-563");
			s[0] = XBUS_READIDCMD;		/* Tag */
			s[1] = HiByte(MANU_MEI);	/* ManuIdNum */
			s[2] = LoByte(MANU_MEI);
			s[3] = HiByte(MANU_MEI_CD563);	/* ManuDevNum */
			s[4] = LoByte(MANU_MEI_CD563);
			s[5] = 0;			/* ManuRevNum */
			s[6] = 0;
			s[7] = 0;			/* ManuFlags */
			s[8] = 0;
			s[9] = 0;			/* ManuRomTagTableSz */
			s[10] = 0;
			s[11] = 0;			/* Status */
		} else
#endif
		{
			puts("bad read ID packet\n");
			ResetDevAndExit();
		}
	}
	lde.ManuIdNum = MakeInt16(s[1],s[2]);
	lde.ManuDevNum = MakeInt16(s[3],s[4]);
	lde.ManuRevNum = MakeInt16(s[5],s[6]);
	lde.ManuFlags = MakeInt16(s[7],s[8]);
	lde.ManuRomTagTableSize = MakeInt16(s[9],s[10]);
	return s[11];
}
#endif /* NULL_DIPIR */

#ifndef NULL_DIPIR

static DriverInitFunction *
FindInitFunction(void)
{
	if (lde.ManuFlags & DEVHAS_DRIVER)
	{
#ifdef DOWNLOAD_DRIVERS
		DIPIR_RETURN err;
		struct DriverHeader *sd;

		puts("Download driver\n");
		if ((err = DownloadDriver(&lde)) != DIPIR_RETURN_THREE_BUCKS) 
		{
			puts("Download failed, err="); puthex(err);
			ResetDevAndExit();
			return NULL;
		}
		/* Device vector table is right after the headers. */
		sd = (struct DriverHeader *)DriverBuffer;
		return sd->sd_Vectors.InitDisc;
#else
		puts("Can't download driver\n");
		ResetDevAndExit();
		return NULL;
#endif
	} else
	{
#ifdef DRIVERPACK
		DIPIR_RETURN err;
		struct DriverHeader *sd;

		if ((err = SwapInDriver(&lde)) != DIPIR_RETURN_THREE_BUCKS) {
			puts("Swapin failed, err="); puthex(err);
			ResetDevAndExit();
			return NULL;
		}
		/* Get InitDisc vector from device vector table */
		sd = (struct DriverHeader *)DriverBuffer;
		return sd->sd_Vectors.InitDisc;
#else
		struct DriverSelect *ds;
		for (ds = driverSelect;  ds->dsel_Init != NULL;  ds++)
		{
			if (ds->dsel_Manu == lde.ManuIdNum &&
			    ds->dsel_ManuDev == lde.ManuDevNum)
			{
				return ds->dsel_Init;
			}
		}
		puts("driver not found");
		return NULL;
#endif
	}
}
#endif /* NULL_DIPIR */

#ifndef NULL_DIPIR

static void
CleanupDevice(void)
{
	uint8 poll;
	ExpansionBus *xb = XBUS_ADDR;	/* all production machines */

	SELECTXBUS(lde.device);
	while ((xb->xb_SetExpCtl & XB_CPUHASXBUS) == 0)
		continue;
	/* Clear the media access bit */
	poll = (uint8)POLL;
	WRITEPOLL((uint32)poll|XBUS_POLL_MEDIA_ACCESS);

	/* Now we start to talk to it! */
	if (TESTSTAT(poll))
	{
		uint8 foo;
		/* junk in the status fifo */
		while (TESTSTAT(poll))
		{
			foo = (uint8)xb->xb_CmdStat[0];
			SELECTXBUS(lde.device);
			poll = (uint8)POLL;
		}
	}
	if (TESTREAD(poll))
	{
		/* Junk in the data read fifo */
		ResetDevAndExit();
	}
	/* We don't care about the write fifo */
#if 0
	if (TESTWRITE(poll))
	{
		/* Junk in the data write fifo */
		ResetDevAndExit();
	}
#endif
}
#endif /* NULL_DIPIR */

#ifndef NULL_DIPIR

static DIPIR_RETURN
dipiru(uint32 unit)
{
	DIPIR_RETURN errstat;
	uint8 stat;
	DriverInitFunction *pInitDisc = 0;
	static uint8 ReadIdCmd[7] = { XBUS_READIDCMD, 0, 0, 0, 0, 0, 0 };

	puts("dipiru: unit="); puthex(unit);
	lde.CurrentBuff = lde.databuff1;
	lde.device = (uint8)(unit & XB_DipirMask);

	/* Clear the media access bit and flush all fifos */
	CleanupDevice();

	/* Read the device ID to decide which driver to use */
	puts("Send ReadId\n");
	SquirtOutCmd(ReadIdCmd, sizeof(ReadIdCmd));
	stat = ParseReadIdStatus();
	puts(" MIdNum=");puthex(lde.ManuIdNum);
	puts(" MDevNum=");puthex(lde.ManuDevNum);
	puts(" MRevNum=");puthex(lde.ManuRevNum);
	puts(" MFlags=");puthex(lde.ManuFlags);
	puts(" MDrvrSize=");puthex(lde.ManuRomTagTableSize);
	puts(" stat=");puthex(stat);

	/* ignore any error here at this time */
	/*** if (stat & 0x10 ) ??? */
	
	/* Call the initialization routine for the device driver. */
	pInitDisc = FindInitFunction();
	puts("InitDisc="); puthex(pInitDisc);
	if (pInitDisc == NULL)
		return DIPIR_RETURN_NODISC;
	puts("PreInitDisc\n");
    	errstat = (*pInitDisc)(&lde);
	puts("PostInitDisc, errstat="); puthex(errstat);
	dvr = lde.dvr;
    	if (errstat == 0)
    	{
		return DIPIR_RETURN_NODISC;
    	}

	/* Now we can do device dependent stuff since */
	/* we have initialized the driver */

	/* Major decision point.  What do we do with it? */
	errstat = cd_is_3do();
	if (errstat < 0)
	{	/* caught bad 3do disk */
		EJECTDISC();
		return DIPIR_RETURN_TROJAN;	/* to original program */
	}
	if (errstat == DIPIR_RETURN_SPECIAL)
	{	/* Dale wanted it, Toby ok'ed it.  Return even in external! */
		return DIPIR_RETURN_SPECIAL;	/* to original program */
	}
#ifndef ALLOW_NONZERO_DEVICE
	/* Other than the boot device? */
	if (lde.device != BootDeviceUnit())
	{
		if (errstat != DIPIR_RETURN_DATADISC)
		{	/* Not 3do data disc */
			EJECTDISC();
			return DIPIR_RETURN_EXT_EJECT;
		}
		puts("Data disk in 2nd drive!\n");
		return DIPIR_RETURN_DATADISC;
	}
#endif
	/* Good disc, but not 3DO disc.  Softboot to the ROM applications. */
	if (errstat == DIPIR_RETURN_ROMAPPDISC)
	{
		puts("dipiru reboot\n");
		reboot((uint32 *)MAGIC_RESET,0,0,0);
		/* just in case we are a development system */
		return DIPIR_RETURN_ROMAPPDISC;
	}

	/*
	 * gets here if cdromdipir on 3do disk returns >0.  May be
	 * a data disc, or something yet to be defined.
	 */
	puts("Returning to previous OS\n");
	return errstat;
}
#endif /* NULL_DIPIR */

#ifndef NULL_DIPIR

DIPIR_RETURN
UncleDipir(void)
{
#ifndef NO_DIPIR_FMV
	/* This function should be integrated with dipiru. */
	DriverInitFunction *pInitDisc;

	lde.ManuIdNum = MANU_3DO;
	lde.ManuDevNum = MANU_3DO_FMV;
#ifdef DRIVERPACK
	{
		struct DriverHeader *sd;
		DIPIR_RETURN err;

		if ((err = SwapInDriver(&lde)) != DIPIR_RETURN_THREE_BUCKS)
		{
			puts("Swapin failed\n");
			ResetDevAndExit();
			return err;
		}
		/* Get InitDisc vector from device vector table */
		sd = (struct DriverHeader *)DriverBuffer;
		pInitDisc = sd->sd_Vectors.InitDisc;
	}
#else
	{
		pInitDisc = FMV_InitDisc;
	}
#endif
	puts("Calling FMV_InitDisc\n");
	if ((*pInitDisc)(&lde) == 0)
    	{
		/* FMV_InitDisc always returns 1, so this cannot happen. */
		return DIPIR_RETURN_NODISC;
    	}
	dvr = lde.dvr;
	if (cd_is_3do() != DIPIR_RETURN_THREE_BUCKS)
	{
		puts("Fake FMV Card - Ejecting :-)!\n");
		EJECTDISC();
		reboot((uint32 *)MAGIC_RESET,0,0,0);
		/* In case we're on development system */
		return DIPIR_RETURN_TROJAN;	
	}
	puts("FMV card kosher\n");
#endif /* NO_DIPIR_FMV */
	return DIPIR_RETURN_THREE_BUCKS;
}
#endif /* NULL_DIPIR */

static uint32 OldSlackTime;

static void
TimerRestore(void)
{
	*Slack = OldSlackTime;
}

static void
ExpInit(void)
{
	SetXBusSpeed(0);
}


static void
TimerInit(void)
{
	HardTimer *ht;
	uint32 *p;
	ht = (HardTimer *)(Timer0);
	ht += 14;	/* 15th timer */
	/* for red only! */
	{
		int nsecs;
		int ticks;
		OldSlackTime = *Slack;
		nsecs = (100*2*1000000)/50000;
		ticks = (16000*100)/nsecs;
		*Slack = (uint32)((uint32)ticks - 64);
		ht->ht_cnt = ((1000)/16) - 1;
		ht->ht_cntBack = ((1000)/16) - 1;
	}
	p = (uint32 *)SetTm1;
	/* first clear all bits */
	p[1] = 0xff000000;
	p[0] = 0x73000000;
	/* make timer 14 cascade into timer 15 */
	/* timer 14's period is 1 msec */
	/* timer 15 counts milliseconds */
}

DIPIR_RETURN
dipir(void)
{
#ifndef NULL_DIPIR
	jmp_buf jb;
#endif
	uint32 dipir1,dipir2;
	ExpansionBus *xb = XBUS_ADDR;
	DIPIR_RETURN ret = DIPIR_RETURN_TRYAGAIN;
	vuint32 *adbio_pointer = ADBIO;
	uint32 saved_rom_select = 0;

	lde.DipirRoutines = &dr;
	lde.device = 0;		/* Set default */
#ifdef DEBUG
	PutCharInit();
#endif

#ifndef	ROMBUILD
	/*
	 * Enable IRQs to allow debugging.  IRQs should be disabled on 
	 * production machines.  We disable *ANYWAY* from ROM code, to make
	 * bad-guy explorations harder!   -Bryce
	 */
	EnableIrq();	/* !!! are other IRQs disabled? */
#endif /* ROMBUILD */
#ifdef	LEAKY_DIPIR
	EnableIrq();	/* Don't kill the debugger */
#endif

	if (IS_ANVIL()) {
		/* Disable Anvil watchdog timer.
		 * The SoftReset that got us here was supposed to disable it,
		 * but the hardware has a bug.  Luckily, hardware has another
		 * bug that allows us to disable it manually.
		 */
		((Madam*)MADAM)->AnvilFeature &= ~(MADAM_AnvilWatchDog);
	}

	puts("Welcome to Dipir\n");
	RSAInit();
	puts("after RSAInit\n");
	InitECC();

	dipir1 = xb->xb_DIPIR;
	dipir2 = xb->xb_DIPIR2;
	puts("dipir1="); puthex(dipir1);
	puts("dipir2="); puthex(dipir2);

	TimerInit();
	ExpInit();
	entryMctl = ModMCTL(MCTL_OP_NOP, 0);
#ifdef DRIVERPACK
	PackFile = OpenRomFile(RT_SUBSYS_ROM, ROM_DIPIR_DRIVERS);
#endif

	/*
	 * Save state of ADBIO pin which selects primary or secondary
	 * ROM. Softreset will cause the hardware to map in the ROM
	 * to execute reset code - so force ROM select to primary
	 * ROM with OS. If there isn't a hard reset,
	 * state of CLIO ADBIO pin is restored before returning from dipir.
	 */
	saved_rom_select = *adbio_pointer & ADBIO_OTHERROM;
	puts("saved adbio="); puthex(saved_rom_select);
	*adbio_pointer |= ADBIO_OTHERROM_EN;
	*adbio_pointer &= ~(ADBIO_OTHERROM);

#ifdef	NULL_DIPIR
	puts("Null Dipir\n");
	ret = DIPIR_RETURN_THREE_BUCKS;	/* Fake it */
#else
	jmp_back = &jb;	
	if (dipir1 == 0 && dipir2 == 0)
	{
		/* Dipir for no aparent reason?  Must be Uncle Dipir! */
		if (setjmp(jb) == 0)
		{	
			ret = UncleDipir();
			ret = DIPIR_RETURN_THREE_BUCKS; // ZZZ ???
			puts("UncleDipir returns="); puthex(ret);
		} else {
			ret  = DIPIR_RETURN_TRYAGAIN;	/* Special code? */
		}
		puts("Exit Uncle="); puthex(ret);
	} else
	{
		thdoflags = GetCurrent3DOFlags();
		puts("ThdoFlags="); puthex(thdoflags);
		if ((thdoflags & _3DO_LUNCH) &&
		    !(thdoflags & _3DO_DATADISCOK) &&
		    !(thdoflags & _3DO_NORESETOK)) {
			puts("hardboot\n");
			hardboot();
		}
		WaitMills(300); /* per spec */

		/* We know this is a boot-time dipir.
		 * Turn video back on, which restores the "static screen"
		 * which was set up by the boot ROM.
		 */
		ModMCTL(MCTL_OP_BITSET, VSCTXEN | CLUTXEN);
		if (dipir1 & XB_DipirNOSR)
		{
			puts("dipir1\n"); 
			if (setjmp(jb) == 0)
			{
				ret = dipiru(dipir1);
			} else
			{
				puts("dipir1 abort\n");
				ret = DIPIR_RETURN_TRYAGAIN;
			}
		}
		if (dipir2 & XB_DipirNOSR)
		{
			puts("dipir2\n");
			if (setjmp(jb) == 0)
			{
				ret = dipiru(dipir2);
			} else
			{
				puts("dipir2 abort\n");
				ret = DIPIR_RETURN_TRYAGAIN;
			}
		}

		/* Disable video again, because OS expects video to be off. */
		/* ModMCTL(MCTL_OP_SET, entryMctl); */
	}

	if (ret != DIPIR_RETURN_TRYAGAIN)
	{
		/* Give the device driver a chance to clean up. */
		DEINITDISC();
	} else
	{
		/* Don't try to talk to the device if we're returning because
		 * of a dipir-induced abort.  The device may be locked.
		 * Instead, do a POLL-RESET to the device.
		 * If we aborted, device may be in a bad state 
		 * (e.g. we might have sent only part of a command sequence).
		 * Reset it to get it in a known state for the next dipir.  */
		WRITEPOLL(XBUS_POLL_RESET);
		WaitMills(100);		/* Expansion Spec Aug, 1993 */
		WRITEPOLL(0);
		WaitMills(600);		/* Expansion Spec Aug, 1993 */
	}
#endif

#ifdef DRIVERPACK
	CloseRomFile(PackFile);
	PackFile = NULL;
#endif
	TimerRestore();
	/*
	 * Restore state of ADBIO pin which selects primary or secondary
	 * ROM to state before dipir happened.
	 * Make sure output enable is on.
	 */
	*adbio_pointer |= (saved_rom_select | ADBIO_OTHERROM_EN);
	puts("Exit Dipir="); puthex(ret);
	return ret;
}


#ifdef DOWNLOAD_DRIVERS
/*****************************************************************
 * Download Dipir Driver
 * 
 * Format of ROM is as follows:
 *
 * 132 bytes          Disc Label for ROM
 * up to 2K           Rom Tag Table
 * 0-4K      (opt)    Device Dipir (found via RomTag)
 * 0-?K      (opt)    New OS (found via RomTag)
 * up to 1K           Dipir Driver for device (found via RomTag)
 * up to ?K           Kernel Driver for Device (found via RomTag) [NOT YET]
 * 0-?K      (opt)    FileSystem (found via RomTag)
 *
 * A Dipir Driver is formated as described by struct DriverHeader.
 ******************************************************************/

RomTag *
FindRTSpec(RomTag *rt, int subtype, int type, int typeSpecific)
{
	/* Find RomTag */
	while (rt->rt_SubSysType != 0)
	{
		if (rt->rt_SubSysType == subtype && rt->rt_Type == type &&
		    rt->rt_TypeSpecific == typeSpecific)
			return rt;
		rt++;
	}
	return 0;
}

static bool 
CheckStatus(DipirExpDev *ed, uint8 tagByte)
{
	uint8 stat;

	/* Wait for status */
	while (TESTSTAT(ReadPoll(ed->ded_Unit)) == 0)
		continue;
	
	/* Read the status byte.  If we get the tagByte, read another one. */
	stat = ReadStatusByte();
	if (stat == tagByte)
	{
		stat = ReadStatusByte();	
	}
	return (stat & XBUS_STATUSERR) ? true : false;
}

static int 
DownloadDevice(DipirExpDev *ed, uint32 offset, uint8 *dest, uint32 size)
{
	volatile uint32 *src;
	int i;
	uint8 cmd[7];
	ExpansionBus *xb;

#define CHUNKIT(a)	(((a)+sizeof(uint32)-1) / sizeof(uint32))
	
	xb = XBUS_ADDR;
	cmd[0] = XBUS_READDRIVERCMD;	/* read driver from rom */
	cmd[1] = (uint8)(offset >> 24);
	cmd[2] = (uint8)(offset >> 16);
	cmd[3] = (uint8)(offset >> 8);
	cmd[4] = (uint8)(offset);
	cmd[5] = (uint8)(size >> 8);
	cmd[6] = (uint8)(size);

	AsyncSquirtOutCmd(cmd, (int)sizeof(cmd));

	size = CHUNKIT(size);	/* number of longwords */
	src = &xb->xb_Data[0];
	for (i = 0; i < size; i++) 
	{
		/* Wait for data; read it in */
		while (TESTREAD(ReadPoll(ed->ded_Unit)) == 0) 
			continue;
		*dest++ = (uint8)*src;
		*dest++ = (uint8)*src;
		*dest++ = (uint8)*src;
		*dest++ = (uint8)*src;
	}
	if (CheckStatus(ed, XBUS_READDRIVERCMD))
		return -1;
	return 0;
}


DIPIR_RETURN
ReadDriver(DipirExpDev *ed, uint32 off, uint32 size)
{
	uint32 offset = off;
	AIFHeader aifheader;
        int err;

	puts("ReadDriver:"); puthex(DriverBuffer);

	/* we know where and how big the raw image is.  
	   get the AIF header so we can figure our logical size */
	puts("reading aif hdr\n");
	if (DownloadDevice(ed, off, (uint8 *)&aifheader, sizeof(AIFHeader)) < 0)
	{
		puts("cannot read aif header");
		return DIPIR_RETURN_TROJAN;
	}
	
  	ed->ded_LogicalSizeOfDriver = AIFSize(&aifheader);
	ed->ded_PhysicalSizeOfDriver = size;
	if (ed->ded_LogicalSizeOfDriver > size)
		size = ed->ded_LogicalSizeOfDriver;
	if (size > DriverBufferSize) 
	{
		puts("driver code too big\n");
		return DIPIR_RETURN_TROJAN;
	}

	ed->ded_Driver = (uint32 *)DriverBuffer;
	if (DownloadDevice(ed, offset, (uint8 *)DriverBuffer, size) < 0)
	{
		puts("cannot read driver");
		return DIPIR_RETURN_TROJAN;
	}

        /* Now, we have to RSA Check the driver */
	{
		_3DOBinHeader *bh = (_3DOBinHeader *)
			(((char *)DriverBuffer) + sizeof(AIFHeader));
        	bh->_3DO_SignatureLen = 0;
	}
        DipirInitDigest();
	DipirUpdateDigest(((char *)DriverBuffer),(int)(size - SIG_LEN));
        DipirFinalDigest();

        err = RSAFinalTHDO(((unsigned char *)DriverBuffer) + size - SIG_LEN,
				SIG_LEN);
#ifndef NO_RSA_DRIVER_CHECK
	if (!err) return DIPIR_RETURN_TROJAN;
#endif
	puts("calling reloc code\n");
	callaif2(0,0,(unsigned char *)DriverBuffer);
	
	return DIPIR_RETURN_THREE_BUCKS;
}

DIPIR_RETURN
DownloadDriver(struct DipirEnv *lde) 
{
	DipirExpDev ed;
	DIPIR_RETURN err;
	RomTag *rt,*drt;
	uint32 offset;
	int32 rttlen;

	/* first, set up the mini DipirExpDev */
	ed.ded_Unit = lde->device;

	/* Read volume label */
	if (DownloadDevice(&ed, 0, 
		(uint8 *)lde->DiscLabel, sizeof(DiscLabel)) < 0)
	{
		puts("cannot read vol label");
		return DIPIR_RETURN_TROJAN;
	}

	/* Read rom tag table, one entry at a time. */
	for (rt = (RomTag*)lde->databuff2, offset = sizeof(DiscLabel);
	     ;
	     rt++, offset += sizeof(RomTag))
	{
		if ((uint8*)rt >= (uint8*)lde->databuff2 + 
			MAX_BLOCK_SIZE - sizeof(RomTag))
		{
			puts("RomTag table too big");
			return DIPIR_RETURN_TROJAN;
		}
		if (DownloadDevice(&ed, offset, (uint8*)rt, sizeof(RomTag)) < 0)
		{
			puts("cannot read romtag");
			return DIPIR_RETURN_TROJAN;
		}
		if (rt->rt_SubSysType == 0)
			break;
	}
	/* Include the terminator entry */
	rt++;
	offset += sizeof(RomTag);
	rttlen = (uint8*)rt - (uint8*)(lde->databuff2);
	puts("dev rttlen="); puthex(rttlen);
	/* Must have at least one entry plus terminator. */
	if (rttlen < 2*sizeof(RomTag))
	{
		puts("RomTag table too small");
		return DIPIR_RETURN_TROJAN;
	}

	/* Check for DEVDIPIR code for this device */
	drt = FindRT((RomTag *)lde->databuff2, RSANODE, RSA_DEVDIPIR); 
	if (drt != NULL) 
	{
		puts("found DEVDIPIR");
		if (drt->rt_Size > BOOTSTRAPSIZE)
		{
			puts("DEVDIPIR too big");
			return DIPIR_RETURN_TROJAN;
		}
		if (DownloadDevice(&ed, drt->rt_Offset + sizeof(DiscLabel),
			(uint8 *)lde->CDRSACode, drt->rt_Size) < 0)
		{
			puts("cannot read DEVDIPIR");
			return DIPIR_RETURN_TROJAN;
		}
		/* RSA check the DEVDIPIR code on device (if present). */
		{
			_3DOBinHeader *bh = 
				(_3DOBinHeader *)(((char *)lde->CDRSACode) + 
						sizeof(AIFHeader));
			bh->_3DO_SignatureLen = 0;
		}
		DipirInitDigest();
		DipirUpdateDigest((char *)lde->CDRSACode,
				(int)drt->rt_Size-SIG_LEN);
		DipirFinalDigest();
		err = RSAFinalTHDO((unsigned char *)(lde->CDRSACode) +
				drt->rt_Size - SIG_LEN, SIG_LEN);
#ifndef NO_RSA_DRIVER_CHECK
		if (!err) return DIPIR_RETURN_TROJAN;
#endif
	}

	/* Find the DIPIR Driver in the romtag table and download it. */
	rt = FindRTSpec((RomTag *)lde->databuff2, RSANODE, RSA_DRIVER, 0);
	if (rt == NULL)
	{
		puts("no driver found");
		return DIPIR_RETURN_TROJAN;
	}
	err = ReadDriver(&ed, rt->rt_Offset + sizeof(DiscLabel), rt->rt_Size);
	if (err != DIPIR_RETURN_THREE_BUCKS) 
	{
		puts("ReadDriver failed");
		return (err);
	}

	/* Is there DEVDIPIR code on this dev to execute? */
	if (drt != NULL)
	{
		if (AIFSize((AIFHeader*)lde->CDRSACode) > BOOTSTRAPSIZE)
		{
			puts("DEVDIPIR code+bss too big");
			return DIPIR_RETURN_TROJAN;
		}
		err = CallAIF(lde->CDRSACode, lde);
	}
	return err;
}
#endif /*DOWNLOAD_DRIVERS*/

#ifdef DRIVERPACK

static int
FindPackEntry(uint32 ManuId, uint32 ManuDev, struct DriverDesc *dd)
{
	uint32 i;
	uint32 nDrivers;
	uint32 offset;
	uint32 magic;

	offset = 0;
	if (ReadRomFile(PackFile, offset, sizeof(uint32), 
			(uint8*)&magic) != sizeof(uint32)) {
		puts("pack read error (magic)");
		goto Corrupt;
	}
	if (magic != PACK_MAGIC) {
		puts("Bad pack magic: "); puthex(magic);
		goto Corrupt;
	}
	offset += sizeof(uint32);
	if (ReadRomFile(PackFile, offset, sizeof(uint32), 
			(uint8*)&nDrivers) != sizeof(uint32)) {
		puts("pack read error (nDrivers)");
		goto Corrupt;
	}
	offset += sizeof(uint32);
	/* Read each entry in the header until we find the one we want. */
	for (i = 0;  i < nDrivers;  i++)
	{
		if (ReadRomFile(PackFile, offset, sizeof(struct DriverDesc), 
				(uint8*)dd) != sizeof(struct DriverDesc)) {
			puts("pack read error (driver)");
			goto Corrupt;
		}
		offset += sizeof(struct DriverDesc);
		if (dd->dd_ManuId == ManuId && dd->dd_ManuDev == ManuDev)
			return 0;
	}
	puts("pack entry not found");
	return -1;

Corrupt:
	puts("corrupt pack file");
	return -1;
}

DIPIR_RETURN
SwapInDriver(struct DipirEnv *lde)
{
	struct DriverDesc dd;

	if (FindPackEntry(lde->ManuIdNum, lde->ManuDevNum, &dd))
	{
		puts("driver not found");
		return DIPIR_RETURN_TROJAN;
	}
	if (dd.dd_Size > DriverBufferSize)
	{
		puts("driver code too big");
		return DIPIR_RETURN_TROJAN;
	}
	/* Read in the driver */
	if (ReadRomFile(PackFile, dd.dd_Offset, dd.dd_Size, 
		(uint8*)DriverBuffer) != dd.dd_Size)
	{
		puts("cannot read driver from pack file");
		return DIPIR_RETURN_TROJAN;
	}
	/* Sanity check */
	{
		struct DriverHeader *sd;
		sd = (struct DriverHeader *)DriverBuffer;
		if (sd->sd_Magic != DRIVER_MAGIC )
		{
			puts("driver has bad magic number="); 
			puthex(sd->sd_Magic);
			return DIPIR_RETURN_TROJAN;
		}
	}
	if (AIFSize((AIFHeader*)DriverBuffer) > DriverBufferSize)
	{
		puts("driver code+bss too big");
		return DIPIR_RETURN_TROJAN;
	}
	/* Relocate driver */
	callaif2(0,0,(unsigned char *)DriverBuffer);
	return DIPIR_RETURN_THREE_BUCKS;
}

#endif /* DRIVERPACK */

