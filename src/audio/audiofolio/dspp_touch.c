/* $Id: dspp_touch.c,v 1.45 1994/09/08 21:09:53 phil Exp $ */
/*******************************************************************
**
** Interface to DSPP
** This file now contains hardware routines that are source level
** compatible between Anvil and Bulldog.
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
** 930713 PLB Initialize EI memory for dipir scaling romtail.dsp
** 931222 PLB DSPP_SetFullDMA()
** 940126 PLB Remove some annoying messages.
** 940321 PLB Fixed pointer arithmetic bug that was masked by compiler bug.
**	          DMAPtr = (vuint32 *) (RAMtoDSPP0 + (DMAChan<<2)); BAD!!!!
** 940606 PLB Fixed roundoff error in CvtBytesToFrame() thet was causing the
**            number of bytes and frames to disagree.  501 => 500.
** 940810 PLB Clear interrupt using proper mask so that Output DMA
**            works.  This was causing a premature termination of
**            Output DMA if MonitorAttachment() was called.
** 940901 PLB Removed AODOUT setup code.  Now done in ROM.
*/

#include "audio_internal.h"
#include "touch_hardware.h"

#define DBUG(x) /* PRT(x) */
#define DBUGDMA(x) /* PRT(x) */
#define NDBUG(x) /* */

/*******************************************************************/
/* DSPP static data. */

short *Silence;      /* Zero data to play silence. */
short *ScratchRAM;   /* Scratch target of output DMA. */

void dsphDownloadStartupCode( void );

/*******************************************************************
** Write to instruection memory to be executed by DSPP
*******************************************************************/
void dsphWriteCodeMem( int32 CodeAddr, int32 Value )
{
TRACEB(TRACE_INT, TRACE_OFX, ("dsphWriteCodeMem(Naddr = %x, Value = %x)\n", NAddr, Value));
	if ((CodeAddr < 0) || (CodeAddr > LAST_N_MEM))
	{
		ERRDBUG(("PatchDSPPCode: Invalid N Address = $%x\n", CodeAddr));
	}
	else
	{
DBUG(("PatchDSPPCode: 0x%x = 0x%x\n", CodeAddr, Value));
		WriteHardware (DSPX_CODE_MEMORY + CodeAddr, Value);
	}
}

/*******************************************************************
** Read address of DMA channel.  Used by WhereAttachment()
*******************************************************************/
int8  *dsphReadChannelAddress( int32 Channel )
{
	vuint32	*DMAPtr;
	
	DMAPtr = DSPX_DMA_STACK + (Channel<<2);
	return (int8 *) ReadHardware( DMAPtr );
}

/********************************************************************
**
**	Reset DSPP
**	Load default code
**
Assumed Initialized by ROM
	AUDOUT
	
Set by Folio
	DSPPGW = 0
	DSPPRST1 = 0
	FIFOINIT for Input DMAs
	FIFOINIT for Output DMAs
	Set all EI memory to 0
	Set all N memory to 8380 (Sleep)
	Input DMA Registers point to Silence
	Output DMA Registers point to Silence
	DSP Semaphore used to test setting of I memory
	
Things I don't set...
	I memory, not a reboot issue
	EO memory
	
********************************************************************/
int32	dsphInitDSPP( void )
{
	int32 i;

   DBUG(("Entering DSPP_Init()\n"));

	Silence = (short *) SuperMemAlloc(SILENCE_SIZE, MEMTYPE_AUDIO|MEMTYPE_FILL);
	if (Silence == 0) return AF_ERR_NOMEM;
	ScratchRAM = (short *) SuperMemAlloc(SILENCE_SIZE, MEMTYPE_AUDIO);
	if (ScratchRAM == 0) return AF_ERR_NOMEM;

/* Configure DSPP */
	dsphHalt();
	dsphReset();	
	dsphResetAllFIFOs();
	
/* Initialize EI memory. 930713 */
	for (i=0; i<LAST_EI_MEM; i++)
	{
		dsphWriteEIMem( i, 0 );
	}
	
/* Paint with SLEEP to prevent runaway code */
	for (i=0; i<256; i++)
	{
		WriteHardware(DSPPN32+i, 0x83808380);	/* SLEEP; SLEEP  */
	}
	
	dsphDownloadStartupCode();

	dsphStart();		/* Start DSPP */
	dsphInitDMA();

	dsphResetAllFIFOs();
	
	return(0);
}

/*******************************************************************/
int32	dsphTermDSPP( void )
{

	dsphHalt();
	dsphReset();
		
	if (Silence != 0) SuperMemFree(Silence, SILENCE_SIZE);
	Silence = 0;
	if ( ScratchRAM != 0) SuperMemFree(ScratchRAM, SILENCE_SIZE);
	ScratchRAM = 0;

	return(0);
}
