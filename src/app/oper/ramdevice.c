/* $Id: ramdevice.c,v 1.82.1.2 1994/12/16 21:41:33 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "kernel.h"
#include "operror.h"
#include "super.h"
#include "mem.h"
#include "inthard.h"
#include "clio.h"
#include "madam.h"
#include "sysinfo.h"


extern Superkprintf(const char *fmt, ... );

extern void Duck(void);		/* from duckandcover.c */
extern void SoftReset(void);	/* from softreset.s */
extern void MissedMe(void);	/* from duckandcover.c */

/*#define DEBUG*/

/*
 * DBUG prints debug stuff (USER MODE ONLY)
 * KBUG prints debug stuff while in kernel mode
 * SINFO prints SRAM debug info, kernel mode.
 * RRBDBUG prints ReadRomByte debug info, kernel mode.
 */

#define DBUG(x)	 /*kprintf x*/
#define KBUG(x)	 /*Superkprintf x*/
#define SINFO(x)	 /*Superkprintf x*/
#define RRBDBUG(x)	 /*Superkprintf x*/

static void
myAbortIO(ior)
IOReq *ior;
{
}

/*#define ALLOCATE_RAMDISK*/	/* only for development */
#define MEI	/* define for real MEI rom burn */

#define BLOCKSIZE	1

#define NVRAM_START	(uint8 *)(0x03140000)

#define NVRAM_SIZE	(32*1024)
#define NVRAM_ABORTS

#define ONEMEG  (1024*1024)
#define HALFMEG (512*1024)

#define MAXUNITS 8
#define FLASH_START	(uint8 *) (0x3800000)
#define FLASH_SIZE  (ONEMEG)
#define FLASH_END	( (uint32) FLASH_START + FLASH_SIZE )
#define BOOTFLASH_START (uint8 *) (0x3A00000)
#define BOOTFLASH_SIZE (ONEMEG)
#define BOOTFLASH_END ( (uint32) BOOTFLASH_START + BOOTFLASH_SIZE )

uint8  *romfs = NULL;
uint32 romfs_size = 0;
uint32 rom_size = ONEMEG;

/* defines for rom2, the kanji font rom on fz1j and later hardware */
#define	ROM2_BASE	0x3000000
#define	ROM2_BASE_ANVIL	0x3800000
#define	ROM2_SIZE	ONEMEG

typedef enum DeviceUnits
{
    DEVUNIT_DRAM,
    DEVUNIT_FSROM,      /* starts at base of ROMFS                         */
    DEVUNIT_ROM,        /* real base of ROM                                */
    DEVUNIT_NVRAM_1,    /* base NVRAM                                      */
    DEVUNIT_UNCLEROM,   /* in the FMV card                                 */
    DEVUNIT_KANJIROM,   /* Kanji font ROM                                  */
    DEVUNIT_FLASHROM_1, /* Scientific Atlanta flash rom in big trace range */
    DEVUNIT_FLASHROM_2  /* Scientific Atlanta flash rom in big trace range */
} DeviceUnits;


typedef struct RamDev
{
    int rd_size;	/* in blocks */
    int rd_sizeinbytes;
    uint8 *rd_addr;	/* base address (uint8 *)*/
    int rd_bs;		/* basic block size */
} RamDev;

static RamDev RamDevices[MAXUNITS];

static void
ReadUncle(uint32 a,uchar *dst,int cnt)	/* Read a byte from Woody/Clio external rom */
{
   UncleRegs *wr = (UncleRegs *)UNCLEADDR;
   while (cnt--)
   {
   	wr->unc_AddressReg = a++;
   	*dst++ =  (uchar)wr->unc_DataReg;
   }
}

extern struct KernelBase *KernelBase;

static Item
ramInit(dev)
Device *dev;
{
int i;
uint32	rom2_baseaddr;		/* base address of kanji ROM */
uint32	retval = 0;
PlatformID platform;

	KBUG(("ramInit\n"));
	KBUG(("flags=%lx\n",KernelBase->kb_CPUFlags));

	/* Find which platform this box is running on */
	memset(&platform, 0, sizeof(PlatformID));
	if (SuperQuerySysInfo(SYSINFO_TAG_PLATFORMID, &platform, sizeof(PlatformID)) !=
						SYSINFO_SUCCESS)
	{
		KBUG(("Couldn't locate platform ID from SysInfo\n"));
	}

	/* Set ROMFS and ROMFS_SIZE for use by the ramdevice */
	if (SuperQuerySysInfo(SYSINFO_TAG_ROMFSADDR, &romfs, sizeof(uint32)) !=
						SYSINFO_ROMFS_SUPPORTED)
	{
		KBUG(("Couldn't get ROMFS address from SysInfo\n"));
	}
	else
	{
		romfs_size = rom_size - ((uint32)romfs & (rom_size-1));
	}
	KBUG(("ROMFS=0x%x, ROMFS_SIZE=0x%x, ROM_SIZE=0x%x\n", romfs, romfs_size, rom_size));

	/* memory needed is in use */

#ifdef ALLOCATE_RAMDISK
	/* try to allocate a 512kbyte ramdisk */
	RamDevices[DEVUNIT_DRAM].rd_addr = (uint8 *)
		    AllocMemFromMemLists(KernelBase->kb_MemFreeLists,
			64*1024+romfs_size,MEMTYPE_STARTPAGE);

#else
	RamDevices[DEVUNIT_DRAM].rd_addr = 0;
#endif

	if (RamDevices[DEVUNIT_DRAM].rd_addr)
	{
	    RamDevices[DEVUNIT_DRAM].rd_addr += 64*1024;
	    RamDevices[DEVUNIT_DRAM].rd_sizeinbytes = romfs_size;
	}
	else
	{
	    SINFO(("Using SRAM for ramdisk\n"));
	    if ( (KernelBase->kb_CPUFlags & (KB_NODBGR|KB_NODBGRPOOF))
			  == KB_NODBGR )
	    {
	       	RamDevices[DEVUNIT_DRAM].rd_addr = 0;
	       	RamDevices[DEVUNIT_DRAM].rd_sizeinbytes = 0;
	    }
	    else
	    {
	       	RamDevices[DEVUNIT_DRAM].rd_addr = (uint8 *)0x3701000;
	       	RamDevices[DEVUNIT_DRAM].rd_sizeinbytes = 188*1024/BLOCKSIZE;
	    }
	}

	if (RamDevices[DEVUNIT_DRAM].rd_sizeinbytes == 0)
			SINFO(("RamDevice unit:0 disabled\n"));

	/* set up Romdevice */
	if ( (KernelBase->kb_CPUFlags & (KB_NODBGR|KB_NODBGRPOOF))
	      == KB_NODBGR )
	{
		/* no SRAM here, use actual ROM */
#ifdef MEI
	    RamDevices[DEVUNIT_FSROM].rd_addr = romfs;
	    RamDevices[DEVUNIT_ROM].rd_addr = (uint8 *)0x3000000;
	    RamDevices[DEVUNIT_FSROM].rd_sizeinbytes = romfs_size;
	    RamDevices[DEVUNIT_ROM].rd_sizeinbytes = rom_size;
#else
	    RamDevices[DEVUNIT_FSROM].rd_addr = (uint8 *)0x3701000;
	    RamDevices[DEVUNIT_ROM].rd_addr = (uint8 *)0x3700000;
	    RamDevices[DEVUNIT_FSROM].rd_sizeinbytes = 188*1024;
	    RamDevices[DEVUNIT_ROM].rd_sizeinbytes = 256*1024;
#endif
	}
	else
	{
		/* yes SRAM here */
#ifndef MEI
	    RamDevices[DEVUNIT_FSROM].rd_addr = (uint8 *)0x3701000;	/* start of romfs img */
	    RamDevices[DEVUNIT_ROM].rd_addr = (uint8 *)0x3700000;	/* base of rom +13000 for fs */
	    RamDevices[DEVUNIT_FSROM].rd_sizeinbytes = 188*1024;
	    RamDevices[DEVUNIT_ROM].rd_sizeinbytes = 256*1024;
#else
	    RamDevices[DEVUNIT_FSROM].rd_addr = romfs;
	    RamDevices[DEVUNIT_FSROM].rd_sizeinbytes = romfs_size;
	    RamDevices[DEVUNIT_ROM].rd_addr = (uint8 *)0x3000000;
	    RamDevices[DEVUNIT_ROM].rd_sizeinbytes = rom_size;
#endif
	}
	RamDevices[DEVUNIT_NVRAM_1].rd_addr = NVRAM_START;
	RamDevices[DEVUNIT_NVRAM_1].rd_sizeinbytes = NVRAM_SIZE;

	/*
	 * On green platforms (fz1j etc) second (kanji) rom
	 * is located at same address as main rom (0x3000000), and
	 * selected with ADBIO.
	 * On Anvil platforms, second (kanji) rom is located
	 * in Big Trace space (0x3800000)  and selected with an Anvil
	 * feature register bit.
	 */

	retval = SuperQuerySysInfo(SYSINFO_TAG_ROM2BASE, &rom2_baseaddr, sizeof(uint32));
	if(retval != SYSINFO_SUCCESS) {
		KBUG(("In ramInit, no ROM2BASE found, using ROM2_BASE\n"));
		rom2_baseaddr = ROM2_BASE;
	}
	KBUG(("Using 0x%08x as rom2 base\n", rom2_baseaddr));
	RamDevices[DEVUNIT_KANJIROM].rd_addr = (uint8 *)rom2_baseaddr;
	RamDevices[DEVUNIT_KANJIROM].rd_sizeinbytes = ROM2_SIZE;

	/* These should only be visible on an Scientific Atlanta STT */
	if (platform.mfgr == SYSINFO_MFGR_SA)
	{
		RamDevices[DEVUNIT_FLASHROM_1].rd_addr = FLASH_START;
		RamDevices[DEVUNIT_FLASHROM_1].rd_sizeinbytes = FLASH_SIZE;
		RamDevices[DEVUNIT_FLASHROM_2].rd_addr = BOOTFLASH_START;
		RamDevices[DEVUNIT_FLASHROM_2].rd_sizeinbytes = BOOTFLASH_SIZE;
	}

	/* Clio Uncle space */
	{
	    jmp_buf jb;	/* for catching rom aborts */
	    jmp_buf *old_co;
	    uint32 old_quiet;

	    old_co = KernelBase->kb_CatchDataAborts;
	    old_quiet = KernelBase->kb_QuietAborts;
	    KernelBase->kb_CatchDataAborts = &jb;
	    KernelBase->kb_QuietAborts = ABT_CLIOT;
	    if (setjmp(jb) == 0)
	    {	/* try again */
		UncleRegs *wr = (UncleRegs *)UNCLEADDR;
		uint32 id = wr->unc_IdRevBits;
		if ((id & (HardID_MASK | UNC_NOROM)) == UNCLE_ID)
		{
		    	RamDevices[DEVUNIT_UNCLEROM].rd_sizeinbytes = 2*1024*1024;
		}
		wr->unc_SysBits = 0;	/* Set software rev */
		if ( (RamDevices[DEVUNIT_UNCLEROM].rd_sizeinbytes) &&
		     ( strcmp(KernelBase->kb_BootVolumeName,"rom") == 0) )
		{
		    Superkprintf("calling SoftReset for Uncle\n");
		    /* fake a dipir softreset and see if we load new os */

		    Duck();
		    SoftReset();
		    MissedMe();

		    /* It's there but no os, just mount it. */
		}
	    }
	    KernelBase->kb_CatchDataAborts = old_co;
	    KernelBase->kb_QuietAborts = old_quiet;
	}
	for (i = 0; i < MAXUNITS; i++)
	{
	    RamDevices[i].rd_bs = BLOCKSIZE;
	    RamDevices[i].rd_size = RamDevices[i].rd_sizeinbytes/BLOCKSIZE;
	}
	dev->dev_MaxUnitNum = MAXUNITS-1;
	dev->dev.n_Priority = 200;	/* must be first */
	return dev->dev.n_Item;
}

vuint32 lastrom_access_data;
vuint32 *lastrom_access_addr = 0;

static uint8
ReadRomByte(p)
uint8 *p;
{
	uint32 val;
	vuint32 *p2;
	uint32 p3 = (uint32)p;

	p2 = (uint32 *)(p3 & 0xfffffffc);

	if ( lastrom_access_addr == p2 )
	{
	    val = lastrom_access_data;
	}
	else
	{
	    val = *p2;	/* this may abort */
	    lastrom_access_addr = p2;
	    lastrom_access_data = val;
	}

	p3 &= 3;
	RRBDBUG(("rrb: val=%lx p3=%lx ",val,p3));
	if (p3 == 0) val >>= 24;
	else if (p3 == 1) val >>= 16;
	else if (p3 == 2) val >>= 8;
	RRBDBUG((" retval=%lx\n",val));
	return (uint8)(0xff & val);
}

static int
CmdRead(ior)
IOReq *ior;
{
	/* Command Read */
	char *src;
	uint32 retval = 0;
	int32  offset = ior->io_Info.ioi_Offset;
	void *dst = ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	int	unit = ior->io_Info.ioi_Unit;
	RamDev *rd = &RamDevices[unit];

#ifdef DEBUG
	Superkprintf("CmdREAD len=%d",len);
	Superkprintf(" offset=%lx",offset);
	Superkprintf(" unit=%lx\n",unit);
	Superkprintf("baseaddr=%lx size=%d\n",(long)rd->rd_addr,rd->rd_size);
#endif

	if (offset < 0) goto abort;
	if (len == 0) return 1;
	if (len < 0) goto abort;
	offset *= rd->rd_bs;
	if (offset+len > rd->rd_sizeinbytes) goto abort;

	src = rd->rd_addr;

#ifdef DEBUG
	Superkprintf("dst=%lx src=%lx",dst,src);
	Superkprintf(" len=%d\n",len);
#endif

	switch (unit)
	{
	    case DEVUNIT_DRAM:
		    {	/* RAM */
	    		memcpy(dst,src+offset,len);
			break;
		    }
	    case DEVUNIT_FSROM:
	    case DEVUNIT_ROM:
	    case DEVUNIT_KANJIROM:
		    {	/* ROM */
#ifdef KILLDMA
			uint32 oldmctl;
#endif


	    		jmp_buf jb;	/* for catching rom aborts */
			jmp_buf *old_co;
			uint32 old_quiet;
	    		uint8 *pdst;
	    		uint8 *psrc;
	    		volatile int cnt = 0;
			uint32 rombank;
			rombank = SuperQuerySysInfo(SYSINFO_TAG_CURROMBANK,0, 0);
			KBUG(("rombank is 0x%08x\n", rombank));

			if (unit == 5)	/* Enable second ROM bank (kanji) */
			{
				KBUG(("Calling SuperSetSysInfo for second rom bank\n"));
				retval = (SuperSetSysInfo(SYSINFO_TAG_SETROMBANK,
					(void *)SYSINFO_ROMBANK2, 0));
				if (retval)
				{
					KBUG(("SuperSetSysInfo ERROR for second bank\n"));
					KBUG(("retval is 0x%x\n"));
				}
			}
			else if ((unit == 1) || (unit == 2))
			{
				retval = (SuperSetSysInfo(SYSINFO_TAG_SETROMBANK,
					(void *)SYSINFO_ROMBANK1, 0));
				if (retval)
				{
					KBUG(("SuperSetSysInfo ERROR for main bank\n"));
					KBUG(("retval is 0x%x\n"));
				}
			}
			KBUG(("ADBIO contains 0x%x\n", clio->ADBIOBits));
			KBUG(("AnvilFeature contains 0x%x\n",
				madam->AnvilFeature));
			rombank = SuperQuerySysInfo(SYSINFO_TAG_CURROMBANK,0, 0);
			KBUG(("rombank is 0x%08x\n", rombank));

			src += offset;
			old_co = KernelBase->kb_CatchDataAborts;
			old_quiet = KernelBase->kb_QuietAborts;
#ifdef KILLDMA
			oldmctl = *MCTL;
			*MCTL = oldmctl & ~(/*CLUTXEN|*/VSCTXEN);
#endif
		catchabort:
	    		psrc = src + cnt;
	    		pdst = (uint8 *)dst;
	    		pdst += cnt;
	    		KernelBase->kb_CatchDataAborts = &jb;
	    		KernelBase->kb_QuietAborts = ABT_ROMF;
	    		if (setjmp(jb))
	    		{	/* try again */
				goto catchabort;
	    		}

	    		for ( ; cnt < len ; cnt++)
	    		{
				*pdst++ = ReadRomByte(psrc++);
	    		}

	    		KernelBase->kb_CatchDataAborts = old_co;
	    		KernelBase->kb_QuietAborts = old_quiet;
#ifdef KILLDMA
			*MCTL = oldmctl;
#endif
			break;
		    }
	    case DEVUNIT_NVRAM_1:
		    {	/* NVRAM */
#ifdef NVRAM_ABORTS
	    		jmp_buf jb;	/* for catching rom aborts */
			jmp_buf *old_co;
			uint32 old_quiet;
	    		uint8 *pdst;
	    		uint32 *psrc;
	    		volatile int cnt = 0;
			old_co = KernelBase->kb_CatchDataAborts;
			old_quiet = KernelBase->kb_QuietAborts;
	    		src = src + offset*sizeof(*psrc);
		nvcatchabort:
	    		psrc = (uint32 *)src + cnt;
	    		pdst = (uint8 *)dst;
	    		pdst += cnt;
	    		KernelBase->kb_CatchDataAborts = &jb;
	    		KernelBase->kb_QuietAborts = ABT_ROMF;
	    		if (setjmp(jb))
	    		{	/* try again */
				/*if (cnt) cnt--;*/	/* back up by one */
				goto nvcatchabort;
	    		}

	    		for ( ; cnt < len ; cnt++)
	    		{
				*pdst++ = (uint8)(*psrc++);
	    		}

	    		KernelBase->kb_CatchDataAborts = old_co;
	    		KernelBase->kb_QuietAborts = old_quiet;
#else
	    		uint8 *pdst;
	    		uint32 *psrc;
	    		psrc = (uint32 *)src + offset;
	    		pdst = (uint8 *)dst;
	    		while (len--)
	    		{
				*pdst++ = (uint8)(*psrc++);
	    		}
#endif
			break;
		    }
	    case DEVUNIT_UNCLEROM:
		    {	/* Uncle downloadable rom */
			/* Uncle space does not cause aborts */
			ReadUncle((uint32)offset, (uchar *)dst, (int)len);
			break;
		    }

		case DEVUNIT_FLASHROM_1:
		case DEVUNIT_FLASHROM_2:
		{
			uint32 cnt;
			uint8 *pdst=(uint8 *) dst;
			for (cnt = 0; cnt < len; cnt++)
			{
				uint32 noff=(offset+cnt);
				*pdst++ = ReadRomByte( ((noff & ~1) << 1) + 2 + (noff & 1) + (uint32) rd->rd_addr );
			}
			break;
		}
	}
	ior->io_Actual = len;
	return 1;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdWrite(ior)
IOReq *ior;
{
	/* Command Write */
	int32  offset = ior->io_Info.ioi_Offset;
	char *dst;
	char *src = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	int32 len = ior->io_Info.ioi_Send.iob_Len;
	int	unit = ior->io_Info.ioi_Unit;
	RamDev *rd = &RamDevices[unit];

	if (unit == DEVUNIT_NVRAM_1)
        {
            /* Only privileged code can write to NVRAM by talking to
             * the RAM device. Everybody else must be going through the
             * file system.
             *
             * If the callback is set, then the caller has enough privilege.
             * If the callback is not set, check to see if the owner of the
             * I/O req is privileged
             */

	    if (ior->io_CallBack == NULL)
            {
                if ((((Task *)LookupItem(ior->io.n_Owner))->t.n_Flags & TASK_SUPER) == 0)
                {
                    ior->io_Error = BADPRIV;
                    return 1;
                }
	    }
	}

#ifdef DEBUG
	Superkprintf("CmdWrite unit=%ld\n",unit);
	Superkprintf("buffer=0x%lx len=%d\n",src,len);
	Superkprintf("offset=%lx\n",offset);
#endif
	if (offset < 0) goto abort;
	if (len == 0) return 1;
	if (len < 0) goto abort;
	offset *= rd->rd_bs;
	if (offset+len > rd->rd_sizeinbytes) goto abort;

	dst = rd->rd_addr;

	switch (unit)
	{
	    case DEVUNIT_DRAM:
		    {	/* RAM */
	    		memcpy(dst+offset,src,len);
			ior->io_Actual = len;
			break;
		    }
	    case DEVUNIT_FSROM:
	    case DEVUNIT_ROM:
	    case DEVUNIT_UNCLEROM:
	    case DEVUNIT_KANJIROM:
	    	    {
			ior->io_Error = NOSUPPORT;	/* can't write to ROM! */
			break;
		    }
	    case DEVUNIT_NVRAM_1:
		    {	/* NVRAM */
#ifdef NVRAM_ABORTS
	    		jmp_buf jb;	/* for catching rom aborts */
			jmp_buf *old_co;
			uint32 old_quiet;
	    		uint8 *psrc;
	    		uint32 *pdst;
	    		volatile int cnt = 0;
			old_co = KernelBase->kb_CatchDataAborts;
			old_quiet = KernelBase->kb_QuietAborts;
	    		dst = dst + offset*sizeof(*pdst);
		nvcatchabort:
	    		pdst = (uint32 *)dst + cnt;
	    		psrc = (uint8 *)src;
	    		psrc += cnt;
	    		KernelBase->kb_CatchDataAborts = &jb;
	    		KernelBase->kb_QuietAborts = ABT_ROMF;
	    		if (setjmp(jb))
	    		{	/* try again */
				/*if (cnt) cnt--;*/	/* back up by one */
				goto nvcatchabort;
	    		}

	    		for ( ; cnt < len ; cnt++)
	    		{
				*pdst++ = (uint32)(*psrc++);
	    		}

	    		KernelBase->kb_CatchDataAborts = old_co;
	    		KernelBase->kb_QuietAborts = old_quiet;
#else
			uint32 *pdst = (uint32 *)dst;
			pdst += offset;
			ior->io_Actual = len;
			while (len--)
			{
			    *pdst++ = *src++;
			}
#endif
			break;
		    }
	}
	return 1;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdStatus(ior)
IOReq *ior;
{
	ulong  unit = ior->io_Info.ioi_Unit;
	RamDeviceStatus *dst = (RamDeviceStatus *)ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	RamDev *rd = &RamDevices[unit];
	RamDeviceStatus mystat;

	KBUG(("CmdStatus unit=%d dst=%lx len=%d\n",unit,dst,len));


	if (len < 8)	goto abort;

	mystat.ramdev_ds.ds_DriverIdentity = DI_RAM;
	mystat.ramdev_ds.ds_DriverStatusVersion = 0;
	mystat.ramdev_ds.ds_FamilyCode = DS_DEVTYPE_OTHER;
	mystat.ramdev_ds.ds_headerPad = 0;
	mystat.ramdev_ds.ds_MaximumStatusSize = sizeof(DeviceStatus);
	mystat.ramdev_ds.ds_DeviceBlockSize = rd->rd_bs;
	mystat.ramdev_ds.ds_DeviceBlockCount = rd->rd_size;
	mystat.ramdev_ds.ds_DeviceFlagWord = 0;
	if (unit != DEVUNIT_ROM)
	    mystat.ramdev_ds.ds_DeviceUsageFlags = DS_USAGE_FILESYSTEM;
	if (	(unit == DEVUNIT_FSROM) ||
		(unit == DEVUNIT_ROM) ||
		(unit == DEVUNIT_UNCLEROM) ||
		(unit == DEVUNIT_KANJIROM) ||
		(unit == DEVUNIT_FLASHROM_1) ||
		(unit == DEVUNIT_FLASHROM_2))	/* mark ROM units as read only */
		mystat.ramdev_ds.ds_DeviceUsageFlags |= DS_USAGE_READONLY;
	mystat.ramdev_ds.ds_DeviceLastErrorCode = 0;
	mystat.ramdev_DeviceAddress = (uint32)rd->rd_addr;
	if (len > sizeof(RamDeviceStatus)) len = sizeof(RamDeviceStatus);
	memcpy(dst,&mystat,len);
	ior->io_Actual = len;

	KBUG(("BlockSize=%d BlockCount=%d\n",rd->rd_bs,rd->rd_size));

	return 1;
abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int (*CmdTable[])() =
{
	CmdWrite,
	CmdRead,
	CmdStatus,
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"ram",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)myAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)3,
	CREATEDRIVER_TAG_CMDTABLE,	(void *)CmdTable,
	TAG_END,		0,
};

static TagArg devArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	CREATEDEVICE_TAG_DRVR,		0,
	TAG_ITEM_NAME,	"ram",
	CREATEDEVICE_TAG_INIT,	(void *)((long)ramInit),
	TAG_END,		0,
};

Item
createRamDriver(void)
{
	Item drvrItem;
	Item devItem = 0;

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	DBUG(("Creating Ram driver returns drvrItem=%lx\n",drvrItem));
	if (drvrItem >= 0)
	{
		int i;
		devArgs[1].ta_Arg = (void *)drvrItem;
		devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),devArgs);
		/*kprintf("ram-devItem=%lx\n",devItem);*/
		for (i = 0; i < MAXUNITS ; i++)
		DBUG(("start of ramdisk %ld=%lx size=%d\n",i, \
			RamDevices[i].rd_addr,RamDevices[i].rd_size));
	}
	return devItem;
}

