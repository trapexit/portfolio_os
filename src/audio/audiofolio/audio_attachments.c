/* $Id: audio_attachments.c,v 1.50 1994/12/01 05:41:46 phil Exp $ */
/****************************************************************
**
** Audio Internals to support Attachments
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
** 930415 PLB Track connections between Items
** 930517 PLB Add NOAUTOSTART and FATLADYSINGS flags
** 930726 PLB Fixed bug in StartAt range check for NumFrames == 0
** 930829 PLB Only allow Template or Instrument in CreateAttachment
** 930828 PLB Check for illegal flags in SetAttachmentInfo
** 930828 PLB Check for Cue requests other than CUE_AT_END
** 931117 PLB Support for envelopes added to WhereAttachment
** 931130 PLB Fix bug with premature STOP for looping samples with FATLADYSINGS
** 931201 PLB Keep an attachment reference for template attachments too.
**            This fixes a bug Don found if he shared a sample on multiple templates
**            and and tried to delete one template.
** 931220 PLB Fixed cutoff of sound if Attachment2 Linked or Unlinked when 
**               Attachment1 has loops.
** 940224 PLB Move Attachment list to aitp to prepare for shared dtmp.
** 930404 PLB Fix trace debug, c/aitp/aatt/
** 940422 DGB Change spelling of SetMode to SETMODE, same as inthard.h
** 940623 WJB Added some potential internalGetAttachmentInfo() innards (commented out until approved).
** 940810 PLB Only set SETMODE for Input DMA channels.
** 940811 WJB Fixed AllocSignal() usage to correctly detect a 0 result.  
** 940825 PLB Distinguish between Sample and Envelope Attachments in
**            StartAttachment(), ReleaseAttachment(), StopAttachment()
** 940901 PLB Use dsphEnableChannelInterrupt()
** 940922 PLB swiLinkAttachment distinguishes between samples and envelopes.
**            Change slow calls to CheckItem() to LookupItem().
** 941116 PLB Removed SendAttachment(). Redesign and implement later.
** 941121 PLB internalCreateAttachment() - Prevent memory leak if AT_TAG_HOOKNAME
**            called more than once.  Plug hole if AF_TAG_SAMPLE and AF_TAG_ENVELOPE
**            both used.
** 941121 PLB Fixed interrupt disable from interrupt.  Was improperly converting
**            from channel to interrupt.  Bug only in babs version.
****************************************************************/

#include "audio_internal.h"
#include "filefunctions.h"

/* Macros for debugging. */
#define DBUG(x)    /* PRT(x) */

/***************************************************************/
/* Generic DMA Interrupt routine */

int32 HandleDMAChannelInterrupt( int32 DMAChannel, int32 InterruptIndex )
{
	AudioDMAControl *admac;
	
/* Find appropriate DMA control structure. */
	admac = &AudioBase->af_DMAControls[DMAChannel];
	
/* Set Next registers if sample is queued. */
	if((admac->admac_NextCountDown > 0) && (--(admac->admac_NextCountDown) == 0))
	{
		admac->admac_ChannelAddr->dmar_NextAddress = admac->admac_NextAddress;
		admac->admac_ChannelAddr->dmar_NextCount = admac->admac_NextCount;
	}

/* Signal foreground if counter reaches zero. */
	if((admac->admac_SignalCountDown > 0) && (--(admac->admac_SignalCountDown) == 0))
	{
		SuperInternalSignal(AudioBase->af_AudioTask, admac->admac_Signal);
	}
	
/* Are we all done? */
	if ((admac->admac_NextCountDown == 0) && (admac->admac_SignalCountDown == 0))
	{
		DisableInterrupt( InterruptIndex ); /* 941121 */
	}

	return 0;
}

/***************************************************************/

#define HANDLEDMACHANNEL(DMAChannel,IntIndex) \
	{ return HandleDMAChannelInterrupt(DMAChannel,IntIndex); }

/* Need different function for each interrupt. */
int32 AudioDMAIntR2D_0(void) HANDLEDMACHANNEL(0,INT_DRD0+0)
int32 AudioDMAIntR2D_1(void) HANDLEDMACHANNEL(1,INT_DRD0+1)
int32 AudioDMAIntR2D_2(void) HANDLEDMACHANNEL(2,INT_DRD0+2)
int32 AudioDMAIntR2D_3(void) HANDLEDMACHANNEL(3,INT_DRD0+3)
int32 AudioDMAIntR2D_4(void) HANDLEDMACHANNEL(4,INT_DRD0+4)
int32 AudioDMAIntR2D_5(void) HANDLEDMACHANNEL(5,INT_DRD0+5)
int32 AudioDMAIntR2D_6(void) HANDLEDMACHANNEL(6,INT_DRD0+6)
int32 AudioDMAIntR2D_7(void) HANDLEDMACHANNEL(7,INT_DRD0+7)
int32 AudioDMAIntR2D_8(void) HANDLEDMACHANNEL(8,INT_DRD0+8)
int32 AudioDMAIntR2D_9(void) HANDLEDMACHANNEL(9,INT_DRD0+9)
int32 AudioDMAIntR2D_10(void) HANDLEDMACHANNEL(10,INT_DRD0+10)
int32 AudioDMAIntR2D_11(void) HANDLEDMACHANNEL(11,INT_DRD0+11)
int32 AudioDMAIntR2D_12(void) HANDLEDMACHANNEL(12,INT_DRD0+12)
int32 AudioDMANoOp(void) { return 0; }
int32 AudioDMAIntD2R_0(void) HANDLEDMACHANNEL(16,INT_DSPPRAM0+0)
int32 AudioDMAIntD2R_1(void) HANDLEDMACHANNEL(17,INT_DSPPRAM0+1)
int32 AudioDMAIntD2R_2(void) HANDLEDMACHANNEL(18,INT_DSPPRAM0+2)
int32 AudioDMAIntD2R_3(void) HANDLEDMACHANNEL(19,INT_DSPPRAM0+3)

int32 (*DMAIntHandlerTable[NUM_AUDIO_DMA_CHANNELS])() =
{
	AudioDMAIntR2D_0,
	AudioDMAIntR2D_1,
	AudioDMAIntR2D_2,
	AudioDMAIntR2D_3,
	AudioDMAIntR2D_4,
	AudioDMAIntR2D_5,
	AudioDMAIntR2D_6,
	AudioDMAIntR2D_7,
	AudioDMAIntR2D_8,
	AudioDMAIntR2D_9,
	AudioDMAIntR2D_10,
	AudioDMAIntR2D_11,
	AudioDMAIntR2D_12,
	AudioDMANoOp,
	AudioDMANoOp,
	AudioDMANoOp,
	AudioDMAIntD2R_0,
	AudioDMAIntD2R_1,
	AudioDMAIntD2R_2,
	AudioDMAIntD2R_3
};

Item DMAfirqItems[NUM_AUDIO_DMA_CHANNELS];
static
TagArg DMAFirqTags[] =
{
	TAG_ITEM_PRI,		(void *)5,
	TAG_ITEM_NAME,	    "AudioDMA",
	CREATEFIRQ_TAG_CODE, 	NULL,
	CREATEFIRQ_TAG_NUM, 	NULL,
	TAG_END, NULL
};

#define DDR0_CHANNEL (16)


/***************************************************************/
int32 InitAudioDMAInt ( int32 DMAChan )
{
	AudioDMAControl *admac;
	
	admac = &AudioBase->af_DMAControls[DMAChan];

/* Allocate signal for interrupt to signal foreground. */
    {
        const int32 sig = SuperAllocSignal(0); 
    
        if (sig <= 0) /* 940811 */
        {
            ERR(("InitAudioDMAInt could not SuperAllocSignal\n"));
            return sig ? sig : AF_ERR_NOSIGNAL;
        }
        admac->admac_Signal = sig; 
    }    
/* OR together for WaitSignal mask. */
	AudioBase->af_DMASignalMask |= admac->admac_Signal;

/* Make an interrupt request item. */
DBUG(("Handler[%d] at 0x%x\n", DMAChan, (void *) ((int32)DMAIntHandlerTable[DMAChan])  ));
	DMAFirqTags[2].ta_Arg = (void *) ((int32)DMAIntHandlerTable[DMAChan]);
	DMAFirqTags[3].ta_Arg = (void *) (dsphConvertChannelToInterrupt(DMAChan));
	
	DMAfirqItems[DMAChan] = SuperCreateItem(MKNODEID(KERNELNODE,FIRQNODE),DMAFirqTags);
	if (DMAfirqItems[DMAChan] < 0)
	{
		ERRDBUG(("InitAudioDMAInt failed to create interrupt: 0x%x\n",
			DMAfirqItems[DMAChan]));
		return DMAfirqItems[DMAChan];
	}
/* Set Mode to 1 for when DMA rolls over. */
/* Only for Input DMA 940810 */
	if(DMAChan < DDR0_CHANNEL)
	{
		*SETMODE = (uint32)INT0_DRDINT0 << DMAChan;
	}
	admac->admac_ChannelAddr = (((DMARegisters *) RAMtoDSPP0) + DMAChan);
	return 0;
}

/***************************************************************/
int32 InitAudioDMAInterrupts ( void )
{
	int32 i;
	int32 Result;
	
	AudioBase->af_DMASignalMask = 0;
	
	for (i=0; i<NUM_AUDIO_INPUT_DMAS; i++)
	{
		Result = InitAudioDMAInt( i + DRD0_CHANNEL );
		if( Result < 0) return Result;
	}
	
	for (i=0; i<NUM_AUDIO_OUTPUT_DMAS; i++)
	{
		Result = InitAudioDMAInt( i + DDR0_CHANNEL );
		if( Result < 0) return Result;
	}
DBUG(("DMAControl array at: 0x%x\n", &AudioBase->af_DMAControls[0]));
DBUG(("&Time = 0x%x\n", &AudioBase->af_Time));

	return 0;
}

/*****************************************************************/
int32 TermAudioDMAInt ( int32 DMAChan )
{
	int32 Result;
	AudioDMAControl *admac;
	
	admac = &AudioBase->af_DMAControls[DMAChan];
		
/* Free signal for interrupt to signal foreground. */
/* !!! doesn't remove from AudioBase->af_DMASignalMask! Is that a problem? NO */
	if(admac->admac_Signal) { 
        SuperFreeSignal(admac->admac_Signal);
        admac->admac_Signal = 0;
    }    
	Result = afi_SuperDeleteItem( DMAfirqItems[DMAChan] );
	{
		ERRDBUG(("TermAudioDMAInterrupts: delete item failed  0x%x\n", Result));
		return Result;
	}
}

/*****************************************************************/
int32 TermAudioDMAInterrupts ( void )
{
	int32 i;
	int32 Result;
	
	AudioBase->af_DMASignalMask = 0;
	
	for (i=0; i<NUM_AUDIO_INPUT_DMAS; i++)
	{
		Result = TermAudioDMAInt( i + DRD0_CHANNEL );
		if( Result < 0) return Result;
	}
	
	for (i=0; i<NUM_AUDIO_OUTPUT_DMAS; i++)
	{
		Result = TermAudioDMAInt( i + DDR0_CHANNEL );
		if( Result < 0) return Result;
	}

	return 0;
}

/*****************************************************************/
/**  Send a signal to any Cue that may be monitoring. ************/
/*****************************************************************/
int32 SignalMonitoringCue( AudioAttachment *aatt )
{
	AudioCue *acue;
	
/* Signal task that called MonitorAttachment. */
	if(aatt->aatt_CueItem)
	{
/*	940922	acue = (AudioCue *)CheckItem(aatt->aatt_CueItem,  AUDIONODE, AUDIO_CUE_NODE); */
		acue = (AudioCue *)LookupItem(aatt->aatt_CueItem );
		if (acue == NULL)
		{
			ERR(("SignalMonitoringCue: Cue deleted!\n"));
			aatt->aatt_CueItem = 0;
		}
		else
		{	
DBUG(("SignalMonitoringCue: cue = 0x%x, signal = 0x%x\n", acue, acue->acue_Signal ));
			SuperinternalSignal( acue->acue_Task, acue->acue_Signal );
		}
	}
	return 0;
}

/*****************************************************************/
/**  Process signal from DMA interrupt. ***************************/
/*****************************************************************/
int32 ProcessAttachmentDMA( AudioAttachment *PrevAtt )
{
	AudioAttachment *CurAtt;
	AudioDMAControl *admac;
	int32 Result;
	int32 DMAChan;
	
	Result=0;
	
	DMAChan = PrevAtt->aatt_Channel;
	admac = &AudioBase->af_DMAControls[DMAChan];

DBUG(("ProcessAttachmentDMA( 0x%x ) SegmentCount = 0x%x\n", PrevAtt, PrevAtt->aatt_SegmentCount));
DBUG(("PrevAtt->aatt_ActivityLevel = 0x%x\n", PrevAtt->aatt_ActivityLevel));
	if((PrevAtt->aatt_SegmentCount > 0) &&
		(--PrevAtt->aatt_SegmentCount == 0))  /* 931130 */
	{
		PrevAtt->aatt_ActivityLevel = AF_STOPPED;	
/* This will prevent the HandleDMASignal() routine from calling us again.
** DSPPGoToAttachment() will set this to the next attachment. */				
		admac->admac_AttachmentItem = 0;  

		if (PrevAtt->aatt_Flags & AF_ATTF_FATLADYSINGS)
		{
DBUG(("ProcessAttachmentDMA: FATLADYSINGS=>STOP\n"));
			Result = swiStopInstrument( PrevAtt->aatt_HostItem, NULL );
			if (Result < 0)
			{
				ERRDBUG(("ProcessAttachmentDMA: error stopping 0x%x\n",
					PrevAtt->aatt_HostItem));
				return Result;
			}
		}
		else if ( PrevAtt->aatt_NextAttachment )
		{
/*	940922		CurAtt = (AudioAttachment *)CheckItem(PrevAtt->aatt_NextAttachment,
**				AUDIONODE, AUDIO_ATTACHMENT_NODE);
*/
			CurAtt = (AudioAttachment *)LookupItem(PrevAtt->aatt_NextAttachment);
			if (CurAtt == NULL)
			{
/* Just eliminate the reference and point to silence. */
				ERR(("ProcessAttachmentDMA: Attachment deleted!\n"));
				 PrevAtt->aatt_NextAttachment = 0;
				DSPP_SilenceDMA(PrevAtt->aatt_Channel);
			}
			else
			{
DBUG(("ProcessAttachmentDMA: Call DSPPStartAttachment()\n"));
				DSPPStartSampleAttachment( CurAtt, FALSE );
			}
		}
		else
		{
			DSPP_SilenceDMA(PrevAtt->aatt_Channel);
DBUG(("ProcessAttachmentDMA: Silence.\n"));
		}
		
/* Signal task that called MonitorAttachment. */
DBUG(("ProcessAttachmentDMA: Send monitoring signal.\n"));
		SignalMonitoringCue( PrevAtt );
	}
	else
	{
		EnableAttSignalIfNeeded( PrevAtt );
	}
	
	return Result;
}
/*****************************************************************/
/* Scan DMAControls to see which one(s) signalled.
** Send Cue to those.
*/

int32 HandleDMASignal( int32 Signals )
{
	AudioDMAControl *admac;
	AudioAttachment *PrevAtt;
	int32 i;
	
DBUG(("\nHandleDMASignal( 0x%x)\n", Signals ));
	admac = &AudioBase->af_DMAControls[0];
	for (i=0; i<NUM_AUDIO_DMA_CHANNELS; i++)
	{
		if (Signals & admac->admac_Signal)
		{
			if(admac->admac_AttachmentItem)
			{
/*		940922		PrevAtt = (AudioAttachment *) CheckItem(admac->admac_AttachmentItem,
**				         AUDIONODE, AUDIO_ATTACHMENT_NODE);
*/
/* PrevAtt just finished */
				PrevAtt = (AudioAttachment *) LookupItem(admac->admac_AttachmentItem);
				if(PrevAtt == NULL)
				{
					ERR(("HandleDMASignal: warning, Attachment deleted.\n"));
				}
				else
				{
					ProcessAttachmentDMA( PrevAtt );
				}
			}
			
/* Once a signal has been handled remove it from mask until it is zero. */
			Signals = Signals ^ admac->admac_Signal;
			if (Signals == 0) break;
		}
		admac++;
	}
	return 0;
}


/*****************************************************************/

 /**
 |||	AUTODOC PUBLIC spg/items/attachment
 |||	Attachment - The binding of a sample or an envelope to an instrument.
 |||
 |||	  Description
 |||
 |||	    An attachment is the item to binds a sample or an envelope to a particular
 |||	    instrument.
 |||
 |||	    An Attachment is associated with precisely one Envelope and one Instrument,
 |||	    or one Sample and one Instrument. An Attachment is said to be an Envelope
 |||	    Attachment if it is attached to an Envelope, or a Sample Attachment if
 |||	    attached to a Sample.
 |||
 |||	    Sample attachments actually come in two flavors: one for input FIFOs and
 |||	    another for output FIFOs, defined by the Hook to which the Sample is
 |||	    attached. Both kinds are considered Sample Attachments and no distinction
 |||	    is made between them.
 |||
 |||	    A single Instrument can have one Envelope Attachment per Envelope hook and
 |||	    one Sample Attachment per Output FIFO. A single Instrument can have
 |||	    multiple Sample Attachments per Input FIFO, but only one will be selected
 |||	    to be played when the instrument is started. This is useful for creating
 |||	    multi-sample instruments, where the sample selected to be played depends on
 |||	    the pitch to be played.
 |||
 |||	    A single Sample or Envelope can have multiple Attachments made to it.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_ATTACHMENT_NODE
 |||
 |||	  Create
 |||
 |||	    AttachSample()
 |||
 |||	    AttachEnvelope()
 |||
 |||	    CreateItem()
 |||
 |||	  Delete
 |||
 |||	    DetachSample()
 |||
 |||	    DetachEnvelope()
 |||
 |||	    DeleteItem()
 |||
 |||	  Query
 |||
 |||	    None
 |||
 |||	  Modify
 |||
 |||	    SetAudioItemInfo()
 |||
 |||	  Use
 |||
 |||	    LinkAttachments()
 |||
 |||	    MonitorAttachment()
 |||
 |||	    ReleaseAttachment()
 |||
 |||	    StartAttachment()
 |||
 |||	    StopAttachment()
 |||
 |||	    WhereAttachment()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_CLEAR_FLAGS           (uint32) Modify. Set of AF_ATTF_ flags to
 |||	                                 clear. Clears every flag for which a 1 is
 |||	                                 set in ta_Arg. See also AF_TAG_SET_FLAGS. The
 |||	                                 final state of any flag is determined by the
 |||	                                 last occurrence of that flag in a
 |||	                                 AF_TAG_SET_FLAGS or AF_TAG_CLEAR_FLAGS.
 |||
 |||	    AF_TAG_ENVELOPE              (Item) Create. Envelope Item to attach to
 |||	                                 Instrument. Exactly one of AF_TAG_ENVELOPE
 |||	                                 or AF_TAG_SAMPLE must be specified.
 |||
 |||	    AF_TAG_HOOKNAME              (const char *) Create. The name of the
 |||	                                 sample or envelope hook in the instrument to
 |||	                                 attach to. For sample hooks, defaults to the
 |||	                                 first one listed for each instrument. For
 |||	                                 envelopes, defaults to "Env".
 |||
 |||	    AF_TAG_INSTRUMENT            (Item) Create. Instrument or Template item
 |||	                                 to attach envelope or sample to. Must be
 |||	                                 specified when creating an Attachment.
 |||
 |||	    AF_TAG_SAMPLE                (Item) Create. Sample Item to attach to
 |||	                                 instrument. Exactly one of AF_TAG_ENVELOPE
 |||	                                 or AF_TAG_SAMPLE must be specified.
 |||
 |||	    AF_TAG_SET_FLAGS             (uint32) Create, Modify. Set of AF_ATTF_
 |||	                                 flags to set. Sets every flag for which a 1
 |||	                                 is set in ta_Arg. See also AF_TAG_CLEAR_FLAGS.
 |||	                                 The final state of any flag is determined by
 |||	                                 the last occurrence of that flag in a
 |||	                                 AF_TAG_SET_FLAGS or AF_TAG_CLEAR_FLAGS.
 |||
 |||	    AF_TAG_START_AT              (int32) Create, Modify. Specifies the point at
 |||	                                 which to start when the attachment is started
 |||	                                 (with StartAttachment() or StartInstrument()).
 |||
 |||	                                 For sample attachments, specifies a sample
 |||	                                 frame number in the sample at which to begin
 |||	                                 playback.
 |||
 |||	                                 For envelopes, attachments specifies the
 |||	                                 segment index at which to start.
 |||
 |||	    AF_TAG_TIME_SCALE            (ufrac16) Create. Scales all of the times in
 |||	                                 the attached envelopes by the supplied
 |||	                                 ufrac16. Defaults to 1.0. Only applies to
 |||	                                 envelope attachments.
 |||
 |||	  Flags
 |||
 |||	    AF_ATTF_FATLADYSINGS         If set, causes the instrument to stop when the
 |||	                                 attachment finishes playing.
 |||
 |||	                                 This bit can be used to mark the one or more
 |||	                                 envelope(s) or sample(s) that are considered
 |||	                                 to be the determiners of when the instrument
 |||	                                 is completely done.
 |||
 |||	                                 For envelopes, the default setting for this
 |||	                                 flag comes from the AF_ENVF_FATLADYSINGS
 |||	                                 flag. Defaults to cleared for samples.
 |||
 |||	    AF_ATTF_NOAUTOSTART          When set, causes StartInstrument() to not
 |||	                                 automatically start this attachment. This
 |||	                                 allows later starting of the attachment by
 |||	                                 using StartAttachment(). Defaults to
 |||	                                 cleared (attachment defaults to starting when
 |||	                                 instrument is started).
 |||
 **/

/*****************************************************************/
/***** Create Attachment Item for Folio **************************/
/*****************************************************************/
Item internalCreateAudioAttachment (AudioAttachment *aatt, TagArg *args)
{
  	char *HookName = NULL;
  	int32 Result=0;
	uint32 tagc, *tagp;
	AudioSample *asmp=NULL;
	AudioEnvelope *aenv=NULL;
	AudioInstrument *ains;
	AudioInsTemplate *aitp;
	DSPPInstrument *dins;
	Item InsOrTmp;
	Item Sample;
	Item Envelope;
	Node *n;
	int32 Temp;
	frac16 TimeScale;

	InsOrTmp = -1;
	Sample = -1;
	Envelope = -1;
	TimeScale = Convert32_F16(1);
	
TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioAttachment( %x, %x )\n",aatt,args));
	Result = TagProcessor( aatt, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioAttachment: TagProcessor failed.\n"));
    	return Result;
    }

	tagp = (uint32 *)args;
	
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			switch (tagc)
			{
			case AF_TAG_INSTRUMENT:
				InsOrTmp = (Item) *tagp++;
				aatt->aatt_HostItem = InsOrTmp;
				break;
						
			case AF_TAG_SAMPLE:
				if( aatt->aatt_SlaveItem ) return AF_ERR_TAGCONFLICT; /* 941121 */
				Sample = (Item) *tagp++;
				asmp = (AudioSample *)CheckItem(Sample,  AUDIONODE, AUDIO_SAMPLE_NODE);
				if (asmp == NULL) return AF_ERR_BADITEM;
				aatt->aatt_SlaveItem = Sample;
				aatt->aatt_Structure = asmp;
				break;
								
			case AF_TAG_ENVELOPE:
				if( aatt->aatt_SlaveItem ) return AF_ERR_TAGCONFLICT; /* 941121 */
				Envelope = (Item) *tagp++;
				aenv = (AudioEnvelope *)CheckItem(Envelope,  AUDIONODE, AUDIO_ENVELOPE_NODE);
				if (aenv == NULL) return AF_ERR_BADITEM;
				aatt->aatt_SlaveItem = Envelope;
				aatt->aatt_Structure = aenv;
				break;

			case AF_TAG_HOOKNAME:
				HookName = (char *) *tagp++;
				break;
				
			case AF_TAG_SET_FLAGS:
				Temp =  *tagp++;
				if(Temp & ~AF_ATTF_LEGALFLAGS)
				{
					ERR(("Illegal attachment flags. 0x%x\n", Temp));
					return AF_ERR_BADTAGVAL;
				}
				aatt->aatt_Flags |= (uint32) Temp;
				break;
				
			case AF_TAG_START_AT:
				aatt->aatt_StartAt = (int32) *tagp++;
				break;
				
			case AF_TAG_TIME_SCALE:
				TimeScale = (int32) *tagp++;
				break;
				
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Unrecognized tag in internalCreateAudioAttachment={0x%x, 0x%x}\n",
					tagc, *tagp));
					return AF_ERR_BADTAG;
				}
			}
		}
	}


/* Process last HookName passed. 941121 */
	if(HookName == NULL)
	{
		aatt->aatt_HookName = NULL;
	}
	else
	{
		Result = afi_IsRamAddr ( HookName, 16 );
		if (Result < 0) return Result;
		aatt->aatt_HookName = afi_AllocateString( HookName );
DBUG(("internalCreateAudioAttachment: HookName = %s\n", HookName));
		if (aatt->aatt_HookName == NULL)
		{
			return AF_ERR_NOMEM;
		}
	}

	n = (Node *)LookupItem(InsOrTmp);
	if ((n==NULL) || (n->n_SubsysType != AUDIONODE))
	{
		ERR(("Bad instrument or template for attachment\n"));
		return AF_ERR_BADITEM;
	}

	switch (n->n_Type)
	{
		case AUDIO_INSTRUMENT_NODE:
			ains = (AudioInstrument *) n;
			dins = (DSPPInstrument *) ains->ains_DeviceInstrument;
			if (Sample > 0)
			{
				if((aatt->aatt_StartAt < 0) ||
					((aatt->aatt_StartAt >= asmp->asmp_NumFrames) &&
					 (asmp->asmp_NumFrames != 0))) /* 930726 */
				{
					ERR(("Attachment START_AT out of range = %d\n", aatt->aatt_StartAt));
					return AF_ERR_BADTAGVAL;
				}
				
				Result =  DSPPAttachSample(dins, asmp, aatt);
				if (Result < 0) return Result;
			}
			else if (Envelope > 0)
			{
				AudioEnvExtension *aeva;
				
				if((aatt->aatt_StartAt < 0) ||
					((aatt->aatt_StartAt >= aenv->aenv_NumPoints) &&
					 (asmp->asmp_NumFrames != 0))) /* 930726 */
				{
					ERR(("Attachment START_AT out of range = %d\n", aatt->aatt_StartAt));
					return AF_ERR_BADTAGVAL;
				}

				
/* Add Extension for envelope management. */
				aeva = (AudioEnvExtension *) EZMemAlloc(sizeof(AudioEnvExtension), MEMTYPE_FILL);
				if (aeva == NULL) return AF_ERR_NOMEM;
				aeva->aeva_Parent = aatt;
				aatt->aatt_Extension = aeva;
				
				Result =  DSPPAttachEnvelope(dins, aenv, aatt);

/* Set scaled time to allow envelope scaling. */
				SetEnvAttTimeScale( aatt, TimeScale );
			}

		break;
    
	case AUDIO_TEMPLATE_NODE:
		aitp = (AudioInsTemplate *) n;
		AddTail(&aitp->aitp_Attachments, (Node *)aatt); /* 940224 */
		break;
		
	default:
		return AF_ERR_BADITEM;  /* Only allow Template or Instrument. 930829 */
	}
	
/* Keep reference node for attachments to both instruments and templates. 931201 */	
	{
		AudioReferenceNode *arnd;
		
		arnd = (AudioReferenceNode *) EZMemAlloc(sizeof(AudioReferenceNode), 0);
		if (arnd == NULL) return AF_ERR_NOMEM;
		arnd->arnd_RefItem = aatt->aatt_Item.n_Item;
				
		if (Sample > 0)
		{
/* Keep reference to Attachment in sample */
			AddTail( &asmp->asmp_AttachmentRefs, (Node *) arnd );
			aatt->aatt_Type = AF_ATT_TYPE_SAMPLE;
		}
		else if (Envelope > 0)
		{
/* Keep reference to Attachment in envelope */
			AddTail( &aenv->aenv_AttachmentRefs, (Node *) arnd );
			aatt->aatt_Type = AF_ATT_TYPE_ENVELOPE;
		}
	}

	if (Result >= 0)
	{
TRACER(TRACE_INT,TRACE_ITEM,
	("internalCreateAudioAttachment returns 0x%x\n",aatt->aatt_Item.n_Item)); /* 940304 */
		return aatt->aatt_Item.n_Item;
	}
TRACER(TRACE_INT,TRACE_ITEM,
	("internalCreateAudioAttachment returns 0x%x\n",Result));
	
	return Result;
}

/*****************************************************************/
int32 internalGetAttachmentInfo (AudioAttachment *aatt, TagArg *args)
{
  #if 0
  
  	int32 Result = 0;  	
	uint32 tagc, *tagp;
  	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("internalGetAttachmentInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case AF_TAG_STATUS:
				*tagp++ = (uint32) aatt->aatt_ActivityLevel;
				break;
				
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Warning - unrecognized tag in internalGetAttachmentInfo - 0x%x: 0x%x\n",
					tagc, *tagp));	
					Result = AF_ERR_BADTAG;
					goto DONE;
				}
				tagp++;
			}
		}
	}
DONE:
	return Result;
	
  #else   
  
	return AF_ERR_UNIMPLEMENTED;
    
  #endif
}

/*****************************************************************/
int32 internalSetAttachmentInfo (AudioAttachment *aatt, TagArg *args)
{
	uint32 tagc, *tagp;
	uint32 NumFrames;
	uint32 Temp;
	AudioSample *asmp;
	AudioEnvelope *aenv;
	int32 StartAt;
	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("internalSetAttachmentInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case AF_TAG_START_AT:
				StartAt = (int32) *tagp++;
				NumFrames = 0;
				switch(aatt->aatt_Type)
				{
					case AF_ATT_TYPE_SAMPLE:
						asmp = (AudioSample *) aatt->aatt_Structure;
						NumFrames = asmp->asmp_NumFrames;
						break;
					case AF_ATT_TYPE_ENVELOPE:
						aenv = (AudioEnvelope *) aatt->aatt_Structure;
						NumFrames = aenv->aenv_NumPoints;
						break;
				}
				if((StartAt < 0) || (StartAt >= NumFrames))
				{
					ERR(("Attachment START_AT out of range = %d\n", StartAt));
					return AF_ERR_BADTAGVAL;
				}
				aatt->aatt_StartAt = StartAt;
				break;

			case AF_TAG_SET_FLAGS:
				Temp =  (uint32) *tagp++;
				if(Temp & ~AF_ATTF_LEGALFLAGS)
				{
					ERR(("Illegal attachment flags. 0x%x\n", Temp));
					return AF_ERR_BADTAGVAL;
				}
				aatt->aatt_Flags |= Temp;
				break;
				
			case AF_TAG_CLEAR_FLAGS:
				Temp =  (uint32) *tagp++;
				if(Temp & ~AF_ATTF_LEGALFLAGS)
				{
					ERR(("Illegal attachment flags. 0x%x\n", Temp));
					return AF_ERR_BADTAGVAL;
				}
				aatt->aatt_Flags &= ~Temp;
				break;
				
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Uunrecognized tag in internalSetAttachmentInfo - 0x%x: 0x%x\n",
						tagc, *tagp));	
					return AF_ERR_BADTAG;
				}
			}
		}
	}
	
/* Everything passed. */
	return 0;
}

/*************************************************************************/
Item SuperAttachSlave( Item Instrument, ItemNode *SlaveItem, char *FIFOName, uint32 Flags)
{
	Item Result = -1;
	TagArg Tags[5];

TRACEE(TRACE_INT,TRACE_ATTACHMENT,("SuperAttachSample( 0x%x, 0x%x, %s )\n", Instrument,
		SlaveItem, FIFOName));

	Tags[0].ta_Tag = AF_TAG_HOOKNAME;
	Tags[0].ta_Arg = (void *) FIFOName;
/* Use Tag appropriate to Slave type. */
	if(SlaveItem->n_Type == AUDIO_SAMPLE_NODE)
	{
		Tags[1].ta_Tag = AF_TAG_SAMPLE;
	}
	else
	{	
		Tags[1].ta_Tag = AF_TAG_ENVELOPE;
	}
	Tags[1].ta_Arg = (void *) SlaveItem->n_Item;
	
	Tags[2].ta_Tag = AF_TAG_INSTRUMENT;
	Tags[2].ta_Arg = (void *) Instrument;
	Tags[3].ta_Tag = AF_TAG_SET_FLAGS;
	Tags[3].ta_Arg = (void *) Flags;
	Tags[4].ta_Tag =  TAG_END;
    Result = SuperCreateItem( MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE), Tags );
TRACER(TRACE_INT, TRACE_SAMPLE, ("SuperAttachSample returns 0x%08x\n", Result));

	return Result;
}

/**************************************************************/

int32 internalDeleteAudioAttachment (AudioAttachment *aatt)
{
/* If Attachment deleted first:
		StopInstrument
		RemoveSampleReference
	If Instrument deleted first:
		RemoveSampleReference
	If Sample deleted first:
		StopInstrument
*/
	AudioEnvelope *aenv;

	AudioSample *asmp;
	Item Host, Slave;
	AudioInstrument *ains;
	AudioInsTemplate *aitp;
	int32 Result = 0;
	
TRACEE(TRACE_INT,TRACE_ITEM|TRACE_ATTACHMENT,
		("internalDeleteAudioAttachment (aatt=0x%x)\n", aatt));
	
	Slave = aatt->aatt_SlaveItem;
	Host = aatt->aatt_HostItem;
TRACKMEM(("internalDeleteAudioAttachment: aatt = 0x%x\n",aatt));

TRACEB(TRACE_INT,TRACE_ITEM|TRACE_ATTACHMENT,
		("internalDeleteAudioAttachment: Host=0x%x, Slave=0x%x\n",
		Host, Slave));
		
	ains = (AudioInstrument *)CheckItem(Host, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains != NULL)
	{
		if (aatt->aatt_ActivityLevel > AF_STOPPED)
		{
			Result = swiStopInstrument( Host, NULL );
			DSPPStopSampleAttachment( aatt );
		}
	}

TRACKMEM(("internalDeleteAudioAttachment: after StopAttachment\n"));

/* Remove from the Instrument or Templates List. */
	aitp = (AudioInsTemplate *)CheckItem(Host, AUDIONODE, AUDIO_TEMPLATE_NODE);
	if((aitp != NULL) || (ains != NULL))   /* Is list valid? */
	{
		ParanoidRemNode( (Node *) aatt );
	}
	else
	{
		ERRDBUG(("RemNode of aatt = 0x%x when ains = 0x%x, aitp = 0x%x\n", aatt, ains, aitp ));
	}
	
TRACKMEM(("internalDeleteAudioAttachment: after RemNode\n"));

/* Remove from the slaves's Attachment references list. */
	asmp = (AudioSample *)CheckItem(Slave, AUDIONODE, AUDIO_SAMPLE_NODE);
	if (asmp != NULL)
	{
		afi_RemoveReferenceNode( &asmp->asmp_AttachmentRefs, aatt->aatt_Item.n_Item);
	}
	else
	{
		aenv = (AudioEnvelope *)CheckItem(Slave, AUDIONODE, AUDIO_ENVELOPE_NODE);
		if (aenv != NULL)
		{
			afi_RemoveReferenceNode( &aenv->aenv_AttachmentRefs, aatt->aatt_Item.n_Item);
		}
/* If object is already dead then don't worry. It will remove reference from its list. */
	}
	
TRACKMEM(("internalDeleteAudioAttachment: after afi_RemoveReferenceNode\n"));

	if (aatt->aatt_Extension) EZMemFree( aatt->aatt_Extension );
	
	if (aatt->aatt_HookName) afi_FreeString( aatt->aatt_HookName );
	
TRACKMEM(("internalDeleteAudioAttachment: after afi_FreeString\n"));

TRACER(TRACE_INT,TRACE_ITEM|TRACE_ATTACHMENT,
		("internalDeleteAudioAttachment returns 0x%x\n", Result));
	return Result;
}

/**************************************************************/
/***** SWIs ***************************************************/
/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/startattachment
 |||	StartAttachment - Starts an attachment.
 |||
 |||	  Synopsis
 |||
 |||	    Err StartAttachment (Item Attachment, TagArg *tagList)
 |||
 |||	    Err StartAttachmentVA (Item Attachment, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure starts playback of an attachment, which may be an attached
 |||	    envelope or sample. This function is useful to start attachments that
 |||	    aren't started by StartInstrument() (e.g. attachments with the
 |||	    AF_ATTF_NOAUTOSTART flag set).
 |||
 |||	    An attachment started with StartAttachment() should be released with
 |||	    ReleaseAttachment() and stopped with StopAttachment() if necessary.
 |||
 |||	  Arguments
 |||
 |||	    Attachment                   The item number for the attachment.
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  Caveats
 |||
 |||	    Prior to V24, StartAttachment() did not support envelope attachments.
 |||
 |||	  See Also
 |||
 |||	    ReleaseAttachment(), StopAttachment(), LinkAttachments(),
 |||	    AttachSample(), AttachEnvelope()
 |||
 **/
int32 swiStartAttachment( Item Attachment, TagArg *tp )
{
	AudioAttachment *aatt;
	int32 Result = 0;
	
	aatt = (AudioAttachment *)CheckItem(Attachment, AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	if (tp) return AF_ERR_BADTAG;
	
/* Distinguish between Sample and Envelope Attachments. 940825 */
	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			Result = DSPPStartSampleAttachment( aatt, TRUE );
			break;
			
		case AF_ATT_TYPE_ENVELOPE:
			Result = DSPPStartEnvAttachment( aatt );
			break;
	}
	return Result;	
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/releaseattachment
 |||	ReleaseAttachment - Releases an attachment.
 |||
 |||	  Synopsis
 |||
 |||	    Err ReleaseAttachment (Item Attachment, TagArg *tagList)
 |||
 |||	    Err ReleaseAttachmentVA (Item Attachment, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure releases an attachment and is commonly used to release
 |||	    attachments started with StartAttachment(). ReleaseAttachment() causes an
 |||	    attachment in a sustain loop to enter release phase. Has no effect on
 |||	    attachment with no sustain loop, or not in its sustain loop.
 |||
 |||	  Arguments
 |||
 |||	    Attachment                   The item number for the attachment.
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    Prior to V24, ReleaseAttachment() did not support envelope attachments.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    StartAttachment(), StopAttachment(), LinkAttachments()
 |||
 **/
int32 swiReleaseAttachment( Item Attachment, TagArg *tp )
{
	AudioAttachment *aatt;
	int32 Result = 0;
	
	aatt = (AudioAttachment *)CheckItem(Attachment, AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	if (tp) return AF_ERR_BADTAG;
		
/* Distinguish between Sample and Envelope Attachments. 940825 */
	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			Result = DSPPReleaseSampleAttachment( aatt );
			break;
			
		case AF_ATT_TYPE_ENVELOPE:
			Result = DSPPReleaseEnvAttachment( aatt );
			break;
	}
	return Result;
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/stopattachment
 |||	StopAttachment - Stops an attachment.
 |||
 |||	  Synopsis
 |||
 |||	    Err StopAttachment (Item Attachment, TagArg *tagList)
 |||
 |||	    Err StopAttachmentVA (Item Attachment, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure abruptly stops an attachment. The attachment doesn't
 |||	    go into its release phase when stopped this way.
 |||
 |||	  Arguments
 |||
 |||	    Attachment                   The item number for the attachment.
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    Prior to V24, StopAttachment() did not support envelope attachments.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    StartAttachment(), ReleaseAttachment(), LinkAttachments()
 |||
 **/
int32 swiStopAttachment(  Item Attachment, TagArg *tp )
{
	AudioAttachment *aatt;
	int32 Result = 0;
	
	aatt = (AudioAttachment *)CheckItem(Attachment, AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	if (tp) return AF_ERR_BADTAG;
			
/* Distinguish between Sample and Envelope Attachments. 940825 */
	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			Result = DSPPStopSampleAttachment( aatt );
			break;
			
		case AF_ATT_TYPE_ENVELOPE:
			Result = DSPPStopEnvAttachment( aatt );
			break;
	}
	return Result;
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/linkattachments
 |||	LinkAttachments - Connects sample attachments for sequential playback.
 |||
 |||	  Synopsis
 |||
 |||	    Err LinkAttachments (Item Att1, Item Att2)
 |||
 |||	  Description
 |||
 |||	    This procedure specifies that the attachment Att2 will begin playing when
 |||	    attachment Att1 finishes. This is useful if you want to connect
 |||	    discontiguous sample buffers that are used for playing interleaved audio
 |||	    and video data. It is also a good way to construct big sound effects from
 |||	    a series of small sound effects.
 |||
 |||	    If Att1's sample has a sustain loop, but no release loop, you can follow
 |||	    this with a call to ReleaseAttachment (Att1, NULL) to smoothly transition
 |||	    to Att2.
 |||
 |||	    If Att1's sample has no loops, Att2 will automatically start as soon as
 |||	    Att1 completes (assuming that it has not completed prior to this function
 |||	    being called).
 |||
 |||	    If, after linking Att1 to Att2, StopAttachment() is called on Att1 before it
 |||	    finishes, Att2 will not be automatically started. StopAttachment() on Att1
 |||	    after it finishes has no effect.
 |||
 |||	    All link remains in effect for multiple calls to StartInstrument() or
 |||	    StartAttachment(). That is, if you call LinkAttachments (Att1, Att2),
 |||	    Att1 will flow into Att2 upon completion of Att1 for every subsequent
 |||	    call to StartAttachment (Att1, NULL) (or StartInstrument() on the instrument
 |||	    belonging to Att1) if Att1 would normally be automatically started by
 |||	    starting the instrument).
 |||
 |||	    An attachment (Att1) can link to no more than one attachment. An attachment
 |||	    (Att2) can be linked to multiple attachments. The most recent call to
 |||	    LinkAttachments() for Att1 takes precedence.
 |||
 |||	    The pair of Attachments passed to this function must satisfy all of these
 |||	    requirements:
 |||
 |||	    . Both Attachments must be Sample Attachments.
 |||
 |||	    . Both Attachments must be attached to the same Instrument.
 |||
 |||	    . Both Attachments must be attached to the same FIFO of that Instrument.
 |||
 |||	    Call LinkAttachments (Att1, 0) to remove a previous link from Att1.
 |||
 |||	    A link does not interfere with a Cue associated with an attachment.
 |||
 |||	    Deleting either Attachment, either Attachment's Sample, or the Instrument
 |||	    to which the Attachment belongs, breaks the link.
 |||
 |||	  Arguments
 |||
 |||	    Att1                         The item number for the attachment that is
 |||	                                 to finish. Must be a sample attachment.
 |||
 |||	    Att2                         The item number for the attachment that is
 |||	                                 to begin playing. Must be a Sample Attachment
 |||	                                 attached to the same FIFO on the same
 |||	                                 Instrument as Att1. Can be 0 to remove a link
 |||	                                 from Att1.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    In order for Att2 to start, Att1 must either be currently playing or
 |||	    yet to be played. If it has already completed by the time of this call,
 |||	    Att2 will not be started. 
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    StartAttachment(), ReleaseAttachment()
 |||
 **/

int32 internalLinkAttachments( AudioAttachment *aatt1, Item Att2 )
{
	int32 Result = 0;

/* Link them. */
	aatt1->aatt_NextAttachment = Att2;

/* Update Sample DMA and Interrupts. */
/* Check to ensure type sample. 940922 */
	if(aatt1->aatt_Type == AF_ATT_TYPE_SAMPLE)
	{
		switch( aatt1->aatt_ActivityLevel )
		{
			case AF_STARTED:
				Result = DSPPStartSampleAttachment( aatt1,  FALSE );
				break;
			
			case AF_RELEASED:
				Result = DSPPReleaseSampleAttachment( aatt1 );
				break;
		}
	}

	return Result;
}

int32 swiLinkAttachments( Item Att1, Item Att2 )
{
			
	AudioAttachment *aatt1, *aatt2 = NULL;
	
DBUG(("LinkAttachment: Att1 = 0x%x, Att2 = 0x%x\n", Att1, Att2 ));
	aatt1 = (AudioAttachment *)CheckItem(Att1,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt1 == NULL) return AF_ERR_BADITEM;

	if( Att2 )
	{
		aatt2 = (AudioAttachment *)CheckItem(Att2,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
		if (aatt2 == NULL) return AF_ERR_BADITEM;
	}

	return internalLinkAttachments( aatt1, Att2 );
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/whereattachment
 |||	WhereAttachment - Returns the current playing location attachment.
 |||
 |||	  Synopsis
 |||
 |||	    int32 WhereAttachment (Item Attachment)
 |||
 |||	  Description
 |||
 |||	    This procedure is useful for monitoring the progress of a sample or
 |||	    envelope that's being played. It returns a value indicating where playback
 |||	    is located in the attachment's sample or envelope. For sample attachments,
 |||	    returns the currently playing byte offset of within the sample. For
 |||	    envelope attachments, returns the currently playing segment index.
 |||
 |||	    A sample's offset starts at zero. Note that the offset is not measured in
 |||	    sample frames. You must divide the byte offset by the number of bytes per
 |||	    frame, then discard the remainder to find out which frame is being played.
 |||
 |||	  Arguments
 |||
 |||	    Attachment                   Item number of the attachment.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value indicating position (byte offset of sample or
 |||	    segment index of envelope) on success, negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    If a sample attachment has finished, the return value may be negative, or
 |||	    greater than the length of the sample. This is because the DMA hardware is
 |||	    pointing to a different sample.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    MonitorAttachment()
 |||
 **/
int32 swiWhereAttachment( Item Attachment )
{
	AudioAttachment	*aatt;
	AudioSample		*asmp;
	AudioEnvelope	*aenv;
	int8			*SampleBase;
	
	AudioEnvExtension *aeva;
	int32 Where;
	
DBUG(("swiWhereAttachment( Att = 0x%x)\n", Attachment ));
	aatt = (AudioAttachment *)CheckItem(Attachment,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	
	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			asmp = (AudioSample *) aatt->aatt_Structure;
			SampleBase = (int8 *) asmp->asmp_Data;
			Where = dsphReadChannelAddress(aatt->aatt_Channel) - SampleBase;
			break;
			
/* 931117 Support for envelopes added. */
		case AF_ATT_TYPE_ENVELOPE:
			aenv = (AudioEnvelope *) aatt->aatt_Structure;
			aeva = (AudioEnvExtension *) aatt->aatt_Extension;
			Where = aeva->aeva_CurIndex;
			break;
			
		default:
			Where = 0;
			break;
	}

DBUG(("swiWhereAttachment: dmar = 0x%x, Where = 0x%x\n", dmar, Where));
	return Where;
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/monitorattachment
 |||	MonitorAttachment - Monitors an attachment, sends a cue at a specified point.
 |||
 |||	  Synopsis
 |||
 |||	    Err MonitorAttachment (Item Attachment, Item Cue, int32 Index)
 |||
 |||	  Description
 |||
 |||	    This procedure sends a Cue to the calling task when the specified
 |||	    Attachment reaches the specified point. The procedure is often used
 |||	    with a sample Attachment to send a cue when the sample has been fully
 |||	    played.
 |||
 |||	    There can be only one Cue per Attachment. The most recent call to
 |||	    MonitorAttachment() takes precedence.
 |||
 |||	    To remove the current Cue from an Attachment, call
 |||	    MonitorAttachment (Attachment, 0, 0).
 |||
 |||	  Arguments
 |||
 |||	    Attachment                   The item number for the Attachment.
 |||
 |||	    Cue                          Item number for the Cue to be associated
 |||	                                 with this attachment. Can be 0 to remove
 |||	                                 a the current Cue from the Attachment.
 |||
 |||	    Index                        Value indicating the point to be monitored.
 |||	                                 At this time, the only value that can be
 |||	                                 passed is CUE_AT_END, which asks that a Cue be
 |||	                                 sent at the end of an Attachment.
 |||
 |||	                                 Index is ignored if Cue is 0.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    GetCueSignal(), StartAttachment(), LinkAttachments()
 |||	    WhereAttachment()
 |||
 **/
int32 swiMonitorAttachment( Item Attachment, Item Cue, int32 CueAt )
{
	AudioAttachment *aatt;
	AudioCue *acue;
		
DBUG(("swiMonitorAttachment( Att = 0x%x, Cue = 0x%x )\n", Attachment, Cue ));
	aatt = (AudioAttachment *)CheckItem(Attachment,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	
	
	if (Cue == 0)
	{
		aatt->aatt_CueItem = 0;
	}
	else
	{
		acue = (AudioCue *)CheckItem(Cue,  AUDIONODE, AUDIO_CUE_NODE);
		if (acue == NULL) return AF_ERR_BADITEM;
		if(CueAt != CUE_AT_END) return AF_ERR_UNIMPLEMENTED; /* 930929 */
		aatt->aatt_CueAt = CueAt;
		aatt->aatt_CueItem = Cue;
	}
	
	return 0;
}
			
/**************************************************************/
void EnableAttSignalIfNeeded( AudioAttachment *aatt )
{

DBUG(("EnableAttSignalIfNeeded( aatt = 0x%x )\n", aatt ));
DBUG(("EnableAttSignalIfNeeded: Cue = 0x%x, Next = 0x%x",
	aatt->aatt_CueItem, aatt->aatt_NextAttachment, aatt->aatt_Flags ));
DBUG((", Flags = 0x%x, Count = %d\n",
	aatt->aatt_Flags, aatt->aatt_SegmentCount ));

	if (aatt->aatt_CueItem ||
		aatt->aatt_NextAttachment ||
		((aatt->aatt_Flags & AF_ATTF_FATLADYSINGS) &&
			 (aatt->aatt_SegmentCount > 0)))
	{
		EnableAttachmentSignal( aatt );
	}
}

/***************************************************************
** Set interrupt so that it will send a signal to folio when complete.
***************************************************************/
int32  EnableAttachmentSignal ( AudioAttachment *aatt )
{
	AudioDMAControl *admac;
	int32 chan;

	chan = aatt->aatt_Channel;
DBUG(("EnableAttachmentSignal( aatt = 0x%x ) Channel = 0x%x \n", aatt, chan ));

	admac = &AudioBase->af_DMAControls[chan];
	admac->admac_SignalCountDown = 1;  /*  So it can go to zero and trigger signal */
	dsphEnableChannelInterrupt( chan );
	return 0;
}

/***************************************************************
** Set interrupt so that it will update NextAddr, NextCount.
***************************************************************/
int32  SetDMANextInt ( int32 DMAChan, int32 *Address, int32 Cnt )
{
	AudioDMAControl *admac;
	
DBUG(("SetDMANextInt( chan = %d, addr = 0x%x, cnt = %d )\n", DMAChan, Address, Cnt ));
	if( Address == NULL )
	{
		ERR(("SetDMANextInt: Address = NULL\n"));
		return -1;
	}
	
	admac = &AudioBase->af_DMAControls[DMAChan];
	
	dsphDisableChannelInterrupt( DMAChan );
	admac->admac_NextCountDown = 1;  /*  So it can go to zero and trigger signal */
	admac->admac_NextAddress = Address;
	admac->admac_NextCount = Cnt;
	dsphEnableChannelInterrupt( DMAChan );
	return 0;
}

/************************************j*************************/
int32  DisableAttachmentSignal ( AudioAttachment *aatt )
{
	AudioDMAControl *admac;
	int32 i;
DBUG(("DisableAttachment( aatt = 0x%x )\n", aatt ));
	i = aatt->aatt_Channel;
DBUG(("DisableAttachment: Channel = 0x%x )\n", i ));
	dsphDisableChannelInterrupt(i);
	admac = &AudioBase->af_DMAControls[i];
	admac->admac_SignalCountDown = 0;
	admac->admac_NextCountDown = 0;
	return 0;
}
