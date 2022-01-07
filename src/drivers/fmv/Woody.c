/*
	File:		Woody.c

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<12>	  8/3/94	GM		Cleared SBOF bit during initialization. This prevents the system
									from hanging if no audio data is written. If this bit is set
									(default after reset), no audio reads ever complete.
		<11>	  7/6/94	GDW		??
		<10>	 4/25/94	BCK		Moved PTS interrupt setup to FMVAV110InitPTSInterrupt() in
									AV110.c .
		 <9>	 3/27/94	GM		Removed FMVReset to prevent Woody lockup due to a clock
									synchronization bug.
		 <8>	 2/28/94	GM		Cleaned up code to avoid compiler warnings.
		 <7>	 2/10/94	GM		Removed redefinition of DEBUGP macro.
		 <6>	  2/7/94	GDW		Replaced resample defines.
		 <5>	  2/7/94	GM		Added function for reading from low CL450 DRAM.
		 <4>	  2/4/94	GDW		Added special compile code for clearing HW PTS interrupts.
		 <3>	11/19/93	GM		Did a shit load of stuff, do not worry.
		 <2>	11/18/93	GM		reconciled with latest pre-projector version

*/

/* file: Woody.c */
/* Woody chip control routines */
/* 3/23/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#include "types.h"
#include "inthard.h"
#include "super.h"

#include "CL450.h"
#include "FMV.h"
#include "Woody.h"
#include "FMVDriver.h"

WoodyRegisterSet *gWoody = (WoodyRegisterSet *) WOODYBASEADDRESS;
uint32 gVendor,gWoodyRevision;

void Spin()
{
	int32 spinner;

	for( spinner = 0L; spinner < 20000L; spinner++ )
		;
}

uint32 FMVReadROM(uint32 address)
{
	WdySetAddress( address );
	WdySetLFSR( address >> 1 );
	return( WdyReadROM() );
}

void FMVWriteAudioRegister(uint32 regNum, uint32 value)
{
	WdySetAddress( regNum );
	WdyWriteAudio( value );
}

uint32 FMVReadAudioRegister(uint32 regNum)
{
	WdySetAddress( regNum );
	return( WdyReadAudio() );
}

void FMVWriteVideoRegister(uint32 regNum, uint32 value)
{
	uint32 regAddress;

	regAddress = (regNum >> 1) | (1 << 10);
	WdySetAddress( regAddress );
	WdyWriteVideo( value );
}

uint32 FMVReadVideoRegister(uint32 regNum)
{
	uint32 regAddress;

	regAddress = (regNum >> 1) | (1 << 10);
	WdySetAddress( regAddress );
	return( WdyReadVideo() );
}

uint32 FMVReadVideoRAM(uint32 address)
{
	address >>= 1L;
	WdySetAddress( address );
	return( WdyReadVideo() );
}

void FMVEnableAudioDMAIn( void )
{
	WdySetControlSet( WDYADMAINENABLE );
}

void FMVDisableAudioDMAIn( void )
{
	WdySetControlClear( WDYADMAINENABLE );
}

void FMVEnableAudioDMAOut( void )
{
	WdySetControlSet( WDYADMAOUTENABLE );
}

void FMVDisableAudioDMAOut( void )
{
	WdySetControlClear( WDYADMAOUTENABLE );
}

void FMVEnableVideoDMAIn( void )
{
	WdySetControlSet( WDYVDMAINENABLE );
}

void FMVDisableVideoDMAIn( void )
{
	WdySetControlClear( WDYVDMAINENABLE );
}

void FMVEnableVideoDMAOut( void )
{
	WdySetControlSet( WDYVDMAOUTENABLE );
}

void FMVDisableVideoDMAOut( void )
{
	WdySetControlClear( WDYVDMAOUTENABLE );
}

void FMVEnableVSInterrupt( void )
{
	WdySetControlSet( WDYINTERRUPTENABLE );
}

void FMVDisableVSInterrupt( void )
{
	WdySetControlClear( WDYINTERRUPTENABLE );
}

int32 FMVCheckVSInterrupt( void )
{
	return( WdyGetStatus() & WDYINTERRUPT);
}

void FMVClearVSInterrupt( void )
{
	WdySetStatus( WDYINTERRUPT );
}

#ifdef DEBUGP
//#undef DEBUGP
//#define DEBUGP(args) Superkprintf args
#endif

extern int32 vsyncRework, flag13;

int32 FMVWoodyInit( void )
{
	uint32 temp;

	DEBUGP(("FMVWoodyInit: gWoody = %08lx\n",(int32) gWoody));

	/* set clio timeout */
	temp = *(SLTIME) | 0x7f0000;
	*(SLTIME) = temp;

	/* see if woody is there */
	temp = WdyReadFMVId();
	if( (temp & WDYFMVIDMSK) != WDYFMVID )
	{
		/* no woody!! */
		DEBUGP(("No Woody!!! FMVId = %08lx, bailing...\n",temp));
		return(-1);
	}
	DEBUGP(("Found Woody!!!!"));

	/* determine vendor */
	if( (gVendor = temp & WDYVENDORMSK) == WDYVENDORTI )
	{
		DEBUGP(("Vendor = TI\n"));
	}
	else
	{
		DEBUGP(("Vendor = LSI\n"));
	}
	/* get revision */
	gWoodyRevision = (temp & WDYFMVREVMSK) >> 12L;

	/* reset entire FMV subsystem */
//	DEBUGP(("fmv reset\n"));
//	WdySetControlSet( WDYRESETFMV );

	if( flag13 )
	{
		WdySetControlSet( WDYC13 );
		DEBUGP(("set 13 MHz\n"));
	}
	else
	{
		WdySetControlClear( WDYC13 );
		DEBUGP(("set 20 MHz\n"));
	}
	// clear SBOF flag
	// this keeps the system from hanging if no audio data is written
	WdySetControlClear( WDYSBOF );

#if MIABUILD == 1
	WdySetControlSet( WDYRESETVIDEO );					// Does a HW reset to Thompson
#endif
	return( 0 );
}

/* set up woody video */
void FMVWoodyVideoInit( int32 pixelMode, int32 resampleMode,
						int32 xOffset, int32 yOffset, int32 xSize, int32 ySize )
{
	extern int32 HPWidth, frontBorder, topBorder, bottomBorder;
	extern int32 VSWidth, backBorder;

	/* turn on color range expansion */
	WdySetControlSet( WDYREX );

	/* set pixels dimensions to transfer to Opera */
	WdySetSize( xSize << 16L |						/* video width */
				ySize );							/* video height */

	if( pixelMode == 16L )
	{
		WdySetControlClear( WDYFORMAT16 );
		WdySetControlClear( WDYFORMAT24 );
		WdySetControlClear( WDYRESAMPLE );
		WdySetVideo1Control( WDYPPHSYNC16 << 16 |		/* set pixels per horizontal sync */
							 topBorder << 8 |			/* top border lines */
							 bottomBorder );				/* bottom border lines */
		WdySetVideo2Control( VSWidth << 24 |			/* vertical pulse width */
							 HPWidth << 16 |			/* horizontal pulse width */
							 frontBorder << 8 |			/* front border width */
							 backBorder );
	}
	else if( pixelMode == 24L )
	{
		WdySetControlSet( WDYFORMAT24 );
		WdySetControlClear( WDYFORMAT16 );
		switch( resampleMode )
		{
			case kCODEC_SQUARE_RESAMPLE:
				WdySetControlClear( WDYRESAMPLE );
				WdySetVideo1Control( WDYPPHSYNC24 << 16 |	/* set pixels per horizontal sync */
									 topBorder << 8 |		/* top border lines */
									 bottomBorder );		/* bottom border lines */
				break;
			case kCODEC_NTSC_RESAMPLE:
				WdySetControlClear( WDYSIZE );
				WdySetControlSet( WDYRESAMPLE );
				WdySetVideo1Control( WDYPPHSYNC24REN << 16 |/* set pixels per horizontal sync */
									 topBorder << 8 |		/* top border lines */
									 bottomBorder );		/* bottom border lines */
				break;
			case kCODEC_PAL_RESAMPLE:
				WdySetControlSet( WDYSIZE );
				WdySetControlSet( WDYRESAMPLE );
				WdySetVideo1Control( WDYPPHSYNC24REN << 16 |/* set pixels per horizontal sync */
									 topBorder << 8 |		/* top border lines */
									 bottomBorder );		/* bottom border lines */
				break;
		}
		WdySetVideo2Control( VSWidth << 24 |			/* vertical pulse width */
							 HPWidth << 16 |			/* horizontal pulse width */
							 frontBorder << 8 |		/* front border width */
							 backBorder );
	}
	/* reset video subsystem */
#if MIABUILD == 0
	WdySetControlSet( WDYRESETVIDEO );
#endif
}

void DumpWoodyRegisters()
{
	DEBUGP(("Woody Registers:\n"));
//	DEBUGP(("       fmvid = %08lx\n",gWoody->fmvid));
//	DEBUGP(("     addrreg = %08lx\n",gWoody->addrreg));
	DEBUGP(("    fmvcntls = %08lx\n",gWoody->fmvcntls));
	DEBUGP(("    fmvcntlc = %08lx\n",gWoody->fmvcntlc));
//	DEBUGP(("     fmvstat = %08lx\n",gWoody->fmvstat));
	DEBUGP(("     sizereg = %08lx\n",gWoody->sizereg));
	DEBUGP(("     vid1reg = %08lx\n",gWoody->vid1reg));
	DEBUGP(("     vid2reg = %08lx\n",gWoody->vid2reg));
//	DEBUGP(("     lfsrreg = %08lx\n",gWoody->lfsrreg));
}

#include "CL450.h"
#define PVIDREG(a,b)	{ temp = FMVReadVideoRegister( a ); DEBUGP(("    %s = %08lx\n",b,temp)); }

void DumpVideoRegisters()
{
	int32 temp;

	DEBUGP(("CL450 Registers\n"));
	PVIDREG(HOST_CONTROL,"HOST_CONTROL");
	PVIDREG(CPU_CONTROL,"CPU_CONTROL");
}
