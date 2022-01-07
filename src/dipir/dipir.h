/*	$Id: dipir.h,v 1.52.1.1 1994/12/21 21:43:02 markn Exp $
**
**	Describes interface between rom-dipir and cdrom-dipir
**
**	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
*/

#include "rsa.h"
#include "global.h"
#include "bsafe2.h"
#include "md5.h"

#ifndef RSA_APPSPLASH
#define RSA_APPSPLASH           0x14    /* App splash screen image */
#endif
#ifndef RSA_DEPOTCONFIG
#define RSA_DEPOTCONFIG         0x15    /* Depot configuration file */
#endif
#ifndef RSA_DEVICE_INFO
#define RSA_DEVICE_INFO         0x16    /* Device ID & related info */
#endif


#define	MAGIC_KERNELADR	0x28	/* Address with kernel location, which Drew */
				/* promises has been refreshed. */
#define	SYSROMTAG_ADDR	0x2C	/* Address of pointer to ROM's RomTag table */
#define sysRomTags	(*((RomTag**)(SYSROMTAG_ADDR)))
#define	MAGIC_RESET	0x30	/* Power on reset location, which Drew */
				/* promises has been refreshed. */

/* Magic numbers of various file types. */
/* These must match the values in make_pakhdr.sh */
#define	PACK_MAGIC	0x925FBCD0 		
#define	DRIVER_MAGIC	0x3512ABC0

/* Some well-known addresses */
#define OS_LOAD_ADDR		0xA0000	/* Scratch area */
#define	MAX_OS_SIZE		0x100000
#ifndef ROMBUILD
/* Address of pack file image (development only; non-dev has it in ROM */
#define	PACK_FILE_ADDR		0x201000
#endif /*ROMBUILD*/

#define	SIG_LEN			64	/* Length of RSA signature */
#define	MAX_ROMTAG_BLOCK_SIZE	2048
/* These should be in discdata.h? */
#define	FRAMEperSEC		75
#define	SECperMIN		60

#define	MAX_OS_COMPONENTS	8


/*
 *	Return codes as visible to operator.
 */
typedef int32 DIPIR_RETURN;
#define	DIPIR_RETURN_TROJAN	-1	/* We ejected 'em */
#define	DIPIR_RETURN_TRYAGAIN	 0	/* Dipir failed.  Try again. */
#define	DIPIR_RETURN_THREE_BUCKS 1	/* Good 3DO disc */
#define	DIPIR_RETURN_DATADISC	 2	/* Data disc.  Initiate discchange */
#define	DIPIR_RETURN_NODISC	 3	/* Empty drive */
#define	DIPIR_RETURN_ROMAPPDISC	 4	/* Photo, Red Book Audio, etc. */
#define	DIPIR_RETURN_FMVABORT	 5	/* FMV card gave us an abort */
#define	DIPIR_RETURN_EXT_EJECT	 6	/* Disc ejected from external drive */
#define	DIPIR_RETURN_SPECIAL	 7	/* Special 3DO disc */

/* Operations for ModMCTL. */
#define	MCTL_OP_NOP		0
#define	MCTL_OP_BITSET		1
#define	MCTL_OP_BITCLR		2
#define	MCTL_OP_SET		3


/* Routines provided by rom dipir code for downloaded */
/* driver and downloaded signature code */
/* routines provided by rom dipir code (cdromdipir/driver--calls--> dipir) */
typedef struct DipirRoutines
{
	void	(*putc)(char c);
	void	(*puts)(char *s);	/* msg display */
	void	(*puthex)(int);
	void	(*ResetTimer)(void);	/* reset timer to 0 */
	int	(*ReadTimer)(void);
	void	(*CheckPoint)(int);	/* for debugging */
	void	(*ResetDevAndExit)(void);	/* hard reset */
	void	(*SquirtOutCmd)(char *src,int len);
	void	(*reboot)(uint32 *,int,int,int);
	void	(*InitDigest)(void);
	void	(*UpdateDigest)(char *input, int len);
	void	(*FinalDigest)(void);
	int	(*RSAFinalAPP)(unsigned char *sig, int len);	/* 0=bad */
	int	(*RSAFinalWithKey)(A_RSA_KEY *key, unsigned char *sig, int len);
	uint32	(*reserved5)(void);
	uint32	(*reserved6)(void);
	void	(*memmove)(POINTER, POINTER, unsigned int);
	int	(*Read)(int blk, int bytes);	/* syncronous, multiblock */
	int	(*WaitXBus)();
	void	(*AsyncSquirtOutCmd)(char *src,int len);
	int	(*RSAFinalTHDO)(unsigned char *sig, int len);	/* 0=bad */
	uint32 *(*ROMDoesTransfer)(uint32 *image);
	void	(*hardboot)(void);		/* Major WHAM! */
	uint32	(*reserved1)(void);
	bool  (*rsacheck)(void *buff, int32 buffsize);
	int32  (*genrsacheck)(unsigned char *input,int inputLen,unsigned char *signature,int signatureLen);
	int32	(*SectorECC)(uint8 *buf);
	int	(*SetXBusSpeed)(uint32 speedRate);
	uint32	DipirRoutinesVersion;
	void * 	(*OpenRomFile)(uint32 subsysType, uint32 type);
	uint32	(*ReadRomFile)(void *file, uint32 offset, uint32 length, uint8 *buffer);
	void	(*CloseRomFile)(void *file);
	uint32	(*DipirEndAddr)(void);
	void *	(*FindSysRomTag)(int subsys, int type);
	int32	(*DisplayImage1)(void *image);
	int32	(*DisplayImage)(void *image, char *pattern);
	uint32	(*BootDeviceUnit)(void);
} DipirRoutines;
/* need squirt out command no wait for status */
/* need get status, wait status, sigh */

/* local variables for the rom dipir code: */
typedef struct DipirEnv
{
	int32	VolumeLabelBlock;
	int32	FirstBlock;
	int32	RomTagBlock;
	char	*databuff1;	/* 2K buffer */
	char	*databuff2;	/* 2K buffer */
	char	*CDRSACode;	/* 8K buffer coniguous with the above */
	char	*statbuff;
	char	*CurrentBuff;
	int	DataExpected;
	struct  DiscLabel *DiscLabel;
	struct	DipirRoutines *DipirRoutines;
	struct	DeviceRoutines *dvr;
	uint32	ManuIdNum;
	uint32	ManuDevNum;
	uint32	ManuRevNum;
	uint32	ManuFlags;
	uint32	ManuRomTagTableSize;
	char	statcnt;
	char	device;
	uchar	dipir_version;
	uchar	dipir_revision;
	int	BlockSize;
	unsigned char *digest;
	uint8	DeviceFlags;
	uint8	reserved2, reserved3, reserved4;
	unsigned char *DriverMemory;
} DipirEnv;

/* DeviceFlags */
#define	DEV_NO_TAG_BYTE		0x1

/* Routines provided by the Device driver for use by */
/* romdipir and cddipir */
/* routines provided by device drivers (dipir --calls--> device driver) */
typedef struct DeviceRoutines
{
	int	(*ReadBlock)(int blockno); /* read a block @blockno */
	int	(*ReadDiscInfo)(void);
	int	(*ReadSessionInfo)(void);
	int	(*ReadTOC)(int);
	void	(*EjectDisc)(void);	/* Yes, even for FMV cards :-) */
	void	(*AsyncReadBlock)(int blk);
	uint32	(*GetDriverInfo)(void);	/* Driver ID and version number */
	int32	(*CheckGoldDisc)(void); /* check for gold disc */
	int32   (*GetMfgPlant)(void);   /* returns mfg plant number */
	uint32	DeviceRoutinesVersion;	/* version # of DeviceRoutines */
	int	(*WaitReadBlock)(void);	/* Wait for AsyncReadBlock() */
	void	(*DeinitDisc)(void);	/* Opposite of InitDisc */
	void	(*BeginLongOp)(void);	/* Do something time-consuming */
	void	(*EndLongOp)(void);	/* Do something time-consuming */
	int32	(*CheckCopyProt)(uint32 a); /* Check copy-protection */
	int32	(*LastECC)(void);	/* Info on last ECC error */
} DeviceRoutines;

typedef	int DriverInitFunction(struct DipirEnv *lde);

#if defined(DOWNLOAD_DRIVERS) || defined(DRIVERPACK)
/*
 * Header of a downloadable driver, or a driver within the pack file.
 */
struct DriverHeader
{
	struct AIFHeader sd_AIFHeader;	/* AIF header */
	struct _3DOBinHeader sd_3DOHeader; /* 3DO header */
	struct DeviceVectors {		/* Device driver entrypoints */
		DriverInitFunction *InitDisc;
		uint32 Reserved1;
		uint32 Reserved2;
		uint32 Reserved3;
	} sd_Vectors;
	uint32 sd_Magic;		/* Magic number */
	uint32 sd_Version;		/* Version of the header (1) */
	uint16 sd_ManuId;		/* Copy of dd_ManuId */
	uint16 sd_ManuDev;		/* Copy of dd_ManuDev */
};
#endif

#ifdef APPSPLASH
struct VideoImage {
	uint8		vi_Version;	/* Version # of this header */
	char		vi_Pattern[7];	/* Must contain a SPLASH_PATTERN */
	uint32		vi_Size;	/* Size of image (bytes) */
	uint16		vi_Height;	/* Height of image (pixels) */
	uint16		vi_Width;	/* Width of image (pixels) */
	uint8		vi_Depth;	/* Depth of each pixel (bits) */
	uint8		vi_Type;	/* Representation */
	uint8		vi_Reserved1;	/* must be zero */
	uint8		vi_Reserved2;	/* must be zero */
	uint32		vi_Reserved3;	/* must be zero */
};
#define	HW_SPLASH_PATTERN	"BOOTSCR"
#define	APP_SPLASH_PATTERN	"APPSCRN"
#endif /* APPSPLASH */

#define	DEVICE_ROUTINES_VERSION_0	0xf4830000
#define	DIPIR_ROUTINES_VERSION_0	0xf4840000


#define	MANU_3DO			0x1000
#define	MANU_3DO_FMV			0x0020	/* Must match fmvdevh.id */
#define	MANU_3DO_CDDIPIR		0xF000	/* Must match make_pakhdr.sh */
#define	MANU_3DO_DEPOT			0x0030

#define	MANU_MEI			0x10
#define	MANU_MEI_CD			0x0001
#define	MANU_MEI_CD563			0x0563

#define	MANU_LC				0x544e
#define	MANU_LC_CD			1

/*
 *	Top 16 bits, driver ID.
 *	Bottom 16 bits, version number
 */
#define	DIPIR_DEVICE_ID_MEICDROM	0x00010000
#define	DIPIR_DEVICE_ID_FMVCARD		0x00020000
#define	DIPIR_DEVICE_ID_LOWCOSTCDROM	0x00030000
#define	DIPIR_DEVICE_ID_MEICD563ROM	0x00040000
#define	DIPIR_DEVICE_ID_DEPOT		0x00050000


/* definitions directly from the drive spec */
#define ACB_AUDIO_PREEMPH	1
#define ACB_DIGITAL_COPIABLE	2
#define ACB_DIGITAL_TRACK	4
#define ACB_FOUR_CHANNEL	8

typedef struct DiscInfo
{
	uint8	di_cmd;
	uint8	di_DiscId;
	uint8	di_FirstTrackNumber;
	uint8	di_LastTrackNumber;
	uint8	di_MSFEndAddr_Min;
	uint8	di_MSFEndAddr_Sec;
	uint8	di_MSFEndAddr_Frm;
} DiscInfo;

typedef struct TOCInfo
{
	uint8	toc_cmd;
	uint8	toc_reserved1;
	uint8	toc_AddrCntrl;
	uint8	toc_TrackNumber;
	uint8	toc_reserved2;
	uint8	toc_CDROMAddr_Min;
	uint8	toc_CDROMAddr_Sec;
	uint8	toc_CDROMAddr_Frm;
	uint8	toc_reserved3;
} TOCInfo;

extern struct RomTag *FindRT(struct RomTag *, int, int);
extern int32 CheckRT(struct RomTag *rt);

/* Helper macros for calling routines */
#define	DipirRoutinesVersionBefore(n) \
	(dipr->DipirRoutinesVersion < DIPIR_ROUTINES_VERSION_0+(n) || \
	 dipr->DipirRoutinesVersion > DIPIR_ROUTINES_VERSION_0+1000)
#define	DeviceRoutinesVersionBefore(n) \
	(dvr->DeviceRoutinesVersion < DEVICE_ROUTINES_VERSION_0+(n) || \
	 dvr->DeviceRoutinesVersion > DEVICE_ROUTINES_VERSION_0+1000)

/* routines provided by device drivers (dipir--calls--> device driver) */
#define READSESSIONINFO()	(*dvr->ReadSessionInfo)()
#define READBLOCK(a)		(*dvr->ReadBlock)((int)a)
#define READDISCINFO()		(*dvr->ReadDiscInfo)()
#define READTOC(a)		(*dvr->ReadTOC)((int)a)
#define EJECTDISC()		(*dvr->EjectDisc)()
#define ASYNCREADBLOCK(a)	(*dvr->AsyncReadBlock)((int)a)
#define	WAITREADBLOCK()		(*dvr->WaitReadBlock)()
#define	WAIT()			(DeviceRoutinesVersionBefore(1) ? \
					(*dipr->WaitXBus)() : \
					(*dvr->WaitReadBlock)())
#define	DEINITDISC()		{ if (DeviceRoutinesVersionBefore(2)) \
					; else if (dvr == NULL) \
					; else (*dvr->DeinitDisc)(); }
#define	BEGIN_LONG_OP()		{ if (DeviceRoutinesVersionBefore(3)) \
					; else (*dvr->BeginLongOp)(); }
#define	END_LONG_OP()		{ if (DeviceRoutinesVersionBefore(3)) \
					; else (*dvr->EndLongOp)(); }
#define CHECKGOLDDISC()		(*dvr->CheckGoldDisc)()
#define GETMFGPLANT() 		(*dvr->GetMfgPlant)()
#define	CHECK_COPY_PROT(a)	(DeviceRoutinesVersionBefore(4) ? \
					1 : (*dvr->CheckCopyProt)(a))
#define	LAST_ECC()		(DeviceRoutinesVersionBefore(4) ? \
					0xFFFFFFFF : (*dvr->LastECC)())

/* routines provided by rom dipir code (cdromdipir/driver--calls--> dipir) */
#define CHECKPOINT(x)	(*dipr->CheckPoint)((int)x)	/* writes value to ram */
#define RESETTIMER()	(*dipr->ResetTimer)()	/* restart realtime clock */
#define READTIMER()	(*dipr->ReadTimer)()	/* get current tick in msecs */
#define SQUIRTOUTCMD(a,b)	(*dipr->SquirtOutCmd)((char *)a,(int)b)
#define RESETDEVANDEXIT()	(*dipr->ResetDevAndExit)()
#define REBOOT(x,a,b,c)	(*dipr->reboot)(x,a,b,c)
#define DIGESTINIT()	(*dipr->InitDigest)()
#define UPDATEDIGEST(a,b)	(*dipr->UpdateDigest)((char *)a,(int)b)
#define FINALDIGEST()		(*dipr->FinalDigest)();
#define RSAFINALTHDO(a,b)	(*dipr->RSAFinalTHDO)((unsigned char *)a,(int)b)
#define RSAFINALAPP(a,b)	(*dipr->RSAFinalAPP)((unsigned char *)a,(int)b)
#define RSAFINALWITHKEY(a,b,c)	(*dipr->RSAFinalWithKey)(a,b,c)
#define READ(a,b)	(*dipr->Read)((int)a,(int)b)
#define MEM_MOVE(a,b,c)	(*dipr->memmove)((POINTER)(a),(POINTER)(b),(unsigned int)(c))
#define ASYNCSQUIRTOUTCMD(a,b)	(*dipr->AsyncSquirtOutCmd)(a,b)
#define ROMDOESTRANSFER(a)	(*dipr->ROMDoesTransfer)(a)
#define	WAITXBUS()		(*dipr->WaitXBus)()
#define	SECTORECC(a)		(*dipr->SectorECC)(a)
#define	SETXBUSSPEED(a)		(*dipr->SetXBusSpeed)(a)
#define	OPENROMFILE(a,b)	(*dipr->OpenRomFile)(a,b)
#define	READROMFILE(a,b,c,d)	(*dipr->ReadRomFile)(a,b,c,d)
#define	CLOSEROMFILE(a)		(*dipr->CloseRomFile)(a)
#define	DIPIR_ENDADDR()		(DipirRoutinesVersionBefore(1) ? \
					0xE000 : (*dipr->DipirEndAddr)())
#define	FINDSYSROMTAG(a,b)	((RomTag*)(DipirRoutinesVersionBefore(1) ? \
					0 : (*dipr->FindSysRomTag)(a,b)))
#define	DISPLAYIMAGE(a,b)	((DipirRoutinesVersionBefore(2) || \
				  dipr->DisplayImage == NULL) ? \
					DefaultDisplayImage(a,b) : \
					(*dipr->DisplayImage)(a,b))
#define	BOOTDEVICEUNIT()	(DipirRoutinesVersionBefore(3) ? 0 : \
					(*dipr->BootDeviceUnit)())

#define	SingleBuffer(de,buff)	{ (de)->CurrentBuff = (buff); }
#define	DoubleBuffer(de)	{ (de)->CurrentBuff = (de)->databuff1; \
				  NextBuff = (de)->databuff2; }
#define	SwitchBuffers(de)	{ char *t = (de)->CurrentBuff; \
				  (de)->CurrentBuff = NextBuff; \
				  NextBuff = t; }

/* used by kernel and dipir (direct memory debugging) */
#ifndef DEBUG
#define puts(a)		;
#define puthex(a)	;
#define do_return(a)	;
#define PUTC(x)		;
#define PUTS(x)		;
#define PUTHEX(x)	;
#else
#define puts(a)	realputs(a)
#define puthex(a)	realputhex((int32)(a))
#define PUTC(x)		(*dipr->putc)(x)
#define PUTS(x)		(*dipr->puts)(x)
#define PUTHEX(x)	(*dipr->puthex)((int)x)
#endif


/* only used by dipir code (assumes dipir context) */
int RSAFinalTHDO(unsigned char *signature, int signatureLen);
int RSAFinalAPP(unsigned char *signature, int signatureLen);
int RSAFinalWithKey(A_RSA_KEY *key, unsigned char *signature,
				int signatureLen);
void InitDigest(MD5_CTX *);
void UpdateDigest(MD5_CTX *, unsigned char *input, int inputLen);
void FinalDigest(MD5_CTX *,unsigned char *result);

void DecryptBlock(void *buffer, unsigned long inputlen);

uint32 *XferCodeAndDoNotReboot(uint32 *image);

extern void realputs(char *s);
extern void realputchar(char c);
extern void realputhex(int32 iv);
extern B_memcmp(const char *a, const char *b, int n);

#define	LoByte(x)	((uint8)x)
#define	HiByte(x)	((uint8)(((uint16)(x)) >> 8))
#define	MakeInt16(H,L)	((((uint32)(H))<<8) | ((uint32)(L)))
#define	IS_ANVIL()	((((Madam*)MADAM)->MadamRev & MADAM_CHIPID_MASK) == \
				(MADAM_CHIPID_ANVIL << MADAM_CHIPID_SHIFT))
