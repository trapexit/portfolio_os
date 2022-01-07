/* $Id: audio_structs.h,v 1.30 1994/12/16 08:41:27 phil Exp $ */
#ifndef _audio_structs_h
#define _audio_structs_h

/***********************************************************************
** Internal includes file for audio folio.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
************************************************************************
** 940406 PLB Support TuneInsTemplate()
** 940606 WJB Changed ains_StartTime from uint32 to AudioTime.
** 940608 PLB Added dspp_TicksPerFrame
** 940614 PLB added asmp_BadDogData
** 940815 PLB Moved DSPPStatic to dspp.h
***********************************************************************/

#include "types.h"
#include "stdlib.h"
#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "interrupts.h"
#include "kernel.h"
#include "mem.h"
#include "list.h"
#include "task.h"
#include "folio.h"
#include "kernelnodes.h"
#include "stdarg.h"
#include "string.h"
#include "operror.h"
#include "io.h"
#include "super.h"
#include "semaphore.h"
#include "driver.h"

#include "audio.h"
#include "inthard.h"

/**********************************************************************/
/***************** Constants ******************************************/
/**********************************************************************/

enum af_special_types
{
	AF_SPECIAL_NOT = 0,
	AF_SPECIAL_HEAD,
	AF_SPECIAL_TAIL,
	AF_SPECIAL_SPLIT
};
#define AF_SPECIAL_MAX AF_SPECIAL_SPLIT

/**********************************************************************/
/***************** Internal Structures ********************************/
/**********************************************************************/

typedef struct AudioReferenceNode
{
	Node    arnd_Node;
	Item    arnd_RefItem;
} AudioReferenceNode;

typedef struct AudioEvent
{
	ItemNode   aevt_Node;
	AudioTime  aevt_TriggerAt;
	int32    (*aevt_Perform)();
	List      *aevt_InList;  /* List we are linked into or NULL */
} AudioEvent;

typedef struct AudioCue
{
	AudioEvent acue_Event;
	uint32     acue_Signal;
	Task      *acue_Task;
} AudioCue;
	
typedef struct AudioTuning
{
	ItemNode   atun_Item;
	ufrac16   *atun_Frequencies;     /* 16.16 Hertz */
	int32      atun_NumNotes;        /* Number of notes in Fractions array. */
	int32      atun_BaseNote;        /* Note corresponding to zeroth fraction. */
	int32      atun_NotesPerOctave;
} AudioTuning;

typedef struct AudioEnvelope
{
	ItemNode   aenv_Item;
	DataTimePair *aenv_Points;
	int32      aenv_NumPoints;
	int32      aenv_SustainBegin;	/* Set to -1 if no sustain loop */
	int32      aenv_SustainEnd; 	/* Set to -1 if no sustain loop */
	int32      aenv_SustainTime;	/* Time to get from End to Begin */
	int32      aenv_ReleaseBegin;	/* Set to -1 if no release loop */
	int32      aenv_ReleaseEnd; 	/* Set to -1 if no release loop */
	int32      aenv_ReleaseTime;	/* Time to get from End to Begin */
	int32      aenv_MicrosPerUnit;	/* microseconds per time unit */
	ufrac16    aenv_FramesPerUnit;  /* For fast time calculations.  */
	int32      aenv_Flags;
	int32      aenv_ReleaseJump;    /* Point to jump to upon release, or <0. */
	int32      aenv_Reserved;
	List       aenv_AttachmentRefs;   /* List of reference Nodes. */
} AudioEnvelope;

/* Chunk file format from Envelope FORM. */
typedef struct ENVH_Chunk
{
	int32      envh_NumPoints;
	int32      envh_SustainBegin;	/* Set to -1 if no sustain loop */
	int32      envh_SustainEnd; 	/* Set to -1 if no sustain loop */
	int32      envh_SustainTime;	/* Time to get from End to Begin */
	int32      envh_ReleaseBegin;	/* Set to -1 if no release loop */
	int32      envh_ReleaseEnd; 	/* Set to -1 if no release loop */
	int32      envh_ReleaseTime;	/* Time to get from End to Begin */
	int32      envh_MicrosPerDelta;	/* microseconds per time unit */
	int32      envh_Flags;
	int32      envh_ReleaseJump;
	int32      envh_Reserved;
} ENVH_Chunk;

#define AF_ATT_TYPE_SAMPLE (1)
#define AF_ATT_TYPE_ENVELOPE (2)

#define AF_ATTF_LAST_SEGMENT  (1)    /* SUPER - If set, attachment is on last segment. */

/* Attachment Item **************************************/
typedef struct AudioAttachment
{
	ItemNode   aatt_Item;
	uint8      aatt_Type;
	uint8      aatt_SegmentCount;  /* Counts down, when it reaches zero => at end. */
	int8       aatt_ActivityLevel; /* Abandoned, Stopped, Started, Released.  */
	uint8      aatt_Reserved3;
	uint32     aatt_Flags;
	Item       aatt_HostItem;      /* Audio Instrument or Instrument Template */
	Item       aatt_SlaveItem;     /* Sample or Envelope Item */
	void      *aatt_Structure;     /* Sample or Envelope address */
	char      *aatt_HookName;      /* What we are attached to. */
	int32      aatt_StartAt;       /* Index to Start At when started. */
	int32      aatt_Channel;       /* DMA Channel */
	Item       aatt_CueItem;
	int32      aatt_CueAt;
	Item       aatt_NextAttachment;
	void *     aatt_Extension;
} AudioAttachment;

/**********************************************************
** Envelope Attachment Extension
** Contains data needed to "play" an envelope on an instrument.
**********************************************************/
typedef struct AudioEnvExtension
{
	AudioEvent aeva_Event;
	AudioAttachment *aeva_Parent;
	int32      aeva_TargetEI;      /* EI location of .target Value */
	int32      aeva_IncrEI;        /* EI location of .incr Phase Increment */
	int32      aeva_RequestEI;     /* EI location of .request Value */
	int32      aeva_CurrentEI;     /* EI location of .current Phase Increment */
	int32      aeva_CurIndex;      /* Index that matches TargetValue */
	int32      aeva_FramesLeft;    /* Frames remaining in long segment. */
	int32      aeva_PrevTarget;    /* Target of previous segment. */
	int32      aeva_DeltaTime;     /* Delta Time to use for this segment. */
	ufrac16    aeva_FramesPerUnit; /* Scaled specifically for attachment.  */
} AudioEnvExtension;

/*
** Used by the application to control parameters on an Audio Instrument.
** When a knob is attached, the device fills in function pointers for
** fast execution.
*/
typedef struct AudioKnob
{
	ItemNode	aknob_Item;
	void       *aknob_DeviceInstrument; /* Instrument that knob belongs to */
	void       *aknob_DeviceKnob;
	int32       aknob_Reserved;
	int32       aknob_CurrentValue;     /* Last value sent to device after conversion. */
} AudioKnob;

typedef struct AudioProbe
{
	ItemNode	aprob_Item;
	void       *aprob_DeviceInstrument; /* Instrument that probe belongs to */
	int32       aprob_IfEO;
	int32       aprob_Reserved;
	int32       aprob_DSPI_Address;     /* DSPI address to read. */
} AudioProbe;

typedef struct AudioInsTemplate
{
	ItemNode aitp_Item;
	int32    aitp_OpenCount;		/* Incremented each time a user opens, decr for close. */
	void    *aitp_DeviceTemplate;   /* Device specific information. */
	List     aitp_InstrumentList;   /* List of Instruments allocated from this template. */
	List     aitp_Attachments;      /* List of attached items. */
	Item     aitp_Tuning;           /* Tuning for all child instruments. 940406 */
} AudioInsTemplate;

typedef struct AudioInstrument
{
	ItemNode	ains_Item;
	AudioInsTemplate	*ains_Template;
	void 		*ains_DeviceInstrument;
	uint32		ains_Flags;
	int8        ains_Status;   /* AF_STARTED, AF_RELEASED, etc. */
	uint8       ains_Reserved1;
	uint8       ains_Reserved2;
	uint8       ains_Reserved3;
	List        ains_KnobList;  /* List of all grabbed knobs. */
	List        ains_ProbeList;  /* List of all probes. */
	Item        ains_Tuning;
	frac16      ains_Bend;
	int32       ains_OriginalRate;   /* For bending from. */
	AudioTime	ains_StartTime;
} AudioInstrument;

/* Structure of actual hardware DMA register block, 931220 made volatile */
typedef volatile struct DMARegisters
{
	int32    *dmar_Address;
	uint32    dmar_Count;
	int32    *dmar_NextAddress;
	uint32    dmar_NextCount;
} DMARegisters;

/* Used for passing around the results of sample DMA calculations. */
typedef struct SampleDMA
{
	int32    *sdma_Address;
	uint32    sdma_Count;
	int32    *sdma_NextAddress;
	uint32    sdma_NextCount;
} SampleDMA;


typedef struct AudioDMAControl
{
	DMARegisters *admac_ChannelAddr;  /* Address of DMA Channel */
	int32       admac_NextCountDown;  /* set DMA Next registers at zero, used to count loops */
	int32      *admac_NextAddress;    /* Address of sample memory. */
	uint32      admac_NextCount;      /* number of bytes */
	int32       admac_SignalCountDown;/* signal foreground at zero */
	int32       admac_Signal;         /* used to signal foreground from interrupt, set once */
	Item        admac_AttachmentItem;   /* Used when releasing or stopping instrument */
} AudioDMAControl;

#define NUM_AUDIO_DMA_CHANNELS (20)

/* Structure for each task using audiofolio. */

typedef struct AudioFolioTaskData
{
	int32 aftd_InputEnables;   /* Incremented each time EnableAudioInput() is called. */
} AudioFolioTaskData;

typedef struct AudioFolio
{
	Folio       af_Folio;
	uint32		af_TraceMask;
	int32		af_IndentLevel;
	List        af_TemplateList;
	void		*af_DSPP;             /* data specific to the DSPP */
	
	AudioTime   af_Time;
	AudioTime   af_NextTime;         /* For next waiting task. */
	int32       af_TimerSignal;      /* to signal foreground from int */
	Task       *af_AudioTask;        /* foreground timer and DMA task */
	int32       af_TimerPending;     /* Are any timer events pending. */
	List		af_TimerList;        /* pending Timer items */
	Item		af_TimerListSemaphore;   /* for list access */
	Item        af_TimerDurKnob;     /* CountDown value */
	Item		af_TimerDurSemaphore;
	Item        af_TimerFIRQ;
	uint32      af_Duration;         /* of clock in DSP frames */
/* Used if sample rate changes to update timer rate. */
	frac16		af_DesiredTimerRate; /* Set if SetAudioRate called. */
	
	ufrac16     af_MasterTuning;     /* Frequency of A, default = 440.000 */
	AudioTuning *af_DefaultTuning;   /* Tuning to use if none other specified. */
	
	AudioDMAControl af_DMAControls[NUM_AUDIO_DMA_CHANNELS];
	int32       af_DMASignalMask;    /* All DMA Signals ORed together */
	List        af_SampleList;       /* List of all existing samples. */
	char       *af_StartupDirectory; /* Set at startup. */
	uint32      af_GlobalIndex;      /* Shared Global Index, used for memory alloc debugging */
	int32       af_TimerTicksLost;   /* Debug lost timer ticks. */
	int32       af_LastTickCount;    /* Last tick count recorded by timer interrupt. */
	Item		af_ReadySemaphore;   /* locked until folio ready for use */
	
	Item        af_SplitExecIns;     /* Instrument used for half rate calculations */
	int32       af_SplitExecCount;   /* Open Count */
	int32       af_InputEnableCount; /* Open count for Audio Input enable. */
} AudioFolio;

#define AF_SAMPF_FOLIO_OWNS  (0x01)   /* Folio allocated data. */
#define AF_SAMPF_SUPER_ALLOC (0x02)   /* Allocated in supervisor mode for output DMA */

typedef struct AudioSample
{
	ItemNode asmp_Item;
	void   *asmp_Data;			/* Points to first sample */
	int32	asmp_NumFrames;		/* in frames */
	int32	asmp_SustainBegin;	/* Set to -1 if no sustain loop */
	int32	asmp_SustainEnd;
	int32	asmp_ReleaseBegin;	/* Set to -1 if no release loop */
	int32	asmp_ReleaseEnd;
	Item    asmp_OwnerItem;		/* Item which owns this if embedded. */
	int32	asmp_OpenCount;		/* Incremented each time a user opens, decr for close. */
	int32	asmp_Type;			/* Single or MultiSample */
	int32   asmp_NumBytes;		/* Size in Bytes to be played. */
	ufrac16	asmp_BaseFreq;		/* freq (Hz) if played at 44.1 KHz */
	uint8	asmp_SuperFlags;    /* Do not allow changes by user. */
	uint8	asmp_Bits;			/* ORIGINAL bits per sample BEFORE any compression. */
	uint8	asmp_Width;			/* ORIGINAL bytes per sample BEFORE any compression. */
	uint8	asmp_Channels;		/* channels per frame, 1 = mono, 2=stereo */
	uint8	asmp_BaseNote;		/* MIDI Note when played at original sample rate. */
	int8	asmp_Detune;
	uint8	asmp_LowNote;
	uint8	asmp_HighNote;
	uint8	asmp_LowVelocity;
	uint8	asmp_HighVelocity;
	uint8	asmp_CompressionRatio;   /* 2 = 2:1, 4 = 4:1 */
	uint8	asmp_Reserved1;
	int32	asmp_Reserved2;
	int32	asmp_DataOffset;	/* Offset within AIFF File of first data. */
	int32	asmp_DataSize;		/* Size in bytes of the files data chunk. */
	uint32  asmp_CompressionType;  /* eg. SDX2 , 930210 */
	List    asmp_AttachmentRefs;   /* List of reference Nodes. */
	ufrac16 asmp_SampleRate;    /* Sample Rate recorded at */
	void  (*asmp_CustomFreeMem)();  /* User's custom memory freeer */
	int32   asmp_NumBytesAllocated;
	void   *asmp_BadDogData;    /* cover MakeSample() caller bugs, (Sample *) 940614 */
} AudioSample;

/* Reduce number of allowable markers to make room for spc_SustainLoopType, etc. 940921  */
#define AIFF_MAX_MARKERS 28

/* Parser contexts for IFF files. */
typedef struct SampleParserContext
{
/* Info passed to parser. */
	int32 spc_IfLeaveInPlace;
	void *(*spc_CustomAllocMem)();
	int32 spc_IfReadData;
/* Info set by parser. */
	AudioSample *spc_Sample;
	int32 spc_IfCompressed;
	int32 spc_NumMarkers;
	int32 spc_MarkerIDs[AIFF_MAX_MARKERS];
	int32 spc_MarkerPositions[AIFF_MAX_MARKERS];
/* Save Loop info in case MARK chunk is after INST 940921 */
	int32 spc_SustainLoopType;
	int32 spc_SustainBeginMark;
	int32 spc_SustainEndMark;
	int32 spc_ReleaseLoopType;
	int32 spc_ReleaseBeginMark;
	int32 spc_ReleaseEndMark;
} SampleParserContext;

/* Template */
typedef struct TemplateParserContext
{
/* Info passed to parser. */
	int32   tmplpc_IfLeaveInPlace;
	void *(*tmplpc_CustomAllocMem)();
	void  (*tmplpc_CustomFreeMem)();
	int32   tmplpc_InsMode;  /* Holds a tag. */
/* Info set by parser. */
	Item    tmplpc_SlaveItem;
	char    tmplpc_HookName[AF_MAX_NAME_SIZE];
	Item    tmplpc_TemplateItem;
	char   *tmplpc_InsName;
	uint32  tmplpc_Flags;
} TemplateParserContext;

#define TMPLPC_FLAG_CRDC_FOUND   0x0001
#define TMPLPC_FLAG_DFID_FOUND   0x0002

#endif
