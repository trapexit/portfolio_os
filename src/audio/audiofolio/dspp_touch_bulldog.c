/* $Id: dspp_touch_bulldog.c,v 1.1 1994/09/08 21:05:15 phil Exp $ */
/*******************************************************************
**
** Interface to DSPP for Bulldog
**
** by Phil Burk
**
** Copyright (c) 1992 New Technologies Group, Inc.
** Copyright (c) 1994 3DO
**
*******************************************************************/

/*
** 940901 PLB Removed AUDOUT setup code.  Now done in ROM.
**            Split from dspp_touch.c.
*/

#include "audio_internal.h"
#include "touch_hardware.h"

#define DBUG(x) /* PRT(x) */
#define DBUGDMA(x) /* PRT(x) */
#define NDBUG(x) /* */

#ifdef ASIC_BULLDOG

/*******************************************************************/
/* DSPP static data. */

short *Silence;      /* Zero data to play silence. */
short *ScratchRAM;   /* Scratch target of output DMA. */

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
  0x9b!!!eb , /* 0x0000 */
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

/*******************************************************************
** Address calculations
*******************************************************************/
#define dsphAddressDSPI(DSPPAddress) (DSPX_DATA_MEMORY + (DSPPAddress))
					
#define dsphAddressFIFO_OSC(DMAChan) \
	dsphAddressDSPI(DSPI_FIFO_OSC + (DMAChan*DSPI_FIFO_OSC_SIZE) )
					
/*******************************************************************
** Write to parameter memory that can be read by DSPP
*******************************************************************/
void dsphWriteEIMem( int32 DSPPAddr, int32 Value )
{
#ifdef PARANOID
	if((DSPPAddr < (FIRST_EI_MEM + OFFSET_EI_MEM) ||
		(DSPPAddr > (LAST_EI_MEM + OFFSET_EI_MEM))
	{
		ERR(("dsphWriteEIMem: EI address out of range = 0x%x\n", DSPPAddr));
		return;
	}
#endif

	PutHard(DSPX_DATA_MEMORY+DSPPAddr, val);

}

/*******************************************************************
** Write to parameter memory that can be read by DSPP
*******************************************************************/
void dsphWriteIMem( int32 DSPPAddr, int32 Value )
{
#ifdef PARANOID
	if((DSPPAddr < (FIRST_I_MEM + OFFSET_I_MEM) ||
		(DSPPAddr > (LAST_I_MEM + OFFSET_I_MEM))
	{
		ERR(("dsphWriteIMem: I address out of range = 0x%x\n", DSPPAddr));
		return;
	}
#endif

	PutHard(DSPX_DATA_MEMORY+DSPPAddr, val);

}


/*******************************************************************
** Reset and clear FIFO
*******************************************************************/
void dsphResetFIFO( int32 DMAChan )
{
/* Reset FIFO */
	WriteHardware( DSPX_CHANNEL_RESET, (1L << DMAChan) );  /* Completely clear FIFO before starting. */

/* Clear data at head of FIFO */
	WriteHardware( (dsphAddressFIFO_OSC(DMAChan) + DSPI_FIFO_CURRENT_OFFSET), 0 );
	WriteHardware( (dsphAddressFIFO_OSC(DMAChan) + DSPI_FIFO_NEXT_OFFSET), 0 );
	
}

/*******************************************************************/
void dsphResetAllFIFOs( void )
{
	int32 i;
	
	for( i=0; i<DSPI_NUM_CHANNELS; i++ ) dsphResetFIFO(i);

}

/*******************************************************************/
void dsphReset( void )
{
	WriteHardware(DSPX_RESET,-1);  /* -1 resets everything. */
	return(0);
}

/*******************************************************************/
void dsphHalt( void )
{
	WriteHardware( DSPX_CONTROL, DSPXB_SNOOP );
	return(0);
}

/*******************************************************************/
void dsphStart( void )
{
	WriteHardware( DSPX_CONTROL, DSPXB_SNOOP|DSPXB_GWILLING );
	return(0);
}

void dsphConfigureChannel( int32 Channel, int32 Direction, int32 If8Bit, int32 IfSQXD )
{
	int32 Mask;
	
	Mask = 1 << Channel;
	PRT(("dsphConfigureChannel UNIMPLEMENTED\n"));
}

/*******************************************************************/
void dsphInitDMA( void )
{
	int32 i
	uint32 Mask;
	
	WriteHardware( DSPX_CHANNEL_DISABLE, -1 );
	WriteHardware( DSPX_CHANNEL_DIRECTION_CLR, -1 );
	WriteHardware( DSPX_CHANNEL_8BIT_CLR, -1 );
	WriteHardware( DSPX_CHANNEL_SQXD_CLR, -1 );
	
	dsphResetAllFIFOs();

/* Set direction for Output FIFOs %Q for fixed allocation kludge */
	Mask = -1 << OFFSET_OUT_FIFO;
	WriteHardware( DSPX_CHANNEL_DIRECTION_SET, Mask );
}

/***************************************************************/
uint32 dsphConvertChannelToInterrupt( int32 DMAChan )
{
	return (1 << DMAChan)
}

/***************************************************************/
void dsphEnableChannelInterrupt( int32 DMNAChan )
{
	WriteHardware( DSPX_INT_DMANEXT_SET, dsphConvertChannelToInterrupt(DMNAChan) );
}

/***************************************************************/
void dsphDisableChannelInterrupt( int32 DMNAChan )
{
	WriteHardware( DSPX_INT_DMANEXT_CLR, dsphConvertChannelToInterrupt(DMNAChan) );
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

	DMAPtr = DSPX_DMA_STACK + DSPX_DMA_NEXT_COUNT_OFFSET + (DMAChan<<2);

/*
** Disable all DMA interrupts so we don't interrupt while using shadow!
** Save the current enables.
*/
	SaveInt = ReadHardware( DSPX_INT_SET, DSPXB_INT_ALL_DMA );
	WriteHardware( DSPX_INT_CLR, SaveInt );
	
/* Set shadow control register. */
	{
		int32 Mask;
		Mask = DSPXB_DMA_NEXTVALID | DSPXB_DMA_GO_FOREVER;
		Mask = Mask | (Mask << 16);  /* Set enable bits */
		WriteHardware( DSPX_DMA_STACK_CONTROL + DMAChan, Mask );
	}

#if 0
/* Calculate proper Interrupt Mask from DMAChan 940810 */
	IntMask = (uint32)1 << ConvertDMAChanToInterrupt(DMAChan);
/* Clear interrupt bit from previous DMA %Q This seems scary! */
	WriteHardware( ClrInt0Bits, IntMask ); /* Use proper mask. 940810 */
#endif

	WriteHardware(DMAPtr++, (int32) NextAddr);   /* SHADOW !!! Tell Steve */
	WriteHardware(DMAPtr, NextCnt);
	
/* Restore DMA Interrupts. */
	WriteHardware( DSPX_INT_SET, SaveInt );
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

	DMAPtr = DSPX_DMA_STACK + (DMAChan<<2);

	dsphDisableDMA( DMAChan );
/*
** Disable all DMA interrupts so we don't interrupt while using shadow!
** Save the current enables.
*/
	SaveInt = ReadHardware( DSPX_INT_SET, DSPXB_INT_ALL_DMA );
	WriteHardware( DSPX_INT_CLR, SaveInt );
	
/* Set shadow control register. */
	{
		int32 Mask;
		Mask = DSPXB_DMA_NEXTVALID | DSPXB_DMA_GO_FOREVER;
		Mask = Mask | (Mask << 16);  /* Set enable bits */
		WriteHardware( DSPX_DMA_STACK_CONTROL + DMAChan, Mask );
	}
	
#if 0
/* Calculate proper Interrupt Mask from DMAChan 940810 */
	IntMask = (uint32)1 << ConvertDMAChanToInterrupt(DMAChan);
/* Clear interrupt bit from previous DMA %Q This seems scary! */
	WriteHardware( ClrInt0Bits, IntMask ); /* Use proper mask. 940810 */
#endif

/* Write Addr, Count, NextAddr, NextCount to DMA */
	WriteHardware(DMAPtr++, (int32) Addr);
	WriteHardware(DMAPtr++, Cnt);
	WriteHardware(DMAPtr++, (int32) NextAddr);  /* SHADOW !!! Tell Steve */
	WriteHardware(DMAPtr,   NextCnt);
	
/* Restore DMA Interrupts. */
	WriteHardware( DSPX_INT_SET, SaveInt );
	dsphEnableDMA( DMAChan );

}

/*******************************************************************/
void dsphDisableDMA( int32 DMAChan )
{
	vuint32 *DMAPtr;
	uint32 DMAMask;
	
	DMAPtr = DSPX_DMA_STACK + (DMAChan<<2);
	
/* Disable DMA requests so we don't loop while setting registers! */
	DMAMask = 1L << DMAChan;
	WriteHardware(DSPX_CHANNEL_DISABLE, DMAMask );

/* Wait for DMA to settle. */
	while( *DMAPtr != *DMAPtr)
	{
		DBUGDMA(("dsphDisableDMA: DMA not stopped!\n"));
	}
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
#endif

#endif /* ASIC_BULLDOG */
