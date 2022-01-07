/* $Id: dspp_touch_anvil.c,v 1.6 1994/10/18 19:04:56 peabody Exp $ */
/*******************************************************************
**
** Interface to DSPP
**
** by Phil Burk
**
** Copyright (c) 1992 New Technologies Group, Inc.
**
*******************************************************************/

/*
** 930111 PLB Set WRDCLK to #$0
** 930428 PLB Added DisableDMAChannel() and check for NextAddr = 0
** 930521 PLB Change Hardware to DAC_ControlValue that is passed in.
** 930713 PLB Initialize EI memory (for dipir scaling romtail.dsp )
** 931222 PLB DSPP_SetFullDMA()
** 940126 PLB Remove some annoying messages.
** 940321 PLB Fixed pointer arithmetic bug that was masked by compiler bug.
**	          DMAPtr = (vuint32 *) (RAMtoDSPP0 + (DMAChan<<2)); BAD!!!!
** 940606 PLB Fixed roundoff error in CvtBytesToFrame() thet was causing the
**            number of bytes and frames to disagree.  501 => 500.
** 940810 PLB Clear interrupt using proper mask so that Output DMA
**            works.  This was causing a premature termination of
**            Output DMA if MonitorAttachment() was called.
** 940901 PLB Removed AUDOUT setup code.  Now done in ROM.
**            Split from dspp_touch.c.
** 940912 PLB Use register declaration to handle int16 values for NuPup
*/

#include "audio_internal.h"
#include "touch_hardware.h"

#define DBUG(x) /* PRT(x) */
#define DBUGDMA(x) /* PRT(x) */
#define NDBUG(x) /* */

#ifdef ASIC_ANVIL

static void dsphSendWriteIMem( int32 WriteAddr, int32 WriteValue, int32 ReadAddr );
static Err dsphWaitSemaphore( int32 *ReadValuePtr );


/****************************************
** Source for startup code for Opera.
	AUDLOCK _A     #$ 8000 _MOVE
	WCLKRLD _A     #$ 0000 _MOVE
	OutputLeft _A  #$ 0000 _MOVE
	OutputRight _A #$ 0000 _MOVE
	I_MixLeft _A   #$ 0000 _MOVE
	I_MixRight _A  #$ 0000 _MOVE
	_SLEEP
	_NOP
	_NOP
****************************************/

uint16 StartupCode[]  =  /* Initial DSP code from startup.ins */
{
  0x9beb , /* 0x0000 */
  0xf000 , /* 0x0001 */
  0x9bef , /* 0x0002 */
  0xc000 , /* 0x0003 */
  0x9bfe , /* 0x0004 */
  0xc000 , /* 0x0005 */
  0x9bff , /* 0x0006 */
  0xc000 , /* 0x0007 */
  0x9906 , /* 0x0008 */
  0xc000 , /* 0x0009 */
  0x9907 , /* 0x000a */
  0xc000 , /* 0x000b */
  0x8380 , /* 0x000c */
  0x8000 , /* 0x000d */
  0x8000 , /* 0x000e */
};

void dsphDownloadStartupCode( void )
{
	DSPPDownloadCode (StartupCode, 0, sizeof(StartupCode)/sizeof(uint16));
}

/*******************************************************************
** Reset and clear FIFO
*******************************************************************/
void dsphResetFIFO( int32 DMAChan )
{
		WriteHardware(FIFOINIT, (1L << DMAChan) );  /* Completely clear FIFO before starting. */
	
/* 931116 Clear head of FIFO in EI memory to reduce pops. */
/* 931216 Check for input DMA Type. */
		if( IsDMATypeInput(DMAChan) )
		{
			DBUG(("Clear FIFOHEAD[%d]\n", DMAChan));
			dsphWriteEIMem( DSPP_FIFOHEAD_ZERO_ADDRESS + DMAChan, 0 );
		}

}

/*******************************************************************/
void dsphResetAllFIFOs( void )
{
	WriteHardware(FIFOINIT, (((1L << NUM_AUDIO_INPUT_DMAS) - 1) << DRD0_CHANNEL) );	/* Initialize input FIFOs. */
	WriteHardware(FIFOINIT, (((1L << NUM_AUDIO_OUTPUT_DMAS) - 1) << DDR0_CHANNEL) );	/* Initialize output FIFOs. */
}

/*******************************************************************/
void dsphReset( void )
{
	WriteHardware(DSPPRST1,0);
}

/*******************************************************************/
void dsphHalt( void )
{
	WriteHardware(DSPPGW,0);
}

/*******************************************************************/
void dsphStart( void )
{
	WriteHardware(DSPPGW, DSPP_GWILL);
}

/*******************************************************************/
void dsphInitDMA( void )
{
	int32 i;
	
	for (i=0; i<NUM_AUDIO_INPUT_DMAS; i++)
	{
		DSPP_SilenceDMA(i + DRD0_CHANNEL );
	}
	
	for (i=0; i<NUM_AUDIO_OUTPUT_DMAS; i++)
	{
		DSPP_SilenceDMA(i + DDR0_CHANNEL );
	}
}

/***************************************************************/
uint32 dsphConvertChannelToInterrupt( int32 DMAChan )
{
	if(DMAChan < DDR0_CHANNEL)
	{
		return (INT_DRD0 + DMAChan);
	}
	else
	{
		return (INT_DSPPRAM0 + DMAChan - DDR0_CHANNEL);
	}
}

/***************************************************************/
void dsphEnableChannelInterrupt( int32 DMNAChan )
{
	EnableInterrupt( dsphConvertChannelToInterrupt(DMNAChan) );
}

/***************************************************************/
void dsphDisableChannelInterrupt( int32 DMNAChan )
{
	DisableInterrupt( dsphConvertChannelToInterrupt(DMNAChan) );
}

/*******************************************************************/
void dsphSetDMANext (int32 DMAChan, int32 *NextAddr, int32 NextCnt)
{
	register vuint32 *DMAPtr;
	int32  DMAMask;
	uint32 IntMask;
	
	DBUGDMA(("dsphSetDMANext(0x%x, 0x%x, 0x%x)\n", DMAChan, NextAddr,  NextCnt));
#ifdef PARANOID
	if(NextAddr == 0)
	{
		ERR(("dsphSetDMANext: NextAddr = 0\n"));
		return;
	}
	if(NextCnt < 0)
	{
		
		ERR(("dsphSetDMANext: NextCnt = 0x%x\n", NextCnt));
		return;
	}
#endif

	DMAPtr = RAMtoDSPP0 + DMANextAddress + (DMAChan<<2);
	
/* Calculate proper Interrupt Mask from DMAChan 940810 */
	IntMask = (uint32)1 << dsphConvertChannelToInterrupt(DMAChan);

/* Disable DMA requests so we don't loop while setting registers! */
	DMAMask = 1L << DMAChan;
	dsphDisableDMA( DMAChan );

/* Clear interrupt bit from previous DMA %Q This is scary! */
	WriteHardware( ClrInt0Bits, IntMask ); /* Use proper mask. 940810 */

	WriteHardware(DMAPtr++, (int32) NextAddr);
	WriteHardware(DMAPtr, NextCnt);
	
/* Enable DMA requests. */
	WriteHardware(DMAREQEN, DMAMask );
}

/*******************************************************************/
/* Set all 4 registers of DMA as quickly as possible. */
void dsphSetFullDMA (int32 DMAChan, int32 *Addr, int32 Cnt, int32 *NextAddr, int32 NextCnt )
{
	vuint32 *DMAPtr;
	int32  DMAMask;
	uint32 IntMask;
	
	DBUGDMA(("dsphSetFullDMA( 0x%x, 0x%x, 0x%x, 0x%x, 0x%x )\n",
		DMAChan, Addr, Cnt, NextAddr, NextCnt ));
		
#ifdef PARANOID
	if(NextAddr == 0)
	{
		ERR(("dsphSetFullDMA: NextAddr = 0"));
		return;
	}
	if(NextCnt < 0)
	{
		
		ERR(("dsphSetFullDMA: Length = 0x%x\n", NextCnt ));
		return;
	}

#endif

	DMAPtr = RAMtoDSPP0 + (DMAChan<<2);
	
/* Calculate proper Interrupt Mask from DMAChan 940810 */
	IntMask = (uint32)1 << dsphConvertChannelToInterrupt(DMAChan);

/* Disable DMA requests so we don't loop while setting registers! */
	DMAMask = 1L << DMAChan;
	dsphDisableDMA( DMAChan );
	
/* Clear interrupt bit from previous DMA */
	WriteHardware( ClrInt0Bits, IntMask ); /* Use proper mask. 940810 */

/* Write Addr, Count, NextAddr, NextCount to DMA */
	WriteHardware(DMAPtr++, (int32) Addr);
	WriteHardware(DMAPtr++, Cnt);
	WriteHardware(DMAPtr++, (int32) NextAddr);
	WriteHardware(DMAPtr,   NextCnt);
	
/* Enable DMA requests. */
	WriteHardware(DMAREQEN, DMAMask );
	
}

/*******************************************************************/
void dsphDisableDMA( int32 DMAChan )
{
	vuint32 *DMAPtr;
	uint32 DMAMask;
	
	DMAPtr = RAMtoDSPP0 + (DMAChan<<2);
	
/* Disable DMA requests so we don't loop while setting registers! */
	DMAMask = 1L << DMAChan;
	WriteHardware(DMAREQDIS, DMAMask );

/* Wait for DMA to settle. */
	while( *DMAPtr != *DMAPtr)
	{
		DBUG(("dsphDisableDMA: DMA not stopped!\n"));
	}
}


/*****************************************************************/
/*** DSPP I Memory Access ****************************************/
/*****************************************************************/

int32 DSPP_InitIMemAccess( void )
{
	int32 Alloc, Result;
	
	Result = DSPGetRsrcAlloc( gTailInstrumentItem, DRSC_EI_MEM, DSPP_WRITE_KNOB_NAME, &Alloc);
	if( Result < 0)
	{
		ERR(("DSPP_InitIMemAccess: tail.dsp is stale!\n"));
		return Result;
	}
	
	DSPPData.dspp_MemWriteEI = Alloc;
DBUG(("dspp_MemWriteEI = %d\n", DSPPData.dspp_MemWriteEI));

	Result = DSPGetRsrcAlloc( gTailInstrumentItem, DRSC_EI_MEM, DSPP_READ_KNOB_NAME, &Alloc);
	if( Result < 0) return Result;
	DSPPData.dspp_MemReadEI = Alloc;
DBUG(("dspp_MemReadEI = %d\n", DSPPData.dspp_MemReadEI));

	DSPPData.dspp_IMEMAccessOn = TRUE;

	dsphSendWriteIMem( 0, 0, 0 );
	
	return Result;
}

#define SEMAPHORE_DSPP_WROTE (0x40000)
#define DSPP_FRAME_TIMEOUT   (22*12*100)  /* 12 MIPS * 22 useconds * 100 frames */

static void dsphSendWriteIMem( int32 WriteAddr, int32 WriteValue, int32 ReadAddr )
{
	dsphWriteEIMem( DSPPData.dspp_MemWriteEI, WriteAddr );
	dsphWriteEIMem( DSPPData.dspp_MemReadEI, ReadAddr );
	WriteHardware( SEMAPHORE, WriteValue );
}

static Err dsphWaitSemaphore( int32 *ReadValuePtr )
{
	int32 Result = 0, i;
	uint32 SemaphoreValue;
	int32 TimedOut = TRUE;
	register int16 ShortVal;
	
	TimedOut = TRUE;
	for(i=0; i<DSPP_FRAME_TIMEOUT; i++)
	{
		SemaphoreValue = ReadHardware(SEMAPHORE);
		if(SemaphoreValue & SEMAPHORE_DSPP_WROTE)
		{
			TimedOut = FALSE;
			if( ReadValuePtr )
			{
				ShortVal = (int16) ReadHardware(SEMAPHORE);
				*ReadValuePtr = (int32) ShortVal; /* Sign extend. */
			}
			
			break;
		}
	}
	if(TimedOut)
	{
		ERR(("dsphWaitSemaphore: Timeout\n"));
		Result = AF_ERR_TIMEOUT;
	}
	return Result;
}

/*******************************************************************
** Write to parameter memory that can be read by DSPP
*******************************************************************/
void dsphWriteEIMem( int32 DSPPAddr, int32 Value )
{
#ifdef PARANOID
	if((DSPPAddr < 0) ||
		(DSPPAddr > 127))
	{
		ERR(("dsphWriteEIMem: EI address out of range = 0x%x\n", DSPPAddr));
		return;
	}
#endif

	WriteHardware(DSPPEI16 + DSPPAddr, Value);

}
/*****************************************************************/
int32 dsphWriteIMem( int32 WriteAddr, int32 WriteValue )
{
	int32 Result = 0;
	
DBUG(("dsphWriteIMem( 0x%x, 0x%x )\n", WriteAddr, WriteValue));

	Result = dsphWaitSemaphore( NULL );
	if( Result == 0) dsphSendWriteIMem( WriteAddr, WriteValue, 0);
	
	return Result;
}

/*****************************************************************/
int32 dsphReadIMem( int32 ReadAddr, int32 *ValuePtr )
{
	int32 Result = 0;
	
DBUG(("dsphReadIMem( 0x%x, 0x%x )\n", ReadAddr, ValuePtr));

	Result = dsphWaitSemaphore( NULL );
	if( Result == 0)
	{
		dsphSendWriteIMem( 0, 0, ReadAddr);
		Result = dsphWaitSemaphore( ValuePtr );
	}
	return Result;
}

/*****************************************************************/
/* ReadAddr is relative to base of DSPI data memory. */
int32 dsphReadEOMem( int32 ReadAddr, int32 *ValuePtr )
{
	int32 Result = 0;
	int32 OffsetEO;
DBUG(("dsphReadEOMem( ReadAddr = 0x%x, ValuePtr = 0x%x )\n", ReadAddr, ValuePtr ));

#ifdef PARANOID
	if((ReadAddr < 0x300) ||
		(ReadAddr > 0x30F))
	{
		ERR(("dsphReadEOMem: EO address out of range = 0x%x\n", ReadAddr));
		return;
	}
#endif
	OffsetEO = ReadAddr - 0x300;
	*ValuePtr = (int32) ReadHardware( DSPPEO16 + OffsetEO );

	return Result;
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getaudioframecount
 |||	GetAudioFrameCount - !!!
 |||
 |||	  Synopsis
 |||
 |||	    uint16 GetAudioFrameCount (void)
 |||
 |||	  Description
 |||
 |||	    !!! description!
 |||
 |||	  Arguments
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    !!!
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    GetAudioCyclesUsed()
 |||
 **/
uint16 dsphGetAudioFrameCount( void )
{
	return (int32) ReadHardware( DSPPEO16 + EO_FRAMECOUNT );
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getaudiocyclesused
 |||	GetAudioCyclesUsed - !!!
 |||
 |||	  Synopsis
 |||
 |||	    int32 GetAudioCyclesUsed (void)
 |||
 |||	  Description
 |||
 |||	    !!! description!
 |||
 |||	  Arguments
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    !!!
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    GetAudioFrameCount()
 |||
 **/
int32 dsphGetAudioCyclesUsed( void )
{
	return (int32) ReadHardware( DSPPEO16 + EO_BENCHMARK );
}

#ifdef COMPILE_DSPP_TRACE
/* Use single step to trace execution of DSPP through one cycle. */
Err dsphTraceExecution( void )
{
	int32 i,j;
	
/* Patch Head to set WCLKRLD to zero to turn off reset from clock. */
	PatchDSPPCode( 2, 0xC000);
	PatchDSPPCode( 1, 0x9BEF);
/* Kill time waiting for DSPP to execute complete Frame, 565 ticks. */
	for( i=0; i<1000; i++);
/* Patch Head to set AUDLOCK to zero to turn off reset from DAC. */
	PatchDSPPCode( 2, 0xC000);
	PatchDSPPCode( 1, 0x9BEB);
/* Kill time waiting for DSPP to execute complete Frame, 565 ticks. */
	for( i=0; i<1000; i++);
	
/* Turn off DSPP execution. */
	WriteHardware( DSPPGW, 0 );
/* Reset Processor. */
	WriteHardware(DSPPRST1,0);
	
	for( j=0; j<565; j++)
	{
		WriteHardware( DSPPGW, 2 );
/* Kill time waiting for DSPP to execute advance PC */
		for( i=0; i<4; i++);
		PRT(("PC = 0x%x, NR = 0x%x\n",
			ReadHardware( DSPPPC ) - 1, ReadHardware( DSPPNR ) ));
/* Stop at SLEEP */
		if( ReadHardware( DSPPNR ) == 0x8380 ) break;
	}
	
/* Restore Head */
	PatchDSPPCode( 1, 0x9904);
	PatchDSPPCode( 2, 0x80ef);
/* Reset Processor. */
	WriteHardware(DSPPRST1,0);
/* Turn ON DSPP execution. */
	WriteHardware( DSPPGW, DSPP_GWILL );
	return 0;
}
#endif /* COMPILE_DSPP_TRACE */

#endif /* ASIC_ANVIL */
