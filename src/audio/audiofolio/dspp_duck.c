/* $Id: dspp_duck.c,v 1.2 1994/12/23 01:22:48 markn Exp $ */
/****************************************************************
**
** DSPP Duck and Cover Support
** When DIPIR occurs we either ramp the sound up or down.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/


#include "audio_internal.h"
#include "touch_hardware.h"

/* #define DEBUG */
#define DBUG(x)      /* PRT(x) */

/* Prototypes **************************************/

static Err RampAudioAmplitude( int32 IfRampDown );
static IOReq *SendXBusRequest( Item XBusOpened, char *Name,
	int32 Command, uint32 Direction );
	
IOReq *RampAudioHook(IOReq *theReq);

Err dsppInitDuckAndCover( void );
void dsppTermDuckAndCover( void );

/* Constants ***************************************/

#define EO_FRAMECOUNT  0x002
#define GAININCR (25)
#define MAXTRYS  (100) /* Typically 1 try is enough but if the processor gets real fast... */
#define RAMP_UP    (0)
#define RAMP_DOWN  (1)

/* Global Data ************************************/

static int32 gGainBeforeDuck = 0x7FFF;
static IOReq  *gDuckIOReq, *gRestoreIOReq;


/* Code *******************************************/

static Err RampAudioAmplitude( int32 IfRampDown )
{
	int32 Gain;
	int32 OldFrameCount;
	int32 FrameCount;
	int32 CountDown;
	int32 Result;
	int32 DoIt = TRUE;
	vuint32 *FrameCountPtr;
	AudioKnob *aknob;
	DSPPInstrument *dins;
	DSPPKnob *dknb;
	
	aknob = (AudioKnob *)CheckItem(gHeadAmpKnob, AUDIONODE, AUDIO_KNOB_NODE);
	if (aknob == NULL) return AF_ERR_BADITEM; /* 930828 */
	
	dins = (DSPPInstrument *) aknob->aknob_DeviceInstrument;
	dknb = (DSPPKnob *) aknob->aknob_DeviceKnob;

	Gain = aknob->aknob_CurrentValue;
	if( IfRampDown )
	{
		gGainBeforeDuck = Gain;
	}

/* Setup pointers to DSP registers. */
	FrameCountPtr = DSPPEO16 + EO_FRAMECOUNT;
	
	OldFrameCount = *FrameCountPtr & 0xFFFF;
	
	while( DoIt )
	{
		if( IfRampDown )
		{
			Gain -= GAININCR;  /* Reduce gain */
			if( Gain < 0 )
			{ 
				Gain = 0;
				DoIt = FALSE;
			}
		}
		else
		{
			Gain += GAININCR;  /* Reduce gain */
			if( Gain > gGainBeforeDuck )
			{ 
				Gain = gGainBeforeDuck;
				DoIt = FALSE;
			}
		}

		
		Result = DSPPPutKnob(dins, dknb, Gain, &aknob->aknob_CurrentValue, TRUE);
		if( Result < 0 ) return Result;

/* Wait for next frame. */
		CountDown = MAXTRYS;
		do
		{
			if( CountDown-- <= 0 ) return -1;  /* %Q In case DSP is dead. */
			FrameCount = *FrameCountPtr & 0xFFFF;
		} while( OldFrameCount == FrameCount );
		OldFrameCount = FrameCount;
	};
	
	return Result;
}

/*******************************************************************
** RampAudioHook()
**
** Ramp audio amplitude before or after DIPIR
**
** Parameters: theReq - pointer to the incoming dipirReq
**       ioi_User says whether to ramp up or down.
**
** Returns: IOReq - This automagically resends the recoverReq to xbus.
**
** Called by: OS, just after dipir.
**
********************************************************************/
IOReq *RampAudioHook(IOReq *theReq)
{

	int32 IfRampDown;
	int32 Result;
	
	IfRampDown = (int32) theReq->io_Info.ioi_User;  /* Get direction. */
	
	Result = RampAudioAmplitude( IfRampDown );
	
/* cause kernel to resend this req to xbus if we succeed*/
	if( Result < 0 )
	{
		return NULL;
	}
	else
	{
		return (theReq);
	}
	
}

/**********************************************************************
** The following code is based largely on the LCCD driver use of DuckAndCover
** Thank you Chris Peterson and Mark Nudelman
**********************************************************************/

/*******************************************************************************
** dsppInitDuckAndCover()                                                        *
**
** Sends ioReq to notify xbus that we wish to be notified when
**               some (any) device was just dipired.
**
** Called during folio initialization.
*******************************************************************************/
Err dsppInitDuckAndCover( void )
{
	Item    XBusItem;
	Item    XBusOpened;
	
/* Find XBus Device */
	XBusItem = SuperFindNamedItem(MKNODEID(KERNELNODE, DEVICENODE), "xbus");
	if(XBusItem < 0)
	{
		ERR(("dsppSetupDuckAndCover: Could not find xbus device\n"));
		return XBusItem;
	}
	
/* Open XBus device. */
	XBusOpened = SuperOpenItem(XBusItem, 0);
	if(XBusOpened < 0)
	{
		ERR(("dsppSetupDuckAndCover: Could not open xbus device\n"));
		return XBusOpened;
	}

/* Send IO Requests that stay in effect till RESTART */
	gDuckIOReq = SendXBusRequest( XBusOpened, "audio_duck",
		XBUSCMD_WaitDipirStart, RAMP_DOWN );
	if( gDuckIOReq == NULL ) return -1;
	
	gRestoreIOReq = SendXBusRequest( XBusOpened, "audio_restore",
		XBUSCMD_WaitDipirEnd, RAMP_UP );
	if( gRestoreIOReq == NULL ) return -1;
	
	return (0);
}

/* Send a DuckAndCover request to XBus */
static IOReq *SendXBusRequest( Item XBusOpened, char *Name,
	int32 Command, uint32 Direction )
{
	Item    IOReqItem;
	IOReq  *xbIOReq;
	
/* Create IO Request for callback on duck and cover and send to xbus device. */
	IOReqItem = SuperCreateIOReq( Name, 0, XBusOpened, 0);
	if(IOReqItem < 0)
	{
		ERR(("dsppSetupDuckAndCover: Could not create IO req.\n"));
		return NULL;
	}
	xbIOReq = (IOReq *)LookupItem(IOReqItem);

/* Clear IO Info */
	memset((char *) &(xbIOReq->io_Info), 0, sizeof(IOInfo));

/* Set callback function and ramp direction. */
	xbIOReq->io_CallBack = RampAudioHook;
	xbIOReq->io_Info.ioi_Command = (uint8) Command;
	xbIOReq->io_Info.ioi_User = Direction;
		
	if( SuperInternalSendIO( xbIOReq ) < 0 )
	{
		ERR(("Problem setting up dipir Restore ioReq\n"));
		SuperInternalDeleteItem(xbIOReq->io.n_Item);
		return NULL;
	}
	
	return xbIOReq;
}

/* Remove IO Requests in case folio is shut down *******/
void dsppTermDuckAndCover( void )
{
	if( gRestoreIOReq ) SuperInternalDeleteItem(gRestoreIOReq->io.n_Item);
	gRestoreIOReq = NULL;
	
	if( gDuckIOReq ) SuperInternalDeleteItem(gDuckIOReq->io.n_Item);
	gDuckIOReq = NULL;
}
