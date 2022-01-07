/*
	File:		Woody.h

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<10>	  4/6/94	BCK		Added WDYVIDEOREADFIFOSIZE, WDYVIDEOWRITEFIFOSIZE,
									WDYAUDIOREADFIFOSIZE , WDYAUDIOWRITEFIFOSIZE.
		 <9>	  4/5/94	BCK		Moved masking from WdyReadAudio() to AV110.h
									FMVAV110ReadRegister() macro.
		 <8>	  4/4/94	BCK		WdyReadAudio() now masks the read register with 0xff since only
									the low eight bits are valid.
		 <7>	 3/17/94	GDW		Changed FMVID mask.
		 <6>	  3/8/94	GM		Changed declaration of the Woody register set structure to be
									volatile so that the compiler won't optimize away any accesses.
		 <5>	  2/7/94	GDW		Removed resample defines.
		 <4>	  2/7/94	GM		Added FMVReadVideoRAM function prototype.
		 <3>	11/19/93	GM		More George fix stuff.
		 <2>	11/18/93	GM		reconciled with latest pre-projector version

*/

/* file: Woody.h */
/* Woody chip definitions */
/* 3/23/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

/* include but once */
#ifndef WOODY_HEADER
#define WOODY_HEADER

#define WOODYBASEADDRESS	0x0340c000L

#ifndef THINK_C

#include "types.h"

typedef						/* Woody registers */
	volatile struct
	{
		uint32	fmvid;		/* FMV identification register (read) */
		uint32	rsrvdField;	/* returns 0 when read */
		uint32	addrreg;	/* address register (write) */
		uint32	romreg;		/* ROM read register */
		uint32	fmvcntls;	/* FMV control set register (r/w) */
		uint32	fmvcntlc;	/* FMV control clear register (r/w) */
		uint32	audreg;		/* Audio read/write register */
		uint32	vidreg;		/* Video read/write register */
		uint32	sizereg;	/* Size register (r/w) */
		uint32	vid1reg;	/* Video control register 1 (r/w) */
		uint32	vid2reg;	/* Video control register 2 (r/w) */
		uint32	lfsrreg;	/* LFSR register (r/w) */
		uint32	fmvstat;	/* FMV status register (r/w) */
		uint32	dither1;	/* Dither register 1 (r/w) */
		uint32	dither2;	/* Dither register 2 (r/w) */
	}
	WoodyRegisterSet;
	
#define WdyReadFMVId()			(gWoody->fmvid)
#define WdySetAddress(a)		(gWoody->addrreg = (uint32) (a))
#define WdyReadROM()			(gWoody->romreg)
#define WdySetControlSet(cs)	(gWoody->fmvcntls = (uint32) (cs))
#define WdyGetControlSet()		(gWoody->fmvcntls)
#define WdySetControlClear(cs)	(gWoody->fmvcntlc = (uint32) (cs))
#define WdyGetControlClear()	(gWoody->fmvcntlc)
#define WdySetStatus(s)			(gWoody->fmvstat = (uint32) (s))
#define WdyGetStatus()			(gWoody->fmvstat)
#define WdyWriteAudio(a)		(gWoody->audreg = (uint32) (a))
#define WdyReadAudio()			(gWoody->audreg)
#define WdyWriteVideo(v)		(gWoody->vidreg = (uint32) (v))
#define WdyReadVideo()			(gWoody->vidreg)
#define WdySetSize(s)			(gWoody->sizereg = (uint32) (s))
#define WdyGetSize()			(gWoody->sizereg)
#define WdySetVideo1Control(v)	(gWoody->vid1reg = (uint32) (v))
#define WdyGetVideo1Control()	(gWoody->vid1reg)
#define WdySetVideo2Control(v)	(gWoody->vid2reg = (uint32) (v))
#define WdyGetVideo2Control()	(gWoody->vid2reg)
#define WdySetLFSR(l)			(gWoody->lfsrreg = (uint32) (l))
#define WdyGetLFSR()			(gWoody->lfsrreg)
#define WdySetDither1(dv)		(gWoody->dither1 = (uint32) (dv))
#define WdyGetDither1(dv)		(gWoody->dither1)
#define WdySetDither2(dv)		(gWoody->dither2 = (uint32) (dv))
#define WdyGetDither2(dv)		(gWoody->dither2)

#else

#include <stdio.h>
#include "myTypes.h"

typedef long WoodyRegisterSet;

extern FILE *debugFile;

#define WdySetAddress(a)		{ fprintf(debugFile,"0x%08lx ",(uint32) (a)); fflush(debugFile); }
#define WdyWriteAudio(a)		{ fprintf(debugFile," 0x%08lx wa\n",(uint32) (a)); fflush(debugFile); }
#define WdyReadAudio()			(fprintf(debugFile," ra\n"),fflush(debugFile), 0L)
#define WdyWriteVideo(v)		{ fprintf(debugFile,"0x%08lx wv\n",(uint32) (v)); fflush(debugFile); }
#define WdyReadVideo()			(fprintf(debugFile," rv\n"), fflush(debugFile) , 0L)
#define WdyReadROM()			( 0L )

#endif

extern WoodyRegisterSet *gWoody;	/* = WOODYBASEADDRESS */
extern uint32 gVendor,gWoodyRevision;

/* fmv identification register definitions */
#define WDYFMVID			0x03000000L		/* woody identification register */
#define WDYFMVIDMSK			0xff000000L		/* mask for above */
#define WDYFMVTYPEMSK		0x00ff0000L		/* mask for type field */
#define WDYFMVREVMSK		0x0000F000L		/* mask for revision field */
#define WDYVENDORMSK		0x01L			/* mask for vendor ID */
#define WDYVENDORLSI		0x00L			/* LSI L64111 audio decoder */
#define WDYVENDORTI			0x01L			/* TI TMS320AV110 audio decoder */

/* fmv control set/clear registers definitions */
#define WDYC13				0x02000000L		/* generate 13MHz pixel clock */
#define WDYSFAH				0x01000000L		/* skip first audio halfword */
#define WDYSLAH				0x00800000L		/* skip last audio halfword */
#define WDYSFVH				0x00400000L		/* skip first video halfword */
#define WDYSLVH				0x00200000L		/* skip last video halfword */
#define WDYSBOF				0x00100000L		/* sync to beginning of frame */
#define WDYGPIO2			0x00080000L		/* gpio 2 bit */
#define WDYGPIO1			0x00040000L		/* gpio 1 bit */
#define WDYGPIO0			0x00020000L		/* gpio 0 bit */
#define WDYGPIODIR2			0x00010000L		/* gpio 2 direction */
#define WDYGPIODIR1			0x00008000L		/* gpio 1 direction */
#define WDYGPIODIR0			0x00004000L		/* gpio 0 direction */
#define WDYNAD				0x00002000L		/* no auto disable */
#define WDYREX				0x00001000L		/* range expansion enable */
#define WDYSIZE				0x00000800L		/* resize ratio */
#define WDYRESAMPLE			0x00000400L		/* resampler enable */
#define WDYFORMAT16			0x00000100L		/* 16 bit dithered output */
#define WDYFORMAT24			0x00000200L		/* 24 bit output */
#define WDYVDMAINENABLE		0x00000080L		/* video DMA in enable */
#define WDYVDMAOUTENABLE	0x00000040L		/* video DMA out enable */
#define WDYADMAINENABLE		0x00000020L		/* audio DMA in enable */
#define WDYADMAOUTENABLE	0x00000010L		/* audio DMA out enable */
#define WDYRESETVIDEO		0x00000008L		/* reset video decoder */
#define WDYRESETAUDIO		0x00000004L		/* reset audio decoder */
#define WDYRESETFMV			0x00000002L		/* reset entire FMV subsystem */
#define WDYINTERRUPTENABLE	0x00000001L		/* enable woody interrupt */
#define WDYCLEARCNTLS		0xffffffffL		/* clear bits */

/* video register settings */
//#define WDYBORDERLEFT		0x0aL			/* 10 pixel left border */
#define WDYBORDERTOP		0x0aL			/* 10 pixel top border */
#define WDYRBORDERCOLOR		0x00L			/* red component of border color */
#define WDYGBORDERCOLOR		0x00L			/* green component of border color */
#define WDYBBORDERCOLOR		0x00L			/* blue component of border color */
#define WDYXOFFSET			0x00L			/* 0 pixel x offset into decoded picture */
#define WDYYOFFSET			0x00L			/* 0 pixel y offset into decoded picture */
#define WDYWINDOWWIDTH		320L			/* window into picture width */
#define WDYWINDOWHEIGHT		240L			/* window into picture height */

#define WDYHINT				0x80000000L		/* horizontal interpolate on */
#define WDYPPHSYNC16		128L			/* pixels per horizontal sync */
#define WDYPPHSYNC24		640L			/* pixels per horizontal sync */
//#define WDYPPHSYNC24REN		572L			/* TEST pixels per horizontal sync with resampling */
#define WDYPPHSYNC24REN		704L			/* pixels per horizontal sync with resampling */
#define WDYTBLINES16		10L				/* top border lines */
#define WDYTBLINES24		30L				/* top border lines */
#define WDYBBLINES16		10L				/* bottom border lines */
#define WDYBBLINES24		10L				/* bottom border lines */

#define WDYVPWIDTH			((uint32) 255L)	/* vertical sync pulse width */
#define WDYHPWIDTH			90L				/* horizontal sync pulse width */
#define WDYFBORDER16		102L			/* front border size */
#define WDYFBORDER24		102L			/* front border size */
#define WDYBBORDER16		10L				/* back border size */
#define WDYBBORDER24		10L				/* back border size */

/* status register */
#define WDYINTERRUPT		0x01L			/* woody interrupt status/clear bit */

/* FIFO sizes */
#define WDYVIDEOREADFIFOSIZE	256			/* woody video read FIFO size in bytes */
#define WDYVIDEOWRITEFIFOSIZE	16			/* woody video write FIFO size in bytes */
#define WDYAUDIOREADFIFOSIZE	16			/* woody audio read FIFO size in bytes */
#define WDYAUDIOWRITEFIFOSIZE	16			/* woody audio write FIFO size in bytes */

/* function prototypes */

uint32	FMVReadROM(uint32 address);
void	FMVWriteAudioRegister(uint32 regNum, uint32 value);
uint32	FMVReadAudioRegister(uint32 regNum);
void	FMVWriteVideoRegister(uint32 regNum, uint32 value);
uint32	FMVReadVideoRegister(uint32 regNum);
uint32	FMVReadVideoRAM(uint32 address);
void FMVEnableAudioDMAIn( void );
void FMVDisableAudioDMAIn( void );
void FMVEnableAudioDMAOut( void );
void FMVDisableAudioDMAOut( void );
void FMVEnableVideoDMAIn( void );
void FMVDisableVideoDMAIn( void );
void FMVEnableVideoDMAOut( void );
void FMVDisableVideoDMAOut( void );
void FMVEnableVSInterrupt( void );
void FMVDisableVSInterrupt( void );
int32 FMVCheckVSInterrupt( void );
void FMVClearVSInterrupt( void );

int32 	FMVWoodyInit( void );
void FMVWoodyVideoInit( int32 pixelMode, int32 resampleMode, int32 xOffset, int32 yOffset, int32 xSize, int32 ySize );
void DumpWoodyRegisters( void );
void DumpVideoRegisters( void );

#endif

