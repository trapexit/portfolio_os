/* $Id: dspp_instr.c,v 1.82.1.1 1995/01/27 03:04:38 phil Exp $ */
/****************************************************************
**
** DSPP Instrument Manager
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/*
** 921112 PLB Merge DRSC_OUTPUT and DRSC_I_MEM
** 921113 PLB Add DSPPFindResource, add DSPPConnectInstruments
** 921116 PLB DSPP Code now threaded, Fix bad DSPPRelocate
** 921119 PLB Use AllocThings and FreeThings for proper resource alloc.
**          Call DSPPStopInstrument in DSPPFreeInstrument.
** 921216 PLB Link code in DSPP in same order as in host list.
**          Stop sample after unlinking code.
** 921218 PLB Added AllocAmplitude and FreeAmplitude
** 921221 PLB Added DisconnectInstruments
** 921228 PLB Check for relocation offsets out of bounds to detect LF->CR
**            mangling of xxx.ofx by Mac.
** 930104 PLB Use SampleAttachments list to handle multiple attachments.
**            AttachSample now uses FIFOName.
** 930112 PLB Validate *Alloc in DSPGetRsrcAlloc
** 930126 PLB Change to use .dsp files instead of .ofx
** 930302 PLB StopInstrument does not set Amplitude=0 anymore.
** 930306 PLB Support compound 3INS instruments.
** 930311 PLB Free TemplateSample structures.
** 930323 PLB Switch to generic Attachment structure.
** 930326 PLB Use DSPPStartAttachment instead of DSPP_StartSample
** 930415 PLB Track connections between Items
** 930607 PLB Add tuning.
** 930612 PLB Added IMem access.
** 930616 PLB Added DSP Data Initialization
** 930703 PLB Added Release Loop Support
** 930728 PLB Do not play note if no sample in range.
** 930830 PLB Allow disconnection from Knobs
** 930907 PLB Add FIFOInit to DSPPStartSampleAttachment
** 931116 PLB Clear head of DSPP FIFO to prevent pops.
** 931117 PLB Instrument may have gotten stopped by DSPPReleaseEnvAttachment
**            so check before setting status to AF_RELEASED.   This was
**            causing a crash if the ReleasePoint on an envelope
**            was set to the last point.  Crash was due to double RemNode
**            of instrument when stopped the second time.
** 931130 PLB Fix bug with premature STOP for looping samples with FATLADYSINGS 
** 931201 PLB Rewrote DSPPReleaseSampleAttachment() to fix FATLADYSINGS problems
**				and to fix problem with missing samples between sustain loop and release loop.
** 931211 PLB Implemented PauseInstrument and ResumeInstrument
** 931216 PLB Check for input DMA Type before clearing FIFO HEAD
** 931220 PLB Added DSPPCalcSampleStart, rewrote DSPPStartSampleAttachmant and
**               DSPPNextAttachment to use it.
** 931221 PLB Trap Amplitude < 0 in AllocAmplitude() and FreeAmplitude()
** 940203 PLB Fix check for sample loop > 0
** 940502 PLB Add support for audio input.
** 940608 PLB Added DSPPAdjustTicksPerFrame()
** 940609 PLB Added shared library template support. 
** 940713 PLB Fixed DMA Channel for DSPPStopInstrument for Output channels.
**            It used to turn off any Input channel with the same index.
**            Same for DSPPReleaseInstrument().  
** 940901 PLB Change to new dsph prefix names for Anvil/Bulldog
** 940908 PLB Rewrote DumpList() so it doesn't crash on empty list. DEBUG mode only.
** 941024 PLB Trap negative DMA counts that can occur if AF_TAG_START_AT
**            is past loop end.
** 941031 PLB Clean up handling of dins_ActivityLevel flag.  It now
**            indicates whether an instrument's code is executing.
** 941101 PLB Prevent negative loop sizes from hanging DMA
** 941121 PLB Stop sample attachment if running when started.
** 941121 PLB Don't enable DMA interrupt if release loop in sample.
** 941130 PLB Restore Knob connection on DisconnectInstrument(), CR3763
** 950112 PLB Clear head of FIFO to prevent expansion bus hardware bug
**            from causing the playback of garbage data.
** 950126 PLB Overwrite SLEEP at beginning of HEAD code to enable execution.
**            This was moved from dspp_loader where it was causing premature
**            execution of "head.dsp".  CR4215
****************************************************************/

#include "audio_internal.h"
#include "touch_hardware.h"

/* #define DEBUG */
#define DBUG(x)      /* PRT(x) */
#define DBUGOFX(x)   DBUG(x)
#define DBUGNOTE(x)  DBUG(x)
#define DBUGSAMP(x)  DBUG(x)
#define DBUGLIST(x)  DBUG(x)
#define DBUGATT(x)   DBUG(x)
#define DBUGTUNE(x)  DBUG(x)
#define DBUGRATE(x)  DBUG(x)

/* #define COMPILE_DSPP_TRACE */

#define MIN_DMA_COUNT (4)
/*****************************************************************/
/******** Static Data ********************************************/
/*****************************************************************/

DSPPStatic DSPPData;

static uchar n_mem_table[LAST_N_MEM - FIRST_N_MEM + 1];
static uchar ei_mem_table[LAST_EI_MEM - FIRST_EI_MEM + 1];
static uchar i_mem_table[LAST_I_MEM - FIRST_I_MEM + 1];
static uchar eo_mem_table[LAST_EO_MEM - FIRST_EO_MEM + 1];
static uchar in_fifo_table[LAST_IN_FIFO - FIRST_IN_FIFO + 1];
static uchar out_fifo_table[LAST_OUT_FIFO - FIRST_OUT_FIFO + 1];

TableAllocator DSPPAllocators[DSPP_NUM_RSRC_TYPES] =
{    /* Size                             Offset                table,    many */
	{ LAST_N_MEM - FIRST_N_MEM + 1, OFFSET_N_MEM + FIRST_N_MEM, n_mem_table, 0},
	{ LAST_EI_MEM - FIRST_EI_MEM + 1, OFFSET_EI_MEM + FIRST_EI_MEM, ei_mem_table, 0},
	{ LAST_I_MEM - FIRST_I_MEM + 1,OFFSET_I_MEM + FIRST_I_MEM, i_mem_table, 0},
	{ LAST_EO_MEM - FIRST_EO_MEM + 1, OFFSET_EO_MEM + FIRST_EO_MEM, eo_mem_table, 0},
	{ 0, 0, NULL, 0 },   /* RBASE4 allocated from IMEM */
	{ 0, 0, NULL, 0 },   /* RBASE8 allocated from IMEM */
	{ LAST_IN_FIFO - FIRST_IN_FIFO + 1, OFFSET_IN_FIFO + FIRST_IN_FIFO, in_fifo_table, 0},
	{ LAST_OUT_FIFO - FIRST_OUT_FIFO + 1, OFFSET_OUT_FIFO + FIRST_OUT_FIFO, out_fifo_table, 0},
	{ 0, 0, NULL, 0 },   /* TICKS allocated from AudioBase */
	{ 0, 0, NULL, 0 },   /* Left ADC 940502 */
	{ 0, 0, NULL, 0 },   /* Right ADC 940502 */
	{ 0, 0, NULL, 0 }    /* Noise 940502 */
};

/****** End of Static Data ***************************************/
/*****************************************************************/
/****** Static Prototypes ***************************************/
/*****************************************************************/

static DSPPCodeList *DSPPSelectRunningList( DSPPInstrument *dins );
static Err DSPPLinkCodeToStartList( DSPPInstrument *dins, DSPPCodeList *dcls );
static Err DSPPUnlinkCodeToStopList( DSPPInstrument *dins, DSPPCodeList *dcls );
static void DSPPGetPrevNextIns( DSPPInstrument *dins, DSPPCodeList *dcls,
		DSPPInstrument **PrevDins, DSPPInstrument **NextDins );
static Err DSPPStopCodeExecution( DSPPInstrument *dins );
static Err DSPPStartCodeExecution( DSPPInstrument *dins );

/*****************************************************************/
/******* User Level Code *****************************************/
/*****************************************************************/
/*****************************************************************/
/* Folio Calls */
/* Where did this resource get allocated? */
int32 DSPGetRsrcAlloc (Item InstrumentItem, int32 RsrcType, char *Name, int32 *Alloc)
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	DSPPResource *drsc;
	int32 Result=0;
	
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	drsc = DSPPFindResource( dins, RsrcType, Name);
	if (drsc == NULL) return AF_ERR_BADNAME;
	
	*Alloc = drsc ->drsc_Allocated;

	return Result;
}

/*****************************************************************/
int32 DSPGetTotalRsrcUsed( int32 RsrcType )
{
	int32 Answer;
	if(RsrcType == DRSC_TICKS)
	{
/* Base answer on most heavily used frame. 940812 */
		Answer = DSPP_AVAIL_TICKS - MIN(DSPPData.dspp_AvailableTicks[0], DSPPData.dspp_AvailableTicks[1]);
	}
	else if ((RsrcType<DSPP_NUM_RSRC_TYPES) && (RsrcType>= 0))
	{
		Answer = DSPPAllocators[RsrcType].tall_Many;
	}
	else
	{
		Answer = AF_ERR_UNIMPLEMENTED;
	}
	return Answer;
}

/*****************************************************************/
int32 DSPGetInsRsrcUsed( Item InstrumentItem, int32 RsrcType)
{
	int32 i;
	int32 sum = 0;
	AudioInstrument *ains;
	DSPPInstrument *dins;
	DSPPResource *drsc, *drscarray;

	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;

	drscarray = dins->dins_Resources;
TRACEE(TRACE_INT,TRACE_OFX, ("DSPGetInsRsrcUsed ( dins=0x%lx, Type=0x%lx)\n",
	dins, RsrcType));
TRACEB(TRACE_INT,TRACE_OFX, ("DSPGetInsRsrcUsed: NumRsrc=$%lx\n", dins->dins_NumResources));

	for (i=0; i<dins->dins_NumResources; i++)
	{
		drsc = &drscarray[i];
TRACEB(TRACE_INT,TRACE_OFX, ("DSPGetInsRsrcUsed: i=%d, Type=$%lx, Alloc=$%lx\n",
			i, drsc->drsc_Type, drsc->drsc_Allocated));
		
		if(drsc->drsc_Type == RsrcType)
		{	
			sum += drsc->drsc_Many;
		}
	}
	
TRACER(TRACE_INT,TRACE_OFX, ("DSPGetInsRsrcUsed returns 0x%lx\n", sum));
	return sum;
}

/*****************************************************************/
/******* Supervisor Level Code ***********************************/
/*****************************************************************/

#define CalcTicksPerFrame(Rate) ((SYSTEM_CLOCK_RATE / Rate) - 1 )

int32 DSPP_Init( void )
{
	int32 i;
	
	dsphInitDSPP();

	InitList(&DSPPData.dspp_ExternalList, "DSPP Externals");
	
	InitList(&DSPPData.dspp_ActiveInstruments.dcls_InsList, "DSPP Active");
	InitList(&DSPPData.dspp_HalfRateInstruments[0].dcls_InsList, "DSPP Half Odd");
	InitList(&DSPPData.dspp_HalfRateInstruments[1].dcls_InsList, "DSPP Half Even");

/* Mark everything as free. */
	for (i=0; i<DSPP_NUM_RSRC_TYPES; i++)
	{
		ClearThings(&DSPPAllocators[i]);
	}
	
	DSPPData.dspp_AvailableAmplitude = MAXDSPAMPLITUDE;
	
/* SYSTEM_CLOCK_RATE should be gotten from MUD %Q */
#define SYSTEM_CLOCK_RATE (25000000)
	DSPPData.dspp_SampleRate = DEFAULT_SAMPLERATE;
	DSPPData.dspp_TicksPerFrame = CalcTicksPerFrame(DEFAULT_SAMPLERATE);

/* Set even and odd frames for half rate calculation. 940812 */
	DSPPData.dspp_AvailableTicks[0] = DSPPData.dspp_TicksPerFrame;
	DSPPData.dspp_AvailableTicks[1] = DSPPData.dspp_TicksPerFrame;
DBUG(("DSPPData.dspp_AvailableTicks = %d\n", DSPPData.dspp_AvailableTicks[0] ));
	
	return 0;
}

/******************************************************************
** Adjust Ticks available per frame when the sample rate changes.
** Return error if too many ticks already allocated. 940608
** Do both frames for half rate calculation. 940812
******************************************************************/
int32 DSPPAdjustTicksPerFrame( int32 SampleRate )
{
	int32    NewTicksPerFrame;
	int32    CurrentTicksInUse[DSPP_NUM_TICK_FRAMES];
	int32    i;
	int32    Result=0;
	
	NewTicksPerFrame = CalcTicksPerFrame( SampleRate );
	
/* Validate first so we don't change first then fail on second. */
	for( i=0; i<DSPP_NUM_TICK_FRAMES; i++ )
	{
		CurrentTicksInUse[i] = DSPPData.dspp_TicksPerFrame - DSPPData.dspp_AvailableTicks[i];
	
		if( CurrentTicksInUse[i] > NewTicksPerFrame )
		{
			ERR(("DSPPAdjustTicksPerFrame: too many ticks already allocated.\n"));
			return AF_ERR_NORSRC;
		}
	}
	
	DSPPData.dspp_TicksPerFrame = NewTicksPerFrame;
DBUG(("DSPPData.dspp_TicksPerFrame = %d\n", DSPPData.dspp_TicksPerFrame ));

	for( i=0; i<DSPP_NUM_TICK_FRAMES; i++ )
	{
		DSPPData.dspp_AvailableTicks[i] = NewTicksPerFrame - CurrentTicksInUse[i];
DBUG(("DSPPData.dspp_AvailableTicks = %d\n", DSPPData.dspp_AvailableTicks[i] ));
	}
	return Result;
}
	
/*****************************************************************/

int32 DSPP_Term( void )
{
	dsphTermDSPP();
	
	return 0;
}

/*****************************************************************/
static AudioAttachment *ScanMultiSamples( List *AttList, int32 Note )
{
  	AudioSample *samp;
	AudioAttachment *aatt, *ChosenAAtt;
	
/* Scan list of samples for first that matches range. */
	aatt = (AudioAttachment *)FirstNode(AttList);
	ChosenAAtt = NULL; /* default */
	while (ISNODE(AttList,aatt))
	{
		samp = (AudioSample *) aatt->aatt_Structure;
		if( Note >= samp->asmp_LowNote &&
			Note <= samp->asmp_HighNote )
		{
			ChosenAAtt = aatt;
			break;
		}
#if 0
/* NO, DON'T DO THIS! 930728 */
		else
		{
/* Keep track of closest in case no match. */
			Interval = Note - samp->asmp_BaseNote;
			if(Interval < 0) Interval = -Interval;
			if(Interval < SmallestInterval)
			{
				SmallestInterval = Interval;
				ChosenAAtt = aatt;
			}
		}
#endif

		aatt = (AudioAttachment *)NextNode((Node *)aatt);
	}
	return ChosenAAtt;
}

#if 0
/******************************************************************
** Init All IMem for an Instrument as an experiment.
******************************************************************/
Err DSPPInitAllIMem( DSPPInstrument *dins )
{
	int32 i, ReadValue;
	DSPPResource *drsc, *drscarray;
	Err Result = 0;
	
	if(!DSPPData.dspp_IMEMAccessOn) return 0;
	
	drscarray = dins->dins_Resources;
	
TRACEE(TRACE_INT,TRACE_OFX, ("DSPPInitAllIMem ( dins=0x%lx) , NumRsrc=0x%lx\n",
	dins, dins->dins_NumResources));
PRT(("DSPPInitAllIMem ( dins=0x%lx) , NumRsrc=0x%lx\n",
	dins, dins->dins_NumResources));

	for (i=0; i<dins->dins_NumResources; i++)
	{
	
		drsc = &drscarray[i];
		
		
		if((drsc->drsc_Type == DRSC_I_MEM) && (drsc->drsc_Many == 1))
		{
		
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPInitAllIMem: i=%d, Type=$%lx, Alloc=$%lx\n",
			i, drsc->drsc_Type, drsc->drsc_Allocated));
DBUG(("DSPPInitAllIMem: i=%d, Type=$%lx, Alloc=$%lx\n",
			i, drsc->drsc_Type, drsc->drsc_Allocated));
			
			Result = DSPP_AccessIMem( drsc->drsc_Allocated, 0, drsc->drsc_Allocated, &ReadValue );
			if(Result < 0)
			{
				ERR(("DSPPInitAllIMem: error = 0x%lx\n", Result));
				return Result;
			}

		}
	}
	
TRACER(TRACE_INT,TRACE_OFX, ("DSPPInitAllIMem returns 0x%lx\n", Result));
	return Result;
}
#endif

/********************************************************************
** Start execution on DSPP of instrument.
** Handle special cases of Head, Tail, and Split.
********************************************************************/
static Err DSPPStartCodeExecution( DSPPInstrument *dins )
{
	int32 Result = 0;
	DSPPInstrument *LastDins;
	
DBUGRATE(("DSPPStartCodeExecution: dins = 0x%x, Specialness = %d\n", dins, dins->dins_Specialness ));
/* Activate code if not already. 941031 */
	if (dins->dins_ActivityLevel > AF_STOPPED) return 0;

	switch( dins->dins_Specialness )
	{
		case AF_SPECIAL_NOT:
			Result = DSPPLinkCodeToStartList( dins, DSPPSelectRunningList( dins ) );  /* Most common. */
			break;
			
		case AF_SPECIAL_HEAD:
			DSPPData.dspp_ActiveInstruments.dcls_ListHead = dins;
/* Overwrite SLEEP at beginning of HEAD code to enable execution. 950126 */
			dsphWriteCodeMem(dins->dins_EntryPoint, DSPP_NOP_OPCODE);
 			break;
			
		case AF_SPECIAL_TAIL:
			DSPPData.dspp_ActiveInstruments.dcls_ListTail = dins;
			DSPPData.dspp_HalfRateInstruments[0].dcls_ListTail = dins;
			DSPPData.dspp_HalfRateInstruments[1].dcls_ListTail = dins;
/* Connect Head to Tail, list should be empty. */
			if( DSPPData.dspp_ActiveInstruments.dcls_ListHead )
			{
				DSPPJumpTo( DSPPData.dspp_ActiveInstruments.dcls_ListHead, dins );
			}
			break;
			
		case AF_SPECIAL_SPLIT:
DBUGRATE(("DSPPStartCodeExecution: SPLIT dins = 0x%x\n", dins ));
			DSPPData.dspp_ActiveInstruments.dcls_ListTail = dins;
			DSPPData.dspp_HalfRateInstruments[0].dcls_ListHead = dins;
			DSPPData.dspp_HalfRateInstruments[1].dcls_ListHead = dins;
/* Connect split to tails. */
			if( DSPPData.dspp_HalfRateInstruments[0].dcls_ListTail )
			{
				DSPPJumpTo( dins, DSPPData.dspp_HalfRateInstruments[0].dcls_ListTail );
			}
			if( DSPPData.dspp_HalfRateInstruments[1].dcls_ListTail )
			{
				DSPPJumpFromSplitTo( dins, DSPPData.dspp_HalfRateInstruments[1].dcls_ListTail );
			}
/* Connect last in FullRate list to split. */
			if( ISEMPTYLIST(&DSPPData.dspp_ActiveInstruments.dcls_InsList) )
			{
				LastDins = DSPPData.dspp_ActiveInstruments.dcls_ListHead;
			}
			else
			{
				LastDins = (DSPPInstrument *) LASTNODE( &DSPPData.dspp_ActiveInstruments.dcls_InsList );
			}
			DSPPJumpTo( LastDins, dins );
			break;
	}
	
	dins->dins_ActivityLevel = AF_STARTED;
	
	return Result;
}

/*****************************************************************/
int32 DSPPStartInstrument( AudioInstrument *ains, TagArg *args )
{
	DSPPInstrument *dins;
  	uint32 tagc, *tagp, tagv;
	AudioAttachment *aatt, *ChosenAAtt;
	int32 Amplitude, IfConvert;
	ufrac16 Freq;
	int32 IfFreqSet;
	ufrac16 TimeScale;
	int32 IfTimeScaleSet;
	Node *n;
	List *AttList;
	int32 i, StartIndex;
	int32 Result;
	AudioKnob *aknob;

	dins = (DSPPInstrument *) ains->ains_DeviceInstrument;
	Amplitude = -1;
	IfFreqSet = FALSE;
	IfTimeScaleSet = FALSE;
	TimeScale = Convert32_F16(1);
	ChosenAAtt = NULL;
	IfConvert = FALSE;
	Result = 0;
	
TRACEE(TRACE_INT,TRACE_DSP|TRACE_NOTE,("DSPPStartInstrument ( dins=0x%lx, args=0x%lx\n",
	dins, args));

/* Stop the executing DSP code. 941031 */
	Result = DSPPStopCodeExecution( dins );
	if( Result < 0 ) return Result;

	tagp = (uint32 *)args;
	if (tagp)
	{
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPStartInstrument: tagp = $%lx\n", tagp));
		while ((tagc = *tagp++) != 0)
		{
			tagv = *tagp++;
			
			switch (tagc)
			{
			case AF_TAG_AMPLITUDE:
				Amplitude = tagv;
				break;
			case AF_TAG_VELOCITY:
				Amplitude = (tagv) << 8;
				break;
			case AF_TAG_FREQUENCY:
				Freq = tagv;
				IfFreqSet = TRUE;
				IfConvert = TRUE;
				break;
			case AF_TAG_RATE:
				Freq = (ufrac16) tagv;
				IfFreqSet = TRUE;
				IfConvert = FALSE;
				break;
			case AF_TAG_PITCH:
				if (dins->dins_NumFIFOs)
				{
					AttList = &dins->dins_FIFOControls[0].fico_Attachments;
					ChosenAAtt = ScanMultiSamples( AttList, tagv );
					if(ChosenAAtt == NULL) return 1;  /* No sample matched! 930728 */
#ifdef PARANOID
	ItemStructureParanoid(ChosenAAtt, AUDIO_ATTACHMENT_NODE,
		"DSPPStartInstrument, Attachment");
#endif
					Freq = CalcSampleNoteRate( ains, (AudioSample *) ChosenAAtt->aatt_Structure, tagv);
					IfConvert = FALSE;
				}
				else
				{
					Result = PitchToFrequency( GetInsTuning(ains), tagv, &Freq);
					IfConvert = TRUE;
				}
				IfFreqSet = TRUE;
				break;
				
			case AF_TAG_TIME_SCALE:
				TimeScale = (ufrac16) tagv;
				IfTimeScaleSet = TRUE;
				break;
				
			default:
				ERR (("Unrecognized tag in StartInstrument, tag = 0x%lx, val = 0x%lx\n",
				 tagc, tagv));
				return AF_ERR_BADTAG;
				break;
			}
		}
	}
	
	aknob = &dins->dins_FreqKnob;
	if((aknob->aknob_DeviceInstrument != 0) && (IfFreqSet))
	{
		ains->ains_OriginalRate = Freq;
		if(ains->ains_Bend != Convert32_F16(1))
		{
			Freq = MulSF16( Freq, ains->ains_Bend );
		}
		
		DSPPPutKnob(dins, aknob->aknob_DeviceKnob, Freq,
			&aknob->aknob_CurrentValue, IfConvert);
	}		

	aknob = &dins->dins_AmplitudeKnob;
	if((aknob->aknob_DeviceInstrument != 0) && (Amplitude >= 0))
	{
		DSPPPutKnob(dins, aknob->aknob_DeviceKnob,
			Amplitude, &aknob->aknob_CurrentValue,  TRUE);
	}

/* Test the result of initialization before start. */
/*	Result = DSPPInitAllIMem( dins ); */
/*	if (Result) return Result; */
	
/* Start chosen sample on first FIFO */
	StartIndex = 0;
	if(ChosenAAtt)
	{
		if ((ChosenAAtt->aatt_Flags & AF_ATTF_NOAUTOSTART) == 0)
		{
			DSPPStartSampleAttachment( ChosenAAtt, TRUE );
		}
		else
		{	
/*
** 950112 Clear FIFO and head to prevent initial pops
** that can occur with Opera when Expansion bus completes and accidentally
** sets wrong FIFOSTATUS.  This was added as a workaround for a hardware bug.
*/
			dsphResetFIFO( ChosenAAtt->aatt_Channel );
		}
		StartIndex = 1;
	}

/* Scan all remaining FIFOs and start a sample on each. */
	for (i=StartIndex; i<dins->dins_NumFIFOs; i++)
	{
		AttList = &dins->dins_FIFOControls[i].fico_Attachments;
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPStartInstrument: AttList = 0x%lx\n", AttList));
		n = FirstNode(AttList);
		while ((n != NULL) && ISNODE(AttList,n))  /* YES, CHECK FOR NULL ! */
		{
			aatt = (AudioAttachment *) n;
			n = NextNode(n);
			if ((aatt->aatt_Flags & AF_ATTF_NOAUTOSTART) == 0)
			{
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPStartInstrument: start 0x%lx\n", aatt));
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPStartInstrument: channel =  0x%lx\n", aatt->aatt_Channel));
				DSPPStartSampleAttachment( aatt, TRUE );
				dins->dins_FIFOControls[i].fico_CurrentAttachment = aatt;
				n = NULL;  /* Stop list traversal. */
			}
			else
			{
				dsphResetFIFO( aatt->aatt_Channel ); /* 950112 */
			}
		}
	}
	
/* Start all envelopes. */
	AttList = &dins->dins_EnvelopeAttachments;
	n = FirstNode( AttList );
	while ( ISNODE(AttList,n))
	{
		aatt = (AudioAttachment *) n;
		if(IfTimeScaleSet) SetEnvAttTimeScale( aatt, TimeScale );
		n = NextNode(n);
		if ((aatt->aatt_Flags & AF_ATTF_NOAUTOSTART) == 0)
		{
TRACEB(TRACE_INT, TRACE_ENVELOPE|TRACE_NOTE, ("DSPPStartInstrument: env start 0x%lx\n", aatt));
			DSPPStartEnvAttachment( aatt );
		}
	}

/* Initialize Instrument I Memory for start */
	Result = DSPPInitInsMemory( dins, DINI_AT_START );
	if (Result != 0) return Result;

/* Link code into execution chain of DSPP. */
	Result = DSPPStartCodeExecution( dins );
	
TRACER(TRACE_INT,TRACE_DSP|TRACE_NOTE,("DSPPStartInstrument returns 0\n"));

	return Result;
}

/*****************************************************************/
int32 DSPPPauseInstrument( DSPPInstrument *dins )
{
/* Stop the executing DSP code. */
	if( dins->dins_Specialness > 0 )
	{
		ERR(("DSPPPauseInstrument: called specialness = %d\n", dins->dins_Specialness ));
		return -1; /* %QQ need better error */
	}

	return DSPPStopCodeExecution( dins );
}
/*****************************************************************/
int32 DSPPResumeInstrument( DSPPInstrument *dins )
{
/* Link code into execution chain. */
	if( dins->dins_Specialness > 0 )
	{
		ERR(("DSPPResumeInstrument: called specialness = %d\n", dins->dins_Specialness ));
		return -1; /* %QQ need better error */
	}
	return DSPPStartCodeExecution( dins );
}

/********************************************************************
**  Calculate DMA values for startup.
**
**   Loop? Rels? => Do
**     Y     x      Set Cur and Next to This
**     N     Y      Set Cur to This, Set Next to RlsLoop
**     N     N      Set Cur to This, Set Next to NULL
*/
/*******************************************************************/
int32	DSPPCalcSampleStart ( AudioAttachment *aatt, SampleDMA *sdma )
{
	AudioSample *asmp;
	
	asmp = (AudioSample *) aatt->aatt_Structure;

/* Calculate starting address. */
	if(aatt->aatt_StartAt)
	{
		sdma->sdma_Address = (int32 *) (((char *) asmp->asmp_Data) +
			CvtFrameToByte(aatt->aatt_StartAt, asmp));
	}
	else
	{
		sdma->sdma_Address = (int32 *) asmp->asmp_Data;
	}

	if (asmp->asmp_SustainBegin >= 0)
	{
/* Sustain Loop */
		sdma->sdma_Count = CvtFrameToByte(asmp->asmp_SustainEnd - aatt->aatt_StartAt, asmp) - 4;
		sdma->sdma_NextAddress = (int32 *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_SustainBegin, asmp));	
		sdma->sdma_NextCount  = CvtFrameToByte((asmp->asmp_SustainEnd -
						asmp->asmp_SustainBegin), asmp) - 4;
	}
	else if (asmp->asmp_ReleaseBegin >= 0)
	{
/* No Sustain Loop, but a Release Loop */
		sdma->sdma_Count      = CvtFrameToByte(asmp->asmp_ReleaseEnd - aatt->aatt_StartAt, asmp) - 4;
		sdma->sdma_NextAddress = (int32 *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_ReleaseBegin, asmp));	
		sdma->sdma_NextCount  = CvtFrameToByte((asmp->asmp_ReleaseEnd -
						asmp->asmp_ReleaseBegin), asmp) - 4;
	}
	else
	{
/* No Loop, play whole thing. */
		sdma->sdma_Count      =  CvtFrameToByte(asmp->asmp_NumFrames - aatt->aatt_StartAt, asmp) - 4;
		sdma->sdma_NextAddress = NULL;
		sdma->sdma_NextCount = 0;
	}

/* Prevent negative counts that can occur if StartAt is beyond loop end! 941024 */
/* Prevent negative counts that can occur if SustainBegin == SustainEnd! 941101 */
DBUG(("sdma->sdma_Count = %d\n", sdma->sdma_Count ));
	if( (int32) sdma->sdma_Count < 0 )
	{
		sdma->sdma_Address = (int32 *)(((char *)sdma->sdma_Address) + sdma->sdma_Count);

/* Prevent sample access out of range. 941101 */
		if( (uint32) sdma->sdma_Address < (uint32) asmp->asmp_Data )
		{
			sdma->sdma_Address = asmp->asmp_Data; 
		}
		sdma->sdma_Count = 0;
	}

/* Prevent negative counts that can occur if ReleaseBegin == ReleaseEnd 941101 */
DBUG(("sdma->sdma_NextCount = %d\n", sdma->sdma_NextCount ));
	if( (int32) sdma->sdma_NextCount < 0 )
	{
		sdma->sdma_NextAddress = (int32 *)(((char *)sdma->sdma_NextAddress) + sdma->sdma_NextCount);
/* Prevent sample access out of range. 941101 */
		if( (uint32) sdma->sdma_NextAddress < (uint32) asmp->asmp_Data )
		{
			sdma->sdma_NextAddress = asmp->asmp_Data; /* Prevent sample access out of range. */
		}
		sdma->sdma_NextCount = 0;
	}

	return(0);
}

/********************************************************************
**  Start playing sample.
**  Setup DMA channel to play sample and loop if appropriate.
**  IfFullStart means that the DMA is starting from a dead stop
**      so we need to set initial DMA pointers. Otherwise we
**      just update the Next addresses and Interrupts.
*/
/*******************************************************************/
int32	DSPPStartSampleAttachment ( AudioAttachment *aatt, int32 IfFullStart )
{
	AudioSample *asmp;
	SampleDMA SDMA;
	AudioAttachment *nextaatt = NULL;
	AudioDMAControl *admac;
	int32 DMAChan, IfLoops;

#ifdef PARANOID
	ItemStructureParanoid(aatt, AUDIO_ATTACHMENT_NODE,
		"DSPPStartSampleAttachment");
#endif

	asmp = (AudioSample *) aatt->aatt_Structure;
	
#ifdef PARANOID
	ItemStructureParanoid(asmp, AUDIO_SAMPLE_NODE,
		"DSPPStartSampleAttachment");
#endif

	DMAChan = aatt->aatt_Channel;
	
#ifdef PARANOID
	if((DMAChan < 0) || (DMAChan >= NUM_AUDIO_DMA_CHANNELS))
	{
		ERR(("Paranoid: DSPPStartSampleAttachment: DMAChan = %d !!\n", DMAChan));
	}
#endif

	if (asmp->asmp_Data == NULL)
	{
		ERR(("Sample has NULL data address.\n"));
		return AF_ERR_NULLADDRESS;
	}
	if (asmp->asmp_NumBytes < MIN_DMA_COUNT)
	{
		ERR(("Sample has NumBytes = 0.\n"));
		return AF_ERR_OUTOFRANGE;
	}

/* Stop if not already. 941121 */
	if( aatt->aatt_ActivityLevel > AF_STOPPED )
	{
		DSPPStopSampleAttachment( aatt );
	}

	aatt->aatt_SegmentCount = 0;  /* 931130 Clear initially */
	admac = &AudioBase->af_DMAControls[DMAChan];
	admac->admac_AttachmentItem = aatt->aatt_Item.n_Item;
#if 0
#define REPORT_ADMAC(name,member) PRT(("admac->%s = 0x%lx\n", name, admac->member));
	REPORT_ADMAC("admac_ChannelAddr", admac_ChannelAddr);
	REPORT_ADMAC("admac_NextCountDown", admac_NextCountDown);
	REPORT_ADMAC("admac_NextCount", admac_NextCount);
	REPORT_ADMAC("admac_SignalCountDown", admac_SignalCountDown);
	REPORT_ADMAC("admac_Signal", admac_Signal);
	REPORT_ADMAC("admac_AttachmentItem", admac_AttachmentItem);
#endif

/* Figure out sample addresses, return in SDMA. */
	DSPPCalcSampleStart( aatt, &SDMA );
	
	if( SDMA.sdma_NextAddress != NULL )
	{
		IfLoops = TRUE;
	}
	else
	{
/* No loops so set up silence or scratch for later DMA. */
		if( IsDMATypeOutput(DMAChan) )
		{
			SDMA.sdma_NextAddress = (int32 *) ScratchRAM;    /* Output DMA, don't overwrite silence. */
		}
		else
		{
			SDMA.sdma_NextAddress = (int32 *) Silence;
		}
		SDMA.sdma_NextCount  = SILENCE_SIZE-4;
		IfLoops = FALSE;
		aatt->aatt_SegmentCount = 1; /* 931130 */
	}
	
	if( IfFullStart )
	{

/* start full dma */
		dsphResetFIFO( DMAChan );
		dsphSetFullDMA( DMAChan, SDMA.sdma_Address, SDMA.sdma_Count,
			SDMA.sdma_NextAddress, SDMA.sdma_NextCount );
DBUG(("Start ATT: 0x%lx, DMAChan = 0x%lx\n", aatt, DMAChan ));
	}
	else
	{
/* Just set loop registers to be picked up on next dma completion. */
DBUG(("SSA: NA=0x%lx, NC=0x%lx\n", SDMA.sdma_NextAddress, SDMA.sdma_NextCount));
		dsphSetDMANext( DMAChan, SDMA.sdma_NextAddress, SDMA.sdma_NextCount );
	}
	
/* setup next attachment if there is one. */
	if (!IfLoops)
	{
		nextaatt = afi_CheckNextAttachment( aatt );
		if(nextaatt)
		{
			DSPPNextAttachment( nextaatt );
		}
		EnableAttSignalIfNeeded(aatt);
	}
	
	aatt->aatt_ActivityLevel = AF_STARTED;
	return(0);
}

#define GetSilencePointer(DMAChan) ((DMAChan > NUM_AUDIO_INPUT_DMAS) ? ScratchRAM : Silence)
/********************************************************************/
void DSPP_SilenceDMA( int32 DMAChan )
{
	int32 *Ptr;
	
	Ptr = (int32 *) GetSilencePointer(DMAChan);
	
	dsphSetFullDMA(DMAChan, Ptr, SILENCE_SIZE-4,
			Ptr, SILENCE_SIZE-4 );
}
/********************************************************************/
void DSPP_SilenceNextDMA( int32 DMAChan )
{
	int32 *Ptr;
	
	Ptr = (int32 *) GetSilencePointer(DMAChan);
	dsphSetDMANext(DMAChan, Ptr, SILENCE_SIZE-4);
}

/********************************************************************/
void dsppQueueSilenceDMA( int32 DMAChan )
{
	int32 *Ptr;
	
	Ptr = (int32 *) GetSilencePointer(DMAChan);
DBUG(("dsppQueueSilenceDMA: Ptr = 0x%08x\n", Ptr));
	SetDMANextInt(DMAChan, Ptr, SILENCE_SIZE-4);
}

/********************************************************************/
/* Queue up for interrupt */
void DSPPQueueAttachment( AudioAttachment *aatt )
{
	SampleDMA SDMA;
	
	DSPPCalcSampleStart( aatt, &SDMA );
DBUG(("DSPPQueueAttachment: sdma_Address = 0x%08x\n", SDMA.sdma_Address));
	if( SDMA.sdma_Address != NULL )
	{
		SetDMANextInt( aatt->aatt_Channel, SDMA.sdma_Address, SDMA.sdma_Count );
	}
}

/********************************************************************/
/* Make this Attachment the next DMA block. */
void DSPPNextAttachment( AudioAttachment *aatt )
{
	SampleDMA SDMA;
		
	DSPPCalcSampleStart( aatt, &SDMA );
	
	if( SDMA.sdma_Address != NULL )
	{
		dsphSetDMANext ( aatt->aatt_Channel, SDMA.sdma_Address, SDMA.sdma_Count );
		if( SDMA.sdma_NextAddress != NULL )
		{
DBUG(("DSPPNextAttachment: sdma_NextAddress = 0x%08x\n", SDMA.sdma_NextAddress));
			SetDMANextInt ( aatt->aatt_Channel, SDMA.sdma_NextAddress, SDMA.sdma_NextCount );
		}
	}
	
DBUG(("DSPPNextAttachment: 0x%lx,0x%lx\n", SDMA.sdma_Address, SDMA.sdma_Count));
}

/*******************************************************************/
AudioAttachment *afi_CheckNextAttachment( AudioAttachment *aatt )
{
	AudioAttachment *nextaatt;
	
/* Is there a valid next attachment? */
	if ( aatt->aatt_NextAttachment )
	{
		nextaatt = (AudioAttachment *)CheckItem(aatt->aatt_NextAttachment,
			AUDIONODE, AUDIO_ATTACHMENT_NODE);
		if (nextaatt == NULL)
		{
/* Just eliminate the reference. */
			aatt->aatt_NextAttachment = 0;
		}
	}
	else
	{
		nextaatt = NULL;
	}
	return nextaatt;
}

/********************************************************************
**  Set for "release" portion of sample.
**
** YES Release Loop............
**  If there is a release loop, then the next attachment will never be reached.
**  Rels? = Is there a gap between the end of the sustain loop and start of release loop?
**   Next? Loop? Rels? => Do
**     N     Y     Y      Set Next to Release Phase, Queue Release Loop
**     N     N     x      Set Next to Release Loop
**     N     Y     N      Set Next to Release Loop
**
** NO Release Loop............
**  Rels? = Is there a gap between the end of the sustain loop and last frame?
**   Next? Loop? Rels? => Do
**     Y     Y     Y      Set Next to Release Phase, Queue NextAttachment
**     N     Y     Y      Set Next to Release Phase, Queue Silence
**
**     Y     N     x      Set Next to NextAttachment, Queue whatever
**     Y     Y     N      Set Next to NextAttachment, Queue whatever
**
**     N     N     x      Set Next to Silence
**     N     Y     N      Set Next to Silence
**

*/
/*  Pseudocode ...................
	if(release loop)
		calc rels?
		if(sustain loop and rels?)
			Set Next to Release Phase
			Queue Release Loop, SC=0
		else
			Set Next to Release Loop, SC=0
		endif
	else
		calc rels?
		if(sustain loop and rels?)
			Set Next to Release Phase
			if(nextatt)
				Queue NextAttachment, SC=2
			else
				Queue Silence, SC=2
			endif
		else
			if(nextatt)
				Set Next to NextAttachment, Queue whatever, SC=1
			else
				Set Next to Silence, SC=1
			endif
		endif
	endif
*/
int32 DSPPReleaseSampleAttachment( AudioAttachment *aatt )
{
	AudioSample *asmp;
	AudioAttachment *nextaatt = NULL;
	int32 *Addr, Cnt;
	int32 *RlsLoopAddr, RlsLoopCnt;
	int32 DMAChan;
	
DBUG(("DSPPReleaseSampleAttachment: 0x%08x\n", aatt ));

	asmp = (AudioSample *) aatt->aatt_Structure;
	if ((AudioSample *) asmp->asmp_Data == NULL)
	{
		ERR(("Sample has NULL data address.\n"));
		return AF_ERR_NULLADDRESS;
	}

	nextaatt = afi_CheckNextAttachment( aatt );
	DMAChan = aatt->aatt_Channel;
	
	if (asmp->asmp_ReleaseBegin >= 0)    /* Release Loop? */
	{
		RlsLoopAddr = (int32 *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_ReleaseBegin, asmp));
		RlsLoopCnt  = CvtFrameToByte((asmp->asmp_ReleaseEnd -
						asmp->asmp_ReleaseBegin), asmp) - 4;
/* Prevent negative loop size from hanging DMA. 941101 */
		if( RlsLoopCnt < 0 ) RlsLoopCnt = 0;

/* Does this sample loop and have release gap? */
		if ((asmp->asmp_SustainBegin >= 0) &&   /* 940203 was >0 */
			(asmp->asmp_SustainEnd != asmp->asmp_ReleaseEnd) &&
			(asmp->asmp_SustainEnd != asmp->asmp_ReleaseBegin) &&
			(asmp->asmp_SustainBegin != asmp->asmp_ReleaseEnd) &&
			(asmp->asmp_SustainBegin != asmp->asmp_ReleaseBegin))
		{
/* Set Next to Release Phase */
			Addr = (int32 *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_SustainEnd, asmp));	
			Cnt  = CvtFrameToByte((asmp->asmp_ReleaseEnd -
						asmp->asmp_SustainEnd), asmp) - 4;

/* Queue Release Loop, SC=0 */
			if( aatt->aatt_ActivityLevel > AF_RELEASED )
			{
				dsphSetDMANext(DMAChan, Addr, Cnt);
			}
DBUG(("DSPPReleaseSampleAttachment: RlsLoopAddr = 0x%08x\n", RlsLoopAddr));
			SetDMANextInt ( DMAChan, RlsLoopAddr, RlsLoopCnt );
		}
		else
		{
/* Set Next to Release Loop, SC=0 */
			if( aatt->aatt_ActivityLevel > AF_RELEASED )
			{
				dsphSetDMANext(DMAChan, RlsLoopAddr, RlsLoopCnt);
			}
		}
		aatt->aatt_SegmentCount = 0;
	}
	else /* NO release loop */
	{	
		Cnt  = CvtFrameToByte((asmp->asmp_NumFrames -
				asmp->asmp_SustainEnd), asmp) - 4;
				
		aatt->aatt_SegmentCount = 1;
/* Does this sample loop and have release gap? */
		if ((asmp->asmp_SustainBegin >= 0) &&   /* 940203 was >0 */
			(Cnt >= MIN_DMA_COUNT))
		{
/* Set Next to Release Phase */
			Addr = (int32 *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_SustainEnd, asmp));	
			if( aatt->aatt_ActivityLevel > AF_RELEASED )
			{
				dsphSetDMANext(DMAChan, Addr, Cnt);
				aatt->aatt_SegmentCount = 2;
			}
			
			if(nextaatt)
			{
				DSPPQueueAttachment( nextaatt );
			}
			else
			{
				dsppQueueSilenceDMA( DMAChan );
			}
		}
		else
		{
			if(nextaatt)
			{
				DSPPNextAttachment( nextaatt );
			}
			else
			{
				DSPP_SilenceNextDMA( aatt->aatt_Channel );
			}
		}

/*
** Only enable interrupts if there is NO release loop. Release loops go forever
** so there is no point in interrupting.  It will never signal app.
*/
		EnableAttSignalIfNeeded( aatt );
	}
	
	aatt->aatt_ActivityLevel = AF_RELEASED;
	return(0);
}

#ifdef DEBUG
/******************************************************************
** Dump nodes in a list.
** 940908 PLB Rewrote so it doesn't crash on empty list.
******************************************************************/
void DumpList ( List *theList )
{
	Node *n;
	
	n = (Node *) FirstNode( theList );
	while (ISNODE( theList, (Node *) n))
	{
		PRT(("Node = 0x%lx\n", n));
		n = (Node *) NextNode((Node *)n);
	}
}
#endif

/*****************************************************************/
static DSPPCodeList *DSPPSelectRunningList( DSPPInstrument *dins )
{
	DSPPCodeList *dcls = NULL;
	
	if( dins->dins_RateShift == 0 )
	{
DBUG(("DSPPCodeList: use dspp_ActiveInstruments\n"));
		dcls = &DSPPData.dspp_ActiveInstruments;
	}
	else
	{
DBUG(("DSPPCodeList: use dspp_HalfRateInstruments[%d]\n", dins->dins_ExecFrame));
		dcls = &DSPPData.dspp_HalfRateInstruments[dins->dins_ExecFrame];
	}
	return dcls;
}

/******************************************************************
** Determine instruments before and after instrument in list.
** May be other non-special instruments, or list head and tail.
******************************************************************/
static void DSPPGetPrevNextIns( DSPPInstrument *dins, DSPPCodeList *dcls,
		DSPPInstrument **PrevDins, DSPPInstrument **NextDins )
{
	List *RunningList;
	DSPPInstrument *FirstDins, *LastDins;
	
	RunningList = &dcls->dcls_InsList;
	
DBUGRATE(("DSPPGetPrevNextIns: dcls = 0x%x\n", dcls ));
/* Link DSP code execution in same order as it appears in the host list.
** First make this instrument point to existing which is safe cuz this is
** not executing yet.
*/
	LastDins = (DSPPInstrument *) LASTNODE(RunningList);
	if (LastDins != dins )  /* nodes after this one so jump to next */
	{
		*NextDins = (DSPPInstrument *) NEXTNODE(dins);
	}
	else
	{
		*NextDins = dcls->dcls_ListTail;
	}
 
	FirstDins = (DSPPInstrument *) FIRSTNODE(RunningList);
	if (FirstDins != dins )  /* nodes before this one so make previous jump to new instrument */
	{
		*PrevDins = (DSPPInstrument *) PREVNODE(dins);
	}
	else
	{
		*PrevDins = dcls->dcls_ListHead;
	}
DBUGRATE(("DSPPGetPrevNextIns: PrevDIns = 0x%x, NextDins = 0x%x\n", *PrevDins, *NextDins ));
}

/******************************************************************
** Link Code into execution chain.
** This should only be called from DSPPStartCodeExecution() since it
** manages the dins_ActivityLevel flag.
******************************************************************/
static Err DSPPLinkCodeToStartList( DSPPInstrument *dins, DSPPCodeList *dcls )
{
	List *RunningList;
	DSPPInstrument *NextDins = (DSPPInstrument *) -1, *PrevDins = (DSPPInstrument *) -1;
	
	RunningList = &dcls->dcls_InsList;
	
/* If list empty, add to head.
** if list has one instrument, jump first to new, add after head.
** If list has two or more, jump new to second, jump first to new,
**	add after head.
*/	
TRACEB(TRACE_INT,TRACE_DSP|TRACE_NOTE,("DSPPLinkCodeToStartList: Priority = %d\n", dins->dins_Node.n_Priority));
DBUGRATE(("DSPPLinkCodeToStartList: Priority = %d\n", dins->dins_Node.n_Priority));
/* %Q This should use HeadInsertNode but it's missing. */
	dins->dins_Node.n_Priority += 1;
	InsertNodeFromTail ( RunningList, (Node *) dins );
	dins->dins_Node.n_Priority -= 1;

/* Link DSP code execution in same order as it appears in the host list.
** First make this instrument point to existing which is safe cuz this is
** not executing yet.
*/
	DSPPGetPrevNextIns( dins, dcls, &PrevDins, &NextDins );
	DSPPJumpTo ( dins, NextDins );
	
/* DSP CAN START EXECUTING INST HERE!!!!! */
	if( (dins->dins_ExecFrame == 1) && (PrevDins->dins_RateShift == 0 ) )
	{
		DSPPJumpFromSplitTo ( PrevDins, dins );
	}
	else
	{      
		DSPPJumpTo ( PrevDins, dins );          
	}
	
#ifdef DEBUG
	PRT(("DSPPLinkCodeToStartList list--------\n"));
	DumpList(RunningList);
#endif

	return 0;
}


/*****************************************************************/
static Err DSPPUnlinkCodeToStopList( DSPPInstrument *dins, DSPPCodeList *dcls )
{
	DSPPInstrument *PrevDins, *NextDins;
	
	DSPPGetPrevNextIns( dins, dcls, &PrevDins, &NextDins );

/* jump over code by patching from previous code to next code. */
	if( (dins->dins_ExecFrame == 1) && (PrevDins->dins_RateShift == 0 ) )
	{
		DSPPJumpFromSplitTo ( PrevDins, NextDins );
	}
	else
	{      
		DSPPJumpTo ( PrevDins, NextDins );          
	}

#ifdef DEBUG
	PRT(("DSPPUnlinkCodeToStopList list--- BEFORE RemNode -----\n"));
	DumpList(&dcls->dcls_InsList);
#define Dump3UInt32(msg,p) \
{ uint32 *xp; xp=(uint32 *)p; \
PRT(("%s : 0x%x = ", msg,xp)); \
PRT(("0x%x", *xp++)); \
PRT((", 0x%x", *xp++)); \
PRT((", 0x%x\n", *xp++)); \
}
	Dump3UInt32("Prev", *(((uint32 *) dins)+1));
	Dump3UInt32("Dins", dins);
	Dump3UInt32("Next", *((uint32 *) dins));
#endif

	ParanoidRemNode ( (Node *) dins );

#ifdef DEBUG
	PRT(("DSPPUnlinkCodeToStopList list--- AFTER RemNode -----\n"));
	DumpList(&dcls->dcls_InsList);
#endif
	return 0;
}

/******************************************************************
** Get actual DMA channel for a particular resource.
** Translate for output channels.
******************************************************************/
int32 DSPPGetResourceChannel( DSPPResource *drsc )
{
	int32 Result = AF_ERR_BADOFX;

	if( (drsc->drsc_Type & 0xFF) == DRSC_OUT_FIFO)
	{
		Result = drsc->drsc_Allocated + DDR0_CHANNEL;
	}
	else if( (drsc->drsc_Type & 0xFF) == DRSC_IN_FIFO)
	{
		Result = drsc->drsc_Allocated;
	}
DBUG(("DSPPGetResourceChannel: Channel = 0x%x\n", Result ));
	return Result;
}

/*****************************************************************/
int32 DSPPReleaseInstrument( DSPPInstrument *dins, TagArg *args)
{
	AudioAttachment *aatt;
	int32 i, ri, chan, Result = 0;
  	uint32 tagc, *tagp, tagv;
	ufrac16 TimeScale;
	int32 IfTimeScaleSet;
	Node *n;
	Item  AttItem;
	
	IfTimeScaleSet = FALSE;
	TimeScale = Convert32_F16(1);
		
/* Process tags */
	tagp = (uint32 *)args;
	if (tagp)
	{
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPReleaseInstrument: tagp = $%lx\n", tagp));
		while ((tagc = *tagp++) != 0)
		{
			tagv = *tagp++;
			
			switch (tagc)
			{				
			case AF_TAG_TIME_SCALE:
				TimeScale = (ufrac16) tagv;
				IfTimeScaleSet = TRUE;
				break;
				
			default:
				ERR (("Unrecognized tag in ReleaseInstrument - 0x%lx: 0x%lx\n", tagc, tagv));
				return AF_ERR_BADTAG;
				break;
			}
		}
	}

/* Scan all sample attachments and release them. */	
	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		ri = dins->dins_FIFOControls[i].fico_RsrcIndex;  /* %Q Got to be a better way. */
/*		chan = dins->dins_Resources[ri].drsc_Allocated; Did not work properly for OutputDMA 940713 */
		chan = DSPPGetResourceChannel( &dins->dins_Resources[ri] ); /* Correct 940713 */

		AttItem = AudioBase->af_DMAControls[chan].admac_AttachmentItem;
		aatt = (AudioAttachment *) CheckItem( AttItem,  AUDIONODE, AUDIO_ATTACHMENT_NODE);;
DBUG(("DSPPReleaseInstrument: release aatt = 0x%lx\n", aatt ));
		if (aatt) Result = DSPPReleaseSampleAttachment( aatt );
		if (Result < 0) break;
	}
	
/* Release all started envelopes. */
	n = FirstNode( &dins->dins_EnvelopeAttachments );
	while ( ISNODE(&dins->dins_EnvelopeAttachments,n))
	{
		aatt = (AudioAttachment *) n;
		if(IfTimeScaleSet) SetEnvAttTimeScale( aatt, TimeScale );
		n = NextNode(n);
TRACEB(TRACE_INT, TRACE_ENVELOPE|TRACE_NOTE, ("DSPPReleaseInstrument: env start 0x%lx\n", aatt));
		if( aatt->aatt_ActivityLevel == AF_STARTED)
		{
			DSPPReleaseEnvAttachment( aatt );
		}
	}
	
	return Result;
}

/********************************************************************
** Stop execution on DSPP of instrument.
** Handle special cases of Head, Tail, and Split.
********************************************************************/
static Err DSPPStopCodeExecution( DSPPInstrument *dins )
{
	int32 Result = 0;
	DSPPInstrument *LastDins = (void *) -1;

DBUGRATE(("DSPPStopCodeExecution( dins = 0x%x )\n", dins ));

	if (dins->dins_ActivityLevel <= AF_STOPPED) return 0; /* 941031 */

	switch( dins->dins_Specialness )
	{
		case AF_SPECIAL_NOT:   /* Most common. */
			Result = DSPPUnlinkCodeToStopList( dins, DSPPSelectRunningList( dins ) );
			break;
			
		case AF_SPECIAL_HEAD:
			DSPPData.dspp_ActiveInstruments.dcls_ListHead = NULL;
  			dsphWriteCodeMem( 0, DSPP_SLEEP_OPCODE);
			break;
			
		case AF_SPECIAL_TAIL:
			DSPPData.dspp_ActiveInstruments.dcls_ListTail = NULL;
			DSPPData.dspp_HalfRateInstruments[0].dcls_ListTail = NULL;
			DSPPData.dspp_HalfRateInstruments[1].dcls_ListTail = NULL;
  			dsphWriteCodeMem( 0, DSPP_SLEEP_OPCODE); /* Kludge, we're leaving anyway. */
			break;
			
		case AF_SPECIAL_SPLIT:
/* Assumes Half Rate lists are empty. */
#ifdef PARANOID
			if( ! ISEMPTYLIST( &DSPPData.dspp_HalfRateInstruments[0].dcls_InsList ) ||
				! ISEMPTYLIST( &DSPPData.dspp_HalfRateInstruments[1].dcls_InsList ) )
			{
				ERR(("DSPPStopCodeExecution: half lists not empty!\n"));
				return AF_ERR_SPECIAL;
			}
#endif
			DSPPData.dspp_ActiveInstruments.dcls_ListTail =
				DSPPData.dspp_HalfRateInstruments[0].dcls_ListTail;
			DSPPData.dspp_HalfRateInstruments[0].dcls_ListHead = NULL;
			DSPPData.dspp_HalfRateInstruments[1].dcls_ListHead = NULL;

/* Connect last in FullRate list to tail. */
			if( ISEMPTYLIST(&DSPPData.dspp_ActiveInstruments.dcls_InsList) )
			{
				LastDins = DSPPData.dspp_ActiveInstruments.dcls_ListHead;
			}
			else
			{
				LastDins = (DSPPInstrument *) LASTNODE( &DSPPData.dspp_ActiveInstruments.dcls_InsList );
			}
			DSPPJumpTo( LastDins, DSPPData.dspp_ActiveInstruments.dcls_ListTail );
			break;
			
		default:
			Result = AF_ERR_SPECIAL;
			break;
	}
	
	dins->dins_ActivityLevel = AF_STOPPED;
	
	return Result;
}

/*****************************************************************/
int32 DSPPStopInstrument( DSPPInstrument *dins, TagArg *args )
{
	AudioAttachment *aatt;
	int32 i, ri, chan, Result = 0;
	Node *n;
	Item AttItem;

DBUGNOTE(("DSPPStopInstrument: >>>>>>>>>>>>>>>>>>>>>>\n"));

/* Stop the executing DSP code. */
	Result = DSPPStopCodeExecution( dins );
	if( Result < 0 ) goto error;
	
/* 921216 %Q WARNING! - the code could still be running if caught in middle! */
/* Scan all sample attachments and stop them. */	
	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		ri = dins->dins_FIFOControls[i].fico_RsrcIndex;  /* Got to be a better way. */

/*		chan = dins->dins_Resources[ri].drsc_Allocated; Did not work properly for OutputDMA 940713 */
		chan = DSPPGetResourceChannel( &dins->dins_Resources[ri] ); /* Correct 940713 */

		AttItem = AudioBase->af_DMAControls[chan].admac_AttachmentItem;
DBUG(("DSPPStopInstrument, i = %d, ri = %d, ", i, ri ));
		aatt = (AudioAttachment *) CheckItem( AttItem,  AUDIONODE, AUDIO_ATTACHMENT_NODE);

DBUG(("   chan=0x%lx, AttItem = 0x%x, aatt = 0x%x\n", chan, AttItem, aatt));
		if (aatt)
		{
#if 0
			if( chan != aatt->aatt_Channel)
			{
				ERR(("PARANOID trap in DSPPStopInstrument, chan = %d, aatt->aatt_Channel = %d\n",
					chan, aatt->aatt_Channel));
				return -1;
			}
			PRT(("DSPPStopInstrument: valid aatt on %d, chan=0x%lx\n", i, chan));
#endif
			Result = DSPPStopSampleAttachment( aatt );
			if (Result < 0) break;
		}
	}
	
/* Stop all started envelopes. */
	n = FirstNode( &dins->dins_EnvelopeAttachments );
	while ( ISNODE(&dins->dins_EnvelopeAttachments,n))
	{
		aatt = (AudioAttachment *) n;
		n = NextNode(n);
TRACEB(TRACE_INT, TRACE_ENVELOPE|TRACE_NOTE, ("DSPPStopInstrument: env stop 0x%lx\n", aatt));
		DSPPStopEnvAttachment( aatt );
	}

DBUGNOTE(("DSPPStopInstrument: <<<<<<<<<<<<<<<<<<<<<<<<<\n"));
error:
	return Result;
}

/*******************************************************************/
int32	DSPPStopSampleAttachment( AudioAttachment *aatt )
{
	int32 DMAChan;
	AudioDMAControl *admac;
	
#ifdef PARANOID
	AudioAttachment *xaatt;
	xaatt = (AudioAttachment *) CheckItem(aatt->aatt_Item.n_Item,
				AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (xaatt == NULL)
	{
		ERR(("DSPPStopSampleAttachment: Attachment dead: 0x%lx\n", aatt->aatt_Item.n_Item));
		return AF_ERR_BADITEM;
	}
#endif
	
	if(aatt->aatt_ActivityLevel <= AF_STOPPED) return 0;
	
	DMAChan = aatt->aatt_Channel;
	DisableAttachmentSignal( aatt );
DBUG(("Stop ATT: 0x%lx, DMAChan = 0x%lx\n", aatt, DMAChan ));
	dsphDisableDMA(DMAChan);
	admac = &AudioBase->af_DMAControls[DMAChan];
	admac->admac_AttachmentItem = 0;
	aatt->aatt_ActivityLevel = AF_STOPPED;
	return(0);
}

/*****************************************************************/
int32 DSPPAttachSample( DSPPInstrument *dins, AudioSample *asmp, AudioAttachment *aatt)
{
	int32 Result = 0;
	FIFOControl *fico;
	DSPPResource *drsc = NULL;
	char *FIFOName;
	
TRACEE(TRACE_INT,TRACE_SAMPLE,("DSPPAttachSample ( dins=0x%lx, asmp=0x%lx)\n",
	dins, asmp));
	FIFOName = aatt->aatt_HookName;
	
	if (FIFOName)
	{
		TRACEE(TRACE_INT,TRACE_SAMPLE,(" %s)\n", FIFOName));
	}

	fico = DSPPFindFIFO( dins, FIFOName );

/* Did we find the FIFO resource. */
	if (fico)
	{
		drsc = &dins->dins_Resources[fico->fico_RsrcIndex];
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPAttachSample: fico->fico_RsrcIndex =  0x%lx\n",
		fico->fico_RsrcIndex));
		if( (drsc->drsc_Type & 0xFF) == DRSC_OUT_FIFO)
		{
			if(!(asmp->asmp_SuperFlags & AF_SAMPF_SUPER_ALLOC))
			{
				ERR(("DSPPAttachSample: Must use DelaySample for Output DMA\n")); 
				Result = AF_ERR_SECURITY;
				goto error;
			}
		}
		aatt->aatt_Channel = DSPPGetResourceChannel( drsc ); /* 940713 */
		
#ifdef PARANOID
		if((aatt->aatt_Channel < 0) || (aatt->aatt_Channel > NUM_AUDIO_DMA_CHANNELS))
		{
			ERR(("Paranoid: DSPPAttachSample: aatt_Channel = %d !!\n", aatt->aatt_Channel));
		}
#endif

TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPAttachSample: aatt_Channel =  0x%lx\n", aatt->aatt_Channel));
		AddTail(&fico->fico_Attachments, (Node *)aatt);
	}
	else
	{
		ERR(("DSPPAttachSample: Could not find Input FIFO = %s\n", FIFOName));
		Result = AF_ERR_NOFIFO;
	}
error:
	return Result;
}

/*******************************************************************************/
/******* DSP SWIs **************************************************************/
/*******************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/allocamplitude
 |||	AllocAmplitude - Allocates amplitude from available system amplitude.
 |||
 |||	  Synopsis
 |||
 |||	    int32 AllocAmplitude( int32 Amplitude )
 |||
 |||	  Description
 |||
 |||	    AllocAmplitude  is a voluntary procedure that checks available system
 |||	    amplitude and allocates the requested amplitude (or less, if the amount
 |||	    requested is not available) to the calling task.  It then marks that
 |||	    amplitude unavailable to other tasks making this call.  When the task
 |||	    finishes using the amplitude, it should call FreeAmplitude() to make the
 |||	    amplitude available to other tasks.  Note that this procedure is voluntary
 |||	    and doesn't restrict the amplitude actually used by a task; the only
 |||	    way it works is for all audio tasks to use this procedure.
 |||
 |||	    This call is necessary, because the output of the DSP is limited by the
 |||	    signed 16-bit precision of the digital-to-analog converter (DAC).  If you
 |||	    add together signals whose amplitude exceeds the limit of 0x7FFF, the
 |||	    result will "wrap around,"  causing a horrendous noise.  Rather
 |||	    than dictate a restriction on use of the DSP, we assume that
 |||	    programmers/composers will restrict themselves using whatever scheme they
 |||	    choose.  It is necessary, however, to tell other tasks how much of the DSP
 |||	    precision you plan to use.  You can allocate some or all of the amplitude
 |||	    for your task and then give it back when done.
 |||
 |||	  Arguments
 |||
 |||	    Amplitude                    The desired amplitude.  This must be value
 |||	                                 from 0 to 0x7FFF.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the amount of amplitude that was allocated.  This
 |||	    will be less than or equal to the desired amount.  If unsuccessful, it
 |||	    returns a negative value (an error code).
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
 |||	    FreeAmplitude()
 |||
 **/
int32 swiAllocAmplitude ( int32 Amplitude )
{
TRACEE(TRACE_TOP, TRACE_DSP, ("swiAllocAmplitude( 0x%lx )\n", Amplitude ));

/* Range check. */
	if ((Amplitude < 0) || (Amplitude > MAXDSPAMPLITUDE))   /* 931221 */
	{
		ERR(("Freeamplitude: illegal amplitude = 0x%08x\n", Amplitude));
		return AF_ERR_OUTOFRANGE;
	}
	
	if (Amplitude > DSPPData.dspp_AvailableAmplitude)
	{
		Amplitude = DSPPData.dspp_AvailableAmplitude;
	}
	DSPPData.dspp_AvailableAmplitude -= Amplitude;
TRACER(TRACE_TOP, TRACE_DSP, ("swiAllocAmplitude returns 0x%lx )\n", Amplitude ));
	return Amplitude;
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/freeamplitude
 |||	FreeAmplitude - Inverse of AllocAmplitude().
 |||
 |||	  Synopsis
 |||
 |||	    Err FreeAmplitude( int32 Amplitude )
 |||
 |||	  Description
 |||
 |||	    This procedure gives back amplitude allocated by AllocAmplitude().
 |||
 |||	  Arguments
 |||
 |||	    Amplitude                    Amount of amplitude given by
 |||	                                 AllocAmplitude().
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
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
 |||	    AllocAmplitude()
 |||
 **/
int32 swiFreeAmplitude ( int32 Amplitude )
{
TRACEE(TRACE_TOP, TRACE_DSP, ("swiFreeAmplitude( 0x%lx )\n", Amplitude ));
	
/* Range check. */
	if ((Amplitude < 0) || (Amplitude > MAXDSPAMPLITUDE))   /* 931221 */
	{
		ERR(("Freeamplitude: illegal amplitude = 0x%08x\n", Amplitude));
		return AF_ERR_OUTOFRANGE;
	}
	
/* This does not return an error code because it may break existing apps. */
	if ((Amplitude + DSPPData.dspp_AvailableAmplitude) > MAXDSPAMPLITUDE )
	{
		Amplitude = MAXDSPAMPLITUDE - DSPPData.dspp_AvailableAmplitude;
	}
	DSPPData.dspp_AvailableAmplitude += Amplitude;
TRACEE(TRACE_TOP, TRACE_DSP, ("swiFreeAmplitude returns 0x%lx\n", Amplitude ));
	return Amplitude;
}

/* Hack test routine for Phil and Don to bang hardware. */

/* extern Err RampAudioAmplitude( int32 IfRampDown ); For DuckAndCOver test */

int32 swiTestHack ( TagArg *args )
{
  	uint32 tagc, *tagp;
  	int32 *addr;
  	int32 Result=0, n;
	
	tagp = (uint32 *)args;
	if (tagp)
	{
DBUG(("tagp = $%lx\n", tagp));
		while ((tagc = *tagp++) != 0)
		{
			switch (tagc)
			{
			case HACK_TAG_READ_EO:
/* For compatibility, this will need to return the TickCount for n=0,
** and return the FrameCount for n=2.  For other addresses,
** map into new fake EO space.
** For non compatibility, return AF_ERR_UNIMPLEMENTED.
*/
				n = *tagp++;
				if ((n < 0) || (n > LAST_EO_MEM))
				{
					ERR(("swiTestHack: Bad EO offset = 0x%lx\n", n));
					Result = AF_ERR_BADTAGVAL;
					goto done;
				}
				Result = ReadHardware(DSPPEO16+n);
				break;
				
			case HACK_TAG_READ_DMA:
				n = *tagp++;
				if ((n < 0) || (n > 0x20))
				{
					ERR(("swiTestHack: Bad DMA channel.\n"));
					Result = AF_ERR_BADTAGVAL;
					goto done;
				}
				addr = (int32 *) RAMtoDSPP0;
				addr += n; /* %Q This is wrong way to address DMA control registers.  Unused anyway. Check DSPReadDMA() */
				Result = ReadHardware(addr);
				break;
#if 0
			case 18:
				n = *tagp++;
				RampAudioAmplitude( n );
				break;
#endif

#ifdef COMPILE_DSPP_TRACE
			case 17:
				DSPPTraceExecution();
				break;
#endif

			default:
				ERR(("Warning - unrecognized argument to TestHack - 0x%lx: 0x%lx\n",
					tagc, *tagp++));
				Result = AF_ERR_BADTAG;
				goto done;
			}
		}
	}
done:
	return Result;
}

static int32 dci_entry;  /* %Q Warning global shared temp variable ! */
void PatchCode16(int32 Value, int32 indx, uint16 *ar )
{
	dsphWriteCodeMem( dci_entry + indx, (uint32) Value);
DBUG(("PatchCode16: Code[$%lx] = $%lx, indx = $%lx\n",
	(dci_entry+indx), Value, indx));
}

/*****************************************************************/
int32 DSPPConnectInstruments ( DSPPInstrument *dins_src, char *name_src,
	DSPPInstrument *dins_dst, char *name_dst)
{
	DSPPTemplate *dtmp;
	DSPPRelocation *drlc;
	DSPPResource *drsc, *drsc_src, *drsc_dst;
	uint32  val, ri;
	int32  i, Result=0;
	
/* Find source resource then modify existing code at destination to use it. */

TRACEE(TRACE_INT,TRACE_OFX,("DSPPConnectInstruments ( 0x%lx, %s, ",
	dins_src, name_src ));
TRACEE(TRACE_INT,TRACE_OFX,("0x%lx, %s\n", dins_dst, name_dst));
	
	drsc_src = DSPPFindResource (dins_src, DRSC_I_MEM, name_src );
	if (drsc_src == NULL)
	{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPConnectInstruments: bad source name.\n"));
		return AF_ERR_BADNAME;
	}
	drsc_dst = DSPPFindResource (dins_dst, DRSC_I_MEM, name_dst );
	if (drsc_dst == NULL)
	{
/* Try a knob. */
		drsc_dst = DSPPFindResource (dins_dst, DRSC_EI_MEM, name_dst );
		if (drsc_dst == NULL)
		{
			
TRACEB(TRACE_INT,TRACE_OFX,("DSPPConnectInstruments: bad dest name.\n"));
			return AF_ERR_BADNAME;
		}
	}
	dtmp = dins_dst->dins_Template;

/* Scan for all relocations in destination that use that resource. */
	for (i=0; i<dtmp->dtmp_NumRelocations; i++)
	{
		drlc = &dtmp->dtmp_Relocations[i];
		
/* Get this relocations resource index and do bounds check. */
		ri = drlc->drlc_RsrcIndex;
		if (ri > dtmp->dtmp_NumResources) return AF_ERR_BADRLOCINDX;
		drsc = &dins_dst->dins_Resources[ri];
		
/* Check to see if it matches drsc_dst */
		if (drsc == drsc_dst)
		{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPConnectInstruments: resource match!\n"));
			val = GetRsrcAttribute(drsc_src, drlc->drlc_Attribute);
TRACEB(TRACE_INT,TRACE_OFX,("-----------------\nRelocate Type = %d, val = $%lx\n",
		drsc->drsc_Type, val));

			dci_entry = dins_dst->dins_EntryPoint; /* %Q sneakily pass entry point */
			Result = DSPPRelocate ( drlc, val , dtmp->dtmp_Codes, PatchCode16 );
			if (Result) goto error;
		}
	}
error:
	return Result;
}

/*****************************************************************/
int32 DSPPDisconnectInstruments ( DSPPInstrument *dins_src, char *name_src,
	DSPPInstrument *dins_dst, char *name_dst)
{
/* %Q Exactly like ConnectInstruments but sets to 0xC000 for #$0000 */
	DSPPTemplate *dtmp;
	DSPPRelocation *drlc;
	DSPPResource *drsc, *drsc_src, *drsc_dst;
	uint32  val, ri;
	int32  i, Result=0;
	
/* Find source resource then relocate existing code at destination to use it. */

TRACEE(TRACE_INT,TRACE_OFX,("DSPPDisconnectInstruments\n"));
	drsc_src = DSPPFindResource (dins_src, DRSC_I_MEM, name_src );
	if (drsc_src == NULL)
	{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPDisconnectInstruments: bad source name.\n"));
		return AF_ERR_BADNAME;
	}
	drsc_dst = DSPPFindResource (dins_dst, DRSC_I_MEM, name_dst );
	if (drsc_dst == NULL)
	{
/* Is it a knob? */
		drsc_dst = DSPPFindResource (dins_dst, DRSC_EI_MEM, name_dst );
		if (drsc_dst == NULL)
		{
			
TRACEB(TRACE_INT,TRACE_OFX,("DSPPDisconnectInstruments: bad dest name.\n"));
			return AF_ERR_BADNAME;
		}
	}
	dtmp = dins_dst->dins_Template;

/* Scan for all relocations in destination that use that resource. */
	for (i=0; i<dtmp->dtmp_NumRelocations; i++)
	{
		drlc = &dtmp->dtmp_Relocations[i];
		
/* Get this relocations resource index and do bounds check. */
		ri = drlc->drlc_RsrcIndex;
		if (ri > dtmp->dtmp_NumResources) return AF_ERR_BADRLOCINDX;
		drsc = &dins_dst->dins_Resources[ri];
		
/* Check to see if resource of dest reloc matches selected resource */
		if (drsc == drsc_dst)
		{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPConnectInstruments: resource match!\n"));

/* Restore original only if babs or later. 941130 */
			if( DiscOsVersion(0) > MakeDiscOsVersion(24,0) ) 
			{
				/* Restore to original knob or dummy input. */
				val = GetRsrcAttribute(drsc, drlc->drlc_Attribute);
			}
			else
			{
				val = 0xC000;  /* Immediate #0 operand. !!!!!!!!!!!!!! */
			}

DBUG(("DisconnectInstruments: Relocate Type = %d, val = $%lx\n", drsc->drsc_Type, val));

			dci_entry = dins_dst->dins_EntryPoint; /* %Q sneakily pass entry point */
			Result = DSPPRelocate ( drlc, val , dtmp->dtmp_Codes, PatchCode16 );
			if (Result) goto error;
		}
	}
error:
	return Result;
}
