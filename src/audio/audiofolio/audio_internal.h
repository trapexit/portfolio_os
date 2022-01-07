/* $Id: audio_internal.h,v 1.72 1994/12/14 23:39:16 phil Exp $ */
#pragma include_only_once
#ifndef _audio_internal_h
#define _audio_internal_h

/*
** Audio Folio Internal Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

/*
** 930816 PLB Removed redefinition of SuperInternalFreeSignal
** 940304 WJB Added stdio.h.
** 940407 PLB Added DEBUG_AUDIO to select between dev and debug.
** 940506 PLB Changed SAMPLERATE to DEFAULT_SAMPLERATE
** 940511 WJB Removed local prototype for isUser().  Including clib.h instead.
** 940802 WJB Added support for demand loading.
** 940811 PLB Added ezmem_tools.h and table_alloc.h
** 940912 PLB Added Write16(), Read16() macros.
** 940915 PLB Call IsItemOpened(), not ItemOpened()
** 940927 WJB Removed redundant Create/DeleteProbe() prototypes.
** 941011 WJB Added prototype for GetMasterTuning() (no longer in audio.h).
** 941012 WJB Syncronized swiAbandonInstrument() prototype in audio.h
** 941024 PLB Removed "UNSUPPORTED" macro, only used by GetKnob()
*/

#include "types.h"
#include "stdlib.h"
#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "tags.h"
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
#include "stdio.h"
#include "clib.h"       /* isUser() */
#include "sherryvers.h"

#include "iff_fs_tools.h"
#include "ezmem_tools.h"
#include "table_alloc.h"

/* Audio specific */

#include "audio.h"
#include "inthard.h"
#include "audio_structs.h"
#include "dspp.h"

#ifdef USERONLY
#define	PRT(x)	{ printf x; }
#else
#define PRT(x)  { if (isUser() == 0) { Superkprintf x;  } \
	 else { printf x; } }
#endif

/**********************************************************************/
/**************************** Debug Support  **************************/
/**********************************************************************/
#ifdef PRODUCTION
	#define ERR(x)  /* PRT(x) */
	#define ERRDBUG(x)  /* PRT(x) */
	#define TRACEDBUG(andmask,ormask,type,msg) /* TRACEDBUG */
#else
	#define ERR(x)  PRT(x)
	#ifdef DEBUG_AUDIO
		#define ERRDBUG(x)  PRT(x)
		#define TRACEDBUG(andmask,ormask,type,msg) \
			if( TraceIndent( andmask, ormask, type) ) PRT(msg);
	#else
		#define ERRDBUG(x)  /* PRT(x) */
		#define TRACEDBUG(andmask,ormask,type,msg) /* TRACEDBUG */
	#endif
#endif

#define TRACEE(andmask,ormask,msg) TRACEDBUG(andmask,ormask,0,msg)
#define TRACEB(andmask,ormask,msg) TRACEDBUG(andmask,ormask,1,msg)
#define TRACER(andmask,ormask,msg) TRACEDBUG(andmask,ormask,2,msg)

#define TRACKMEM(x) /* { PRT(x); afi_ReportMemoryUsage(); } */

/* Macro to simplify error checking. */
#define CHECKVAL(val,msg) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("%s: $%x\n", msg, val)); \
		goto error; \
	}

#define CHECKRSLT(msg) \
	if (Result < 0) \
	{ \
		ERR(msg); \
		goto error; \
	}

#define ItemStructureParanoid(st,type,msg) \
{ \
	if (CheckItem(((ItemNode *) st)->n_Item, AUDIONODE, type) == NULL) \
	{ \
		ERR(("Paranoia Trap %s = 0x%x\n", msg, st)); \
		return AF_ERR_BADITEM; \
	} \
}

#ifdef PARANOID
#define ParanoidRemNode(n) \
{ \
	Node *paranode; \
	paranode = n; \
	if(( paranode->n_Next->n_Prev != n) || ( paranode->n_Prev->n_Next != n)) \
	{ \
		ERR(("Attempt to remove node 0x%x not in list!\n", paranode)); \
	} \
	else \
	{ \
		RemNode(paranode); \
	} \
}
#else
#define ParanoidRemNode(n) RemNode(n)
#endif


/**********************************************************************/
/**************************** Macros  *********************************/
/**********************************************************************/
#define MAX(a,b)    ((a)>(b)?(a):(b))
#define MIN(a,b)    ((a)<(b)?(a):(b))
#define ABS(x)      ((x<0)?(-(x)):(x))
#define CLIPRANGE(n,a,b) ((n)<(a) ? (a) : (n)>(b) ? (b) : (n))      /* range clipping */

#define CURRENTTASKITEM (CURRENTTASK->t.n_Item)

#define OWNEDBYCALLER(ItemPtr) ((((ItemNode *)ItemPtr)->n_Owner) == CURRENTTASKITEM)

extern Err IsItemOpened(Item task, Item it);
#define AUDIOFOLIOOPEN (IsItemOpened(CURRENTTASKITEM, AudioBase->af_Folio.fn.n_Item))
#define CHECKAUDIOOPEN if(AUDIOFOLIOOPEN < 0) \
	{ \
		ERR(("OpenAudioFolio() must be called by each thread!\n")); \
		return AF_ERR_AUDIOCLOSED; \
	}

/**********************************************************************/
/**************************** Constants  ******************************/
/**********************************************************************/

/*Hardware type */
#define BLUTO 0
#define SHERRIE 1

#define REALTIME AudioBase->af_Time
#define DEFAULT_SAMPLERATE (44100)

/* Tags for TestHack routine. */
#define HACK_TAG_LEVEL    101
#define HACK_TAG_READ_EO  102
#define HACK_TAG_READ_DMA 103
#define HACK_TAG_RSRC_NAME 104
#define HACK_TAG_RSRC_TYPE 105
#define HACK_TAG_TEST_DUCK 106

/**********************************************************************/
/**************************** Externs  ********************************/
/**********************************************************************/
extern int32 DAC_ControlValue;
extern int32 SetDAC_ControlValue;
extern AudioFolio *AudioBase;
extern Item gSplitExecTemplate;
extern Item gHeadAmpKnob;

/**********************************************************************/
/************************** Private SWIs ******************************/
/**********************************************************************/
int32 __swi(AUDIOSWI+7) TestHack( TagArg *TagList );
int32 __swi(AUDIOSWI+14) RunAudioSignalTask( Item DurKnob, void *startupdata ); /* Internal timer process SWI */    /* 940802: added startupdata */

/**********************************************************************/
/************************** Prototypes ********************************/
/**********************************************************************/
AudioAttachment *afi_CheckNextAttachment( AudioAttachment *aatt );
AudioTuning *GetInsTuning( AudioInstrument *ains);
Err Convert12TET_F16( int32 Semitones, int32 Cents, frac16 *FractionPtr );
Err swiBendInstrumentPitch( Item Instrument, frac16 BendFrac );
Item AllocInstrumentSpecial ( Item Template, uint8 Priority, int32 Specialness );
Item InitAudioErrors(void);
Item LoadInsExternal( char *InsName, Item AudioDevice);
Item LoadInsTempExt( char *InsName, Item AudioDevice, int32 InsMode );
Item SuperAttachSlave( Item Instrument, ItemNode *SlaveItem, char *FIFOName, uint32 Flags);
Item internalCreateAudioAttachment (AudioAttachment *aitp, TagArg *args);
Item internalCreateAudioCue (AudioCue *aitp, TagArg *args);
Item internalCreateAudioEnvelope (AudioEnvelope *aitp, TagArg *args);
Item internalCreateAudioIns (AudioInstrument *ains, TagArg *args);
Item internalCreateAudioItem (void *n, uint8 ntype, void *args);
Item internalCreateAudioKnob (AudioKnob *aknob, TagArg *args);
Item internalCreateAudioSample (AudioSample *samp, TagArg *args);
Item internalCreateAudioTemplate (AudioInsTemplate *aitp, TagArg *args);
Item internalCreateAudioTuning (AudioTuning *atun, TagArg *args);
Item internalDeleteAudioAttachment (AudioAttachment *aitp);
Item internalDeleteAudioCue (AudioCue *aitp);
Item internalDeleteAudioEnvelope (AudioEnvelope *aitp);
Item internalDeleteAudioIns (AudioInstrument *ains);
Item internalDeleteAudioSample (AudioSample *samp);
Item internalDeleteAudioTemplate (AudioInsTemplate *aitp);
Item internalOpenAudioSample (AudioSample *samp, void *args);
Item internalOpenAudioTemplate (AudioInsTemplate *aitp, void *args);
Item internalSetTuningInfo (AudioTuning *atun, TagArg *args);
Item swiAdoptInstrument( Item InsTemplate );
char *afi_AllocateString( char *s );
Err dsppInitDuckAndCover( void );
void dsppTermDuckAndCover( void );
int32 CalcSampleBaseFreq( AudioSample *asmp );
int32 CallAudioDevice(Item ReqItem, int32 cmd, void *send, int32 slen, void *recv, int32 rlen);
int32 DisableAttachmentSignal ( AudioAttachment *aatt );
int32 EnableAttachmentNext ( AudioAttachment *aatt, void *Address, uint32 Cnt );
int32 EnableAttachmentSignal ( AudioAttachment *aatt );
int32 HandleAIFFChunk( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize );
int32 HandleATNVChunk ( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize );
int32 HandleATNVForm ( iff_control *iffc, uint32 FormType , uint32 FormSize );
int32 HandleATSMChunk ( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize );
int32 HandleATSMForm ( iff_control *iffc, uint32 FormType , uint32 FormSize );
int32 HandleDMASignal( int32 Signals );
int32 HandleENVLChunk( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize );
int32 HandleTimerSignal( void );
int32 InitAudioDMAInterrupts ( void );
int32 InitAudioTimer( void );
int32 InitDefaultTuning( void );
int32 LoadAIFFSample (AudioSample *asmp, char *filename, int32 IfReadData, void *(*UserAllocator)());
int32 ParseAIFFForm( iff_control *iffc, int32 FormType, int32 FormSize );
int32 ParseENVLForm( iff_control *iffc, int32 FormType, int32 FormSize );
int32 PitchToFrequency( AudioTuning *atun, int32 Pitch, ufrac16 *FrequencyPtr);
int32 PostAudioEvent( AudioEvent *aevt, AudioTime Time );
int32 SetDMANextInt ( int32 DMAChan, int32 *Address, int32 Cnt );
int32 SignalMonitoringCue( AudioAttachment *aatt );
int32 TermAudioDMAInterrupts ( void );
int32 TermAudioTimer( void );
int32 TermDefaultTuning( void );
int32 TraceIndent( uint32 AndMask, uint32 OrMask, int32 Type);
int32 UnpostAudioEvent( AudioEvent *aevt );
int32 UpdateAllSampleBaseFreqs( void );
int32 afi_DeleteLinkedItems( List *ItemList );
int32 afi_DeleteReferencedItems( List *RefList );
int32 afi_DummyProcessor( void *i, void *p, uint32 tag, uint32 arg);
int32 afi_RemoveReferenceNode( List *RefList, Item RefItem);
int32 afi_ReportMemoryUsage( void );
int32 afi_SuperDeleteItem( Item it );
int32 afi_SuperDeleteItemNode( ItemNode *n );
Err afi_IsRamAddr( const char *p, int32 Size );
int32 internalCloseAudioSample (Item it, AudioSample *samp);
int32 internalCloseAudioTemplate (Item it, AudioInsTemplate *aitp);
int32 internalDeleteAudioItem (Item it, Task *t);
int32 internalDeleteAudioKnob (AudioKnob *aknob);
int32 internalDeleteAudioTuning (AudioTuning *atun);
int32 internalDumpEnvelope ( AudioEnvelope *aenv );
int32 internalDumpSample ( AudioSample *asmp );
int32 internalGetInstrumentInfo (AudioInstrument *ains, TagArg *args);
int32 internalGetKnobInfo ( AudioKnob *aknob, TagArg *args );
int32 internalGetSampleInfo (AudioSample *asmp, TagArg *args);
int32 internalLoadAIFFSample( AudioSample *asmp, iff_control *iffc, int32 IfReadData, int32 IfCompressed);
int32 internalSetEnvelopeInfo (AudioEnvelope *aenv, TagArg *args);
int32 internalSetSampleInfo (AudioSample *asmp, TagArg *args);
int32 lowSetAudioRate ( frac16 Rate );
Err swiAbandonInstrument( Item Instrument );
int32 swiAbortTimerCue( Item Cue );
int32 swiAllocAmplitude ( int32 Amplitude );
int32 swiConnectInstruments  ( Item SrcIns, char *SrcName,Item DstIns, char *DstName);
int32 swiDisconnectInstruments  ( Item SrcIns, char *SrcName,Item DstIns, char *DstName);
Err swiEnableAudioInput( int32 OnOrOff, TagArg *Tags );
int32 swiFreeAmplitude ( int32 Amplitude );
int32 swiLinkAttachments( Item At1, Item At2 );
int32 swiMonitorAttachment( Item Attachment, Item Cue, int32 Index );
void swiNotImplemented( void );
int32 swiPauseInstrument ( Item InstrumentItem);
int32 swiPutSampleInfo( Item SampleItem, TagArg *args );
int32 swiReleaseAttachment( Item Attachment, TagArg *tp );
int32 swiReleaseInstrument ( Item InstrumentItem,  TagArg *args);
int32 swiResumeInstrument ( Item InstrumentItem);
int32 swiRunAudioSignalTask( Item DurKnob, void *startupdata );     /* 940802: added startup data */
int32 swiScavengeInstrument( Item InsTemplate, uint8 Priority, uint32 MaxActivity, int32 IfSystemWide );
int32 swiSetAudioDuration ( Item Owner, uint32 Duration );
int32 swiSetAudioItemInfo(  Item AnyItem, TagArg *tp );
int32 swiSetAudioRate ( Item Owner, frac16 Rate );
int32 swiSetMasterTuning( ufrac16 Frequency );
int32 swiSignalAtTime( Item ATI, AudioTime Time);
int32 swiStartAttachment( Item Attachment, TagArg *tp );
int32 swiStartInstrument ( Item InstrumentItem, TagArg *args);
int32 swiStopAttachment(  Item Attachment, TagArg *tp );
int32 swiStopInstrument ( Item InstrumentItem, TagArg *args);
int32 swiTestHack ( TagArg *args);
int32 swiTuneInsTemplate( Item InsTemplate, Item Tuning );
int32 swiTuneInstrument( Item Instrument, Item Tuning );
int32 swiTweakKnob ( Item KnobItem, int32 Value );
int32 swiTweakRawKnob( Item KnobItem, int32 Value );
int32 swiWhereAttachment( Item Attachment );
uint32 CalcSampleNoteRate ( AudioInstrument *ains, AudioSample *asmp, int32 Note);
uint32 swiIncrementGlobalIndex( void );
uint32 swiTraceAudio ( uint32 Mask );
void  DSPPNextAttachment( AudioAttachment *aatt );
void  DSPPQueueAttachment( AudioAttachment *aatt );
void  DSPPReleaseSampleLoop( int32 DMAChan, AudioSample *asmp );
void  DefaultSample ( AudioSample *asmp);
void  EnableAttSignalIfNeeded( AudioAttachment *aatt );
void  ReportAvailMem( void );
void  afi_FreeString( char *s );
void SetEnvAttTimeScale( AudioAttachment *aatt, ufrac16 TimeScale );
void NotifyStartupSuccess (void *startupdata);
void Write16( uint16 *Addr,uint16 Data);
uint32 Read16( uint16 *Addr );

Err swiReadProbe ( Item ProbeItem, int32 *ValuePtr );
int32 internalDeleteAudioProbe (AudioProbe *aprob);
Item internalCreateAudioProbe (AudioProbe *aprobe, TagArg *args);

/*
    @@@ This gets a variable, but otherwise doesn't do anything.
        It's in the function table, but not in audio.h, so it's here.
*/
frac16 GetMasterTuning( void );

#endif
